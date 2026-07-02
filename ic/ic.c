/*
 * syspy - lightweight real-time disk/network activity monitor
 *
 * Traces a target program (or attaches to a running PID) using ptrace(2)
 * and reports, as they happen, the syscalls that touch disk or network
 * resources: file opens/closes, reads/writes on tracked file descriptors,
 * socket creation, connects, binds, accepts and sends/receives.
 *
 * Linux x86_64 only. No external dependencies beyond libc.
 *
 * Usage:
 *   syspy [-o outfile] -- PROGRAM [ARGS...]     launch and trace PROGRAM
 *   syspy [-o outfile] -p PID                   attach to a running PID
 *
 * Build: see Makefile (make build)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>

/* ---------------------------------------------------------------------- */
/* Configuration                                                          */
/* ---------------------------------------------------------------------- */

#define MAX_FD        4096   /* highest fd number we track per process   */
#define MAX_TRACEES   256    /* max number of threads/processes tracked  */
#define PATH_BUF      256
#define ADDR_BUF      96

static FILE *out = NULL;             /* output stream (stdout or file)   */
static int   opt_show_meta = 1;      /* show unlink/rename/mkdir etc.    */

/* ---------------------------------------------------------------------- */
/* Resource tracking                                                      */
/* ---------------------------------------------------------------------- */

enum res_type { RES_NONE = 0, RES_FILE, RES_SOCKET };

struct fd_entry {
    enum res_type type;
    char path[PATH_BUF];   /* file path, or socket description          */
    char addr[ADDR_BUF];   /* peer/local address for sockets, if known   */
    long bytes_read;
    long bytes_written;
};

/* One fd table per traced pid, allocated lazily. */
struct proc_state {
    pid_t pid;
    int in_use;
    int in_syscall;             /* toggles between enter/exit stop       */
    long cur_syscall;           /* syscall number captured on enter      */
    long cur_args[6];           /* arguments captured on enter           */
    struct fd_entry *fds;       /* MAX_FD entries, lazily allocated      */
};

static struct proc_state procs[MAX_TRACEES];

static struct proc_state *proc_find(pid_t pid) {
    for (int i = 0; i < MAX_TRACEES; i++)
        if (procs[i].in_use && procs[i].pid == pid)
            return &procs[i];
    return NULL;
}

static struct proc_state *proc_add(pid_t pid) {
    struct proc_state *p = proc_find(pid);
    if (p) return p;
    for (int i = 0; i < MAX_TRACEES; i++) {
        if (!procs[i].in_use) {
            procs[i].in_use = 1;
            procs[i].pid = pid;
            procs[i].in_syscall = 0;
            procs[i].fds = calloc(MAX_FD, sizeof(struct fd_entry));
            return &procs[i];
        }
    }
    fprintf(stderr, "syspy: too many traced threads/processes (limit %d)\n",
            MAX_TRACEES);
    return NULL;
}

static void proc_remove(pid_t pid) {
    struct proc_state *p = proc_find(pid);
    if (!p) return;
    free(p->fds);
    memset(p, 0, sizeof(*p));
}

static struct fd_entry *fd_get(struct proc_state *p, long fd) {
    if (fd < 0 || fd >= MAX_FD) return NULL;
    return &p->fds[fd];
}

/* ---------------------------------------------------------------------- */
/* Output helpers                                                         */
/* ---------------------------------------------------------------------- */

static void ts_print(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tmv;
    localtime_r(&ts.tv_sec, &tmv);
    fprintf(out, "%02d:%02d:%02d.%03ld ", tmv.tm_hour, tmv.tm_min,
            tmv.tm_sec, ts.tv_nsec / 1000000);
}

static void log_line(pid_t pid, const char *tag, const char *fmt, ...) {
    ts_print();
    fprintf(out, "[pid %-6d] %-6s ", pid, tag);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fputc('\n', out);
    fflush(out);
}

/* ---------------------------------------------------------------------- */
/* Reading tracee memory                                                  */
/* ---------------------------------------------------------------------- */

/* Read a NUL-terminated string from the tracee's address space. */
static int read_tracee_str(pid_t pid, unsigned long addr, char *buf, size_t buflen) {
    if (addr == 0) { buf[0] = '\0'; return -1; }
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) { buf[0] = '\0'; return -1; }

    size_t got = 0;
    while (got < buflen - 1) {
        ssize_t n = pread(fd, buf + got, buflen - 1 - got, (off_t)(addr + got));
        if (n <= 0) break;
        char *nul = memchr(buf + got, '\0', (size_t)n);
        if (nul) { got = (size_t)(nul - buf); break; }
        got += (size_t)n;
    }
    buf[got] = '\0';
    close(fd);
    return 0;
}

/* Read a raw memory block from the tracee. */
static int read_tracee_mem(pid_t pid, unsigned long addr, void *buf, size_t len) {
    if (addr == 0) return -1;
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = pread(fd, buf, len, (off_t)addr);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

/* Format a sockaddr read from the tracee into a human readable string. */
static void format_sockaddr(pid_t pid, unsigned long addr_ptr, char *out_buf, size_t out_len) {
    struct sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    if (read_tracee_mem(pid, addr_ptr, &ss, sizeof(ss)) != 0) {
        snprintf(out_buf, out_len, "<unreadable>");
        return;
    }
    if (ss.ss_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
        snprintf(out_buf, out_len, "%s:%d", ip, ntohs(sin->sin_port));
    } else if (ss.ss_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
        char ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &sin6->sin6_addr, ip, sizeof(ip));
        snprintf(out_buf, out_len, "[%s]:%d", ip, ntohs(sin6->sin6_port));
    } else if (ss.ss_family == AF_UNIX) {
        struct sockaddr_un *sun = (struct sockaddr_un *)&ss;
        snprintf(out_buf, out_len, "unix:%s",
                 sun->sun_path[0] ? sun->sun_path : "<anonymous>");
    } else {
        snprintf(out_buf, out_len, "family=%d", ss.ss_family);
    }
}

/* ---------------------------------------------------------------------- */
/* Registers                                                              */
/* ---------------------------------------------------------------------- */

static int get_regs(pid_t pid, struct user_regs_struct *regs) {
    return ptrace(PTRACE_GETREGS, pid, NULL, regs);
}

/* ---------------------------------------------------------------------- */
/* Syscall handling                                                       */
/* ---------------------------------------------------------------------- */

static const char *human_bytes(long n, char *buf, size_t len) {
    if (n < 1024) snprintf(buf, len, "%ld B", n);
    else if (n < 1024 * 1024) snprintf(buf, len, "%.1f KB", n / 1024.0);
    else snprintf(buf, len, "%.1f MB", n / (1024.0 * 1024.0));
    return buf;
}

/* Called when a tracked syscall is entered (arguments available). */
static void on_syscall_enter(struct proc_state *p, struct user_regs_struct *r) {
    long sc = (long)r->orig_rax;
    p->cur_syscall = sc;
    p->cur_args[0] = (long)r->rdi;
    p->cur_args[1] = (long)r->rsi;
    p->cur_args[2] = (long)r->rdx;
    p->cur_args[3] = (long)r->r10;
    p->cur_args[4] = (long)r->r8;
    p->cur_args[5] = (long)r->r9;

    switch (sc) {
    case SYS_unlink: {
        char path[PATH_BUF];
        read_tracee_str(p->pid, (unsigned long)p->cur_args[0], path, sizeof(path));
        if (opt_show_meta) log_line(p->pid, "DISK", "delete    %s", path);
        break;
    }
    case SYS_unlinkat: {
        char path[PATH_BUF];
        read_tracee_str(p->pid, (unsigned long)p->cur_args[1], path, sizeof(path));
        if (opt_show_meta) log_line(p->pid, "DISK", "delete    %s", path);
        break;
    }
    case SYS_rename: {
        char oldp[PATH_BUF], newp[PATH_BUF];
        read_tracee_str(p->pid, (unsigned long)p->cur_args[0], oldp, sizeof(oldp));
        read_tracee_str(p->pid, (unsigned long)p->cur_args[1], newp, sizeof(newp));
        if (opt_show_meta) log_line(p->pid, "DISK", "rename    %s -> %s", oldp, newp);
        break;
    }
    case SYS_renameat:
    case SYS_renameat2: {
        char oldp[PATH_BUF], newp[PATH_BUF];
        read_tracee_str(p->pid, (unsigned long)p->cur_args[1], oldp, sizeof(oldp));
        read_tracee_str(p->pid, (unsigned long)p->cur_args[3], newp, sizeof(newp));
        if (opt_show_meta) log_line(p->pid, "DISK", "rename    %s -> %s", oldp, newp);
        break;
    }
    case SYS_mkdir: {
        char path[PATH_BUF];
        read_tracee_str(p->pid, (unsigned long)p->cur_args[0], path, sizeof(path));
        if (opt_show_meta) log_line(p->pid, "DISK", "mkdir     %s", path);
        break;
    }
    case SYS_mkdirat: {
        char path[PATH_BUF];
        read_tracee_str(p->pid, (unsigned long)p->cur_args[1], path, sizeof(path));
        if (opt_show_meta) log_line(p->pid, "DISK", "mkdir     %s", path);
        break;
    }
    default:
        break;
    }
}

/* Called when a tracked syscall exits (return value available). */
static void on_syscall_exit(struct proc_state *p, struct user_regs_struct *r) {
    long sc = p->cur_syscall;
    long ret = (long)r->rax;

    switch (sc) {
    case SYS_open:
    case SYS_openat: {
        int path_arg = (sc == SYS_open) ? 0 : 1;
        if (ret < 0) break; /* failed open, nothing to track */
        char path[PATH_BUF];
        read_tracee_str(p->pid, (unsigned long)p->cur_args[path_arg], path, sizeof(path));
        struct fd_entry *e = fd_get(p, ret);
        if (e) {
            e->type = RES_FILE;
            strncpy(e->path, path, sizeof(e->path) - 1);
            e->path[sizeof(e->path) - 1] = '\0';
            e->bytes_read = 0;
            e->bytes_written = 0;
        }
        log_line(p->pid, "DISK", "open      fd=%-3ld %s", ret, path);
        break;
    }
    case SYS_close: {
        long fd = p->cur_args[0];
        struct fd_entry *e = fd_get(p, fd);
        if (e && e->type != RES_NONE) {
            if (e->type == RES_FILE)
                log_line(p->pid, "DISK", "close     fd=%-3ld %s (r=%ld B, w=%ld B)",
                          fd, e->path, e->bytes_read, e->bytes_written);
            else
                log_line(p->pid, "NET", "close     fd=%-3ld %s (r=%ld B, w=%ld B)",
                          fd, e->addr[0] ? e->addr : "socket", e->bytes_read, e->bytes_written);
            memset(e, 0, sizeof(*e));
        }
        break;
    }
    case SYS_read:
    case SYS_pread64: {
        if (ret <= 0) break;
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        if (!e || e->type == RES_NONE) break;
        e->bytes_read += ret;
        char hb[32];
        if (e->type == RES_FILE)
            log_line(p->pid, "DISK", "read      fd=%-3ld %-24s %s",
                      p->cur_args[0], e->path, human_bytes(ret, hb, sizeof(hb)));
        else
            log_line(p->pid, "NET", "recv      fd=%-3ld %-24s %s",
                      p->cur_args[0], e->addr[0] ? e->addr : "socket", human_bytes(ret, hb, sizeof(hb)));
        break;
    }
    case SYS_write:
    case SYS_pwrite64: {
        if (ret <= 0) break;
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        if (!e || e->type == RES_NONE) break;
        e->bytes_written += ret;
        char hb[32];
        if (e->type == RES_FILE)
            log_line(p->pid, "DISK", "write     fd=%-3ld %-24s %s",
                      p->cur_args[0], e->path, human_bytes(ret, hb, sizeof(hb)));
        else
            log_line(p->pid, "NET", "send      fd=%-3ld %-24s %s",
                      p->cur_args[0], e->addr[0] ? e->addr : "socket", human_bytes(ret, hb, sizeof(hb)));
        break;
    }
    case SYS_socket: {
        if (ret < 0) break;
        struct fd_entry *e = fd_get(p, ret);
        if (e) {
            e->type = RES_SOCKET;
            e->path[0] = '\0';
            e->addr[0] = '\0';
            e->bytes_read = 0;
            e->bytes_written = 0;
        }
        long domain = p->cur_args[0], type = p->cur_args[1];
        const char *dom = (domain == AF_INET) ? "IPv4" :
                           (domain == AF_INET6) ? "IPv6" :
                           (domain == AF_UNIX) ? "unix" : "other";
        const char *ty = (type & SOCK_STREAM) ? "tcp" :
                          (type & SOCK_DGRAM) ? "udp" : "raw";
        log_line(p->pid, "NET", "socket    fd=%-3ld %s/%s", ret, dom, ty);
        break;
    }
    case SYS_connect: {
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        char addr[ADDR_BUF];
        format_sockaddr(p->pid, (unsigned long)p->cur_args[1], addr, sizeof(addr));
        if (e) {
            e->type = RES_SOCKET;
            strncpy(e->addr, addr, sizeof(e->addr) - 1);
        }
        if (ret == 0 || ret == -EINPROGRESS)
            log_line(p->pid, "NET", "connect   fd=%-3ld -> %s", p->cur_args[0], addr);
        else
            log_line(p->pid, "NET", "connect   fd=%-3ld -> %s FAILED (errno=%ld)",
                      p->cur_args[0], addr, -ret);
        break;
    }
    case SYS_bind: {
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        char addr[ADDR_BUF];
        format_sockaddr(p->pid, (unsigned long)p->cur_args[1], addr, sizeof(addr));
        if (e) {
            e->type = RES_SOCKET;
            strncpy(e->addr, addr, sizeof(e->addr) - 1);
        }
        log_line(p->pid, "NET", "bind      fd=%-3ld on %s", p->cur_args[0], addr);
        break;
    }
    case SYS_listen: {
        log_line(p->pid, "NET", "listen    fd=%-3ld backlog=%ld", p->cur_args[0], p->cur_args[1]);
        break;
    }
    case SYS_accept:
    case SYS_accept4: {
        if (ret < 0) break;
        char addr[ADDR_BUF] = "";
        if (p->cur_args[1])
            format_sockaddr(p->pid, (unsigned long)p->cur_args[1], addr, sizeof(addr));
        struct fd_entry *e = fd_get(p, ret);
        if (e) {
            e->type = RES_SOCKET;
            e->path[0] = '\0';
            strncpy(e->addr, addr, sizeof(e->addr) - 1);
            e->bytes_read = 0;
            e->bytes_written = 0;
        }
        log_line(p->pid, "NET", "accept    fd=%-3ld <- %s", ret, addr[0] ? addr : "unknown");
        break;
    }
    case SYS_sendto: {
        if (ret <= 0) break;
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        if (e) { e->type = RES_SOCKET; e->bytes_written += ret; }
        char addr[ADDR_BUF] = "";
        if (p->cur_args[4])
            format_sockaddr(p->pid, (unsigned long)p->cur_args[4], addr, sizeof(addr));
        char hb[32];
        log_line(p->pid, "NET", "sendto    fd=%-3ld %-24s %s",
                  p->cur_args[0], addr[0] ? addr : (e && e->addr[0] ? e->addr : "socket"),
                  human_bytes(ret, hb, sizeof(hb)));
        break;
    }
    case SYS_recvfrom: {
        if (ret <= 0) break;
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        if (e) { e->type = RES_SOCKET; e->bytes_read += ret; }
        char hb[32];
        log_line(p->pid, "NET", "recvfrom  fd=%-3ld %-24s %s",
                  p->cur_args[0], e && e->addr[0] ? e->addr : "socket",
                  human_bytes(ret, hb, sizeof(hb)));
        break;
    }
    case SYS_sendmsg: {
        if (ret <= 0) break;
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        if (e) { e->type = RES_SOCKET; e->bytes_written += ret; }
        char hb[32];
        log_line(p->pid, "NET", "sendmsg   fd=%-3ld %-24s %s",
                  p->cur_args[0], e && e->addr[0] ? e->addr : "socket",
                  human_bytes(ret, hb, sizeof(hb)));
        break;
    }
    case SYS_recvmsg: {
        if (ret <= 0) break;
        struct fd_entry *e = fd_get(p, p->cur_args[0]);
        if (e) { e->type = RES_SOCKET; e->bytes_read += ret; }
        char hb[32];
        log_line(p->pid, "NET", "recvmsg   fd=%-3ld %-24s %s",
                  p->cur_args[0], e && e->addr[0] ? e->addr : "socket",
                  human_bytes(ret, hb, sizeof(hb)));
        break;
    }
    case SYS_unlink:
    case SYS_unlinkat:
    case SYS_rename:
    case SYS_renameat:
    case SYS_renameat2:
    case SYS_mkdir:
    case SYS_mkdirat:
        /* already logged at syscall-enter, nothing to add on exit */
        break;
    default:
        break;
    }
}

/* Decide whether a syscall number is one we care about. Filtering here
 * (instead of only in the enter/exit handlers) keeps the ptrace loop fast
 * for the vast majority of syscalls we ignore. */
static int is_tracked_syscall(long sc) {
    switch (sc) {
    case SYS_open: case SYS_openat: case SYS_close:
    case SYS_read: case SYS_write: case SYS_pread64: case SYS_pwrite64:
    case SYS_socket: case SYS_connect: case SYS_bind: case SYS_listen:
    case SYS_accept: case SYS_accept4:
    case SYS_sendto: case SYS_recvfrom: case SYS_sendmsg: case SYS_recvmsg:
    case SYS_unlink: case SYS_unlinkat:
    case SYS_rename: case SYS_renameat: case SYS_renameat2:
    case SYS_mkdir: case SYS_mkdirat:
        return 1;
    default:
        return 0;
    }
}

/* ---------------------------------------------------------------------- */
/* ptrace options / event handling                                        */
/* ---------------------------------------------------------------------- */

#define TRACE_OPTS (PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | \
                    PTRACE_O_TRACEFORK    | PTRACE_O_TRACEVFORK | \
                    PTRACE_O_TRACEEXEC    | PTRACE_O_EXITKILL)

static void resume(pid_t pid, int sig) {
    ptrace(PTRACE_SYSCALL, pid, NULL, (void *)(long)sig);
}

/* Main event loop: waits for any traced thread/process to stop and
 * dispatches syscall enter/exit events. */
static int trace_loop(pid_t initial_pid) {
    int status;
    int active = 1; /* number of tracees we believe are still alive */

    for (;;) {
        pid_t pid = waitpid(-1, &status, __WALL);
        if (pid < 0) {
            if (errno == ECHILD) break; /* no more tracees */
            if (errno == EINTR) continue;
            perror("waitpid");
            break;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            proc_remove(pid);
            active--;
            if (pid == initial_pid || active <= 0) {
                if (active <= 0) break;
            }
            continue;
        }

        if (!WIFSTOPPED(status)) continue;

        int sig = WSTOPSIG(status);
        int event = status >> 16;

        struct proc_state *p = proc_find(pid);
        if (!p) { p = proc_add(pid); active++; }

        if (event == PTRACE_EVENT_CLONE || event == PTRACE_EVENT_FORK ||
            event == PTRACE_EVENT_VFORK) {
            /* A new thread/process id is available via GETEVENTMSG, but it
             * will also show up on its own the first time it stops, so we
             * simply resume the parent here. */
            resume(pid, 0);
            continue;
        }

        if (event == PTRACE_EVENT_EXEC) {
            resume(pid, 0);
            continue;
        }

        if (sig == (SIGTRAP | 0x80)) {
            /* Genuine syscall-stop (PTRACE_O_TRACESYSGOOD guarantees this
             * bit is only set for syscall stops, not other traps). */
            struct user_regs_struct regs;
            if (get_regs(pid, &regs) == 0) {
                long sc = (long)regs.orig_rax;
                if (!p->in_syscall) {
                    p->in_syscall = 1;
                    if (is_tracked_syscall(sc))
                        on_syscall_enter(p, &regs);
                    else
                        p->cur_syscall = -1; /* mark as untracked */
                } else {
                    p->in_syscall = 0;
                    if (p->cur_syscall != -1)
                        on_syscall_exit(p, &regs);
                }
            }
            resume(pid, 0);
            continue;
        }

        if (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU) {
            resume(pid, 0);
            continue;
        }

        /* Any other signal: pass it through to the tracee untouched. */
        resume(pid, sig);
    }
    return 0;
}

/* ---------------------------------------------------------------------- */
/* Launch / attach                                                        */
/* ---------------------------------------------------------------------- */

static pid_t launch_target(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[0], argv);
        fprintf(stderr, "syspy: failed to exec '%s': %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0); /* wait for the initial exec-triggered stop */
    ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)(long)TRACE_OPTS);

    struct proc_state *p = proc_add(pid);
    (void)p;
    resume(pid, 0);
    return pid;
}

static pid_t attach_target(pid_t pid) {
    if (ptrace(PTRACE_SEIZE, pid, NULL, (void *)(long)TRACE_OPTS) < 0) {
        perror("ptrace(PTRACE_SEIZE)");
        exit(1);
    }
    if (ptrace(PTRACE_INTERRUPT, pid, NULL, NULL) < 0) {
        perror("ptrace(PTRACE_INTERRUPT)");
        exit(1);
    }
    int status;
    waitpid(pid, &status, 0);
    proc_add(pid);
    resume(pid, 0);
    return pid;
}

/* ---------------------------------------------------------------------- */
/* CLI                                                                    */
/* ---------------------------------------------------------------------- */

static void usage(const char *prog) {
    fprintf(stderr,
        "usage:\n"
        "  %s [-o outfile] [-q] -- PROGRAM [ARGS...]   launch and trace PROGRAM\n"
        "  %s [-o outfile] [-q] -p PID                 attach to a running PID\n"
        "\n"
        "  -o outfile   write output to outfile instead of stdout\n"
        "  -q           quiet: hide filesystem metadata ops (unlink/rename/mkdir)\n",
        prog, prog);
}

int main(int argc, char **argv) {
    out = stdout;
    pid_t attach_pid = -1;
    int i = 1;

    for (; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out = fopen(argv[++i], "w");
            if (!out) { perror("fopen"); return 1; }
        } else if (strcmp(argv[i], "-q") == 0) {
            opt_show_meta = 0;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            attach_pid = (pid_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            break;
        }
    }

    setvbuf(out, NULL, _IOLBF, 0); /* line-buffered for real-time output */

    if (attach_pid > 0) {
        fprintf(stderr, "syspy: attaching to pid %d (Ctrl-C to stop)\n", attach_pid);
        attach_target(attach_pid);
    } else {
        if (i >= argc) {
            usage(argv[0]);
            return 1;
        }
        fprintf(stderr, "syspy: launching '%s'\n", argv[i]);
        launch_target(&argv[i]);
    }

    trace_loop(attach_pid > 0 ? attach_pid : 0);

    fprintf(stderr, "syspy: done\n");
    if (out != stdout) fclose(out);
    return 0;
}
