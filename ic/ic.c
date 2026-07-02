/*
 * ic - lightweight real-time disk/network activity monitor
 * Linux x86_64 only.
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
#include <dirent.h>
#include <pthread.h>
#include <pwd.h>

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>

/* Inline deployment of the webview components */
#define WEBVIEW_GTK
#define WEBVIEW_IMPLEMENTATION
#include "webview.h"

/* Forward declarations */
static void usage(const char *prog);
static void trace_loop(pid_t initial_pid);
static int attach_target(pid_t pid);
static void launch_target(char *const argv[]);
static void log_line(pid_t pid, const char *tag, const char *fmt, ...);
static void external_invoke_handler(struct webview *w, const char *arg);
static void send_processes_to_ui(struct webview *w);

#define MAX_FD        4096   
#define PATH_BUF      256

static FILE *out = NULL;             
static int   opt_show_meta = 1;      
static struct webview w_instance;   
static int is_gui_mode = 0;

/* --- Tracer Logic & Diagnostic Explanations --- */

static void get_process_owner_name(pid_t pid, char *out_buf, size_t max_len) {
    char comm_path[64];
    snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
    struct stat st;
    if (stat(comm_path, &st) == 0) {
        struct passwd *pwd = getpwuid(st.st_uid);
        if (pwd) {
            snprintf(out_buf, max_len, "%s", pwd->pw_name);
            return;
        }
    }
    snprintf(out_buf, max_len, "unknown");
}

static int attach_target(pid_t pid) {
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        int err = errno;
        char target_user[64] = {0};
        get_process_owner_name(pid, target_user, sizeof(target_user));

        char current_user[64] = "unknown";
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) {
            snprintf(current_user, sizeof(current_user), "%s", pwd->pw_name);
        }

        if (err == EPERM) {
            char yama_val = '0';
            int yama_fd = open("/proc/sys/kernel/yama/ptrace_scope", O_RDONLY);
            if (yama_fd >= 0) {
                read(yama_fd, &yama_val, 1);
                close(yama_fd);
            }

            if (yama_val != '0' && getuid() != 0) {
                log_line(pid, "DENIED", "I am user '%s', cannot attach to PID %d (owned by '%s') because "
                         "Yama ptrace_scope is set to %c. Run 'sudo sysctl kernel.yama.ptrace_scope=0' to allow attachment.",
                         current_user, pid, target_user, yama_val);
            } else {
                log_line(pid, "DENIED", "I am user '%s', cannot attach to PID %d (owned by '%s'): "
                         "Insufficient permissions or target is un-traceable.", current_user, pid, target_user);
            }
        } else if (err == ESRCH) {
            log_line(pid, "ERROR", "No process exists with PID %d.", pid);
        } else {
            log_line(pid, "ERROR", "Failed to attach: %s", strerror(err));
        }
        return -1;
    }
    int status;
    waitpid(pid, &status, 0);
    
    /* Set options to automatically trace children spawned via fork, vfork, or clone */
    ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)(size_t)(PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE));
    ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    log_line(pid, "SYSTEM", "Successfully attached to target PID");
    return 0;
}

static void launch_target(char *const argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        raise(SIGSTOP);
        execvp(argv[0], argv);
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        
        /* Ensure newly launched process passes trace properties to its children */
        ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)(size_t)(PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE));
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        log_line(pid, "SYSTEM", "Launched program via fork trace");
    } else {
        perror("fork failed");
    }
}

static void trace_loop(pid_t initial_pid) {
    int status;
    int active_tracees = 1; // Track the total count of running processes/threads

    while (active_tracees > 0) {
        pid_t trapped_pid = waitpid(-1, &status, __WALL);
        if (trapped_pid < 0) {
            if (errno == ECHILD) break;
            continue;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            if (WIFEXITED(status)) {
                log_line(trapped_pid, "SYSTEM", "Process exited with status %d", WEXITSTATUS(status));
            } else {
                log_line(trapped_pid, "SYSTEM", "Process terminated by signal %d", WTERMSIG(status));
            }
            active_tracees--;
            continue;
        }

        if (WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            
            // Check if this stop corresponds to a ptrace-extended fork/clone event
            int event = status >> 16;
            if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK || event == PTRACE_EVENT_CLONE) {
                pid_t new_child_pid;
                if (ptrace(PTRACE_GETEVENTMSG, trapped_pid, NULL, &new_child_pid) == 0) {
                    log_line(trapped_pid, "SYSTEM", "Spawned new child worker (PID: %d)", new_child_pid);
                    active_tracees++;
                }
            }

            if (sig == SIGTRAP) {
                struct user_regs_struct regs;
                if (ptrace(PTRACE_GETREGS, trapped_pid, NULL, &regs) == 0) {
                    long syscall_num = regs.orig_rax;
                    if (syscall_num == SYS_read || syscall_num == SYS_write) {
                        log_line(trapped_pid, syscall_num == SYS_read ? "READ" : "WRITE", 
                                 "Syscall %ld executed on descriptor %lld", syscall_num, regs.rdi);
                    }
                }
            }
            
            // Deliver the signal unless it's the expected ptrace internal trap
            ptrace(PTRACE_SYSCALL, trapped_pid, NULL, (void *)(size_t)(sig == SIGTRAP ? 0 : sig));
        }
    }
    
    if (!is_gui_mode) {
        fprintf(out, "[SYSTEM] All tracked application processes have completed execution.\n");
    }
}

/* --- Logging & UI Communications Helper --- */

static void ts_print(char *buf, size_t len) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tmv;
    localtime_r(&ts.tv_sec, &tmv);
    snprintf(buf, len, "%02d:%02d:%02d.%03ld", tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ts.tv_nsec / 1000000);
}

struct eval_dispatch_args {
    char *script;
};

static void dispatch_eval_cb(struct webview *w, void *arg) {
    struct eval_dispatch_args *args = (struct eval_dispatch_args *)arg;
    if (args && args->script) {
        webview_eval(w, args->script);
        free(args->script);
    }
    free(args);
}

static void log_line(pid_t pid, const char *tag, const char *fmt, ...) {
    char ts[32];
    ts_print(ts, sizeof(ts));
    
    char payload[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(payload, sizeof(payload), fmt, ap);
    va_end(ap);

    if (is_gui_mode) {
        char escaped[3072] = {0};
        char *dst = escaped;
        char formatted_msg[1500];
        snprintf(formatted_msg, sizeof(formatted_msg), "%s [pid %-6d] %-6s %s", ts, pid, tag, payload);

        for (char *src = formatted_msg; *src && (dst - escaped < 3000); src++) {
            if (*src == '\\' || *src == '\'' || *src == '"') {
                *dst++ = '\\'; *dst++ = *src;
            } else if (*src == '\n') {
                *dst++ = '\\'; *dst++ = 'n';
            } else if (*src == '\r') {
                *dst++ = '\\'; *dst++ = 'r';
            } else {
                *dst++ = *src;
            }
        }
        
        struct eval_dispatch_args *dargs = malloc(sizeof(struct eval_dispatch_args));
        if (dargs) {
            dargs->script = malloc(4096);
            snprintf(dargs->script, 4096, "if(window.appendLogEntry){ window.appendLogEntry(\"%s\"); }", escaped);
            webview_dispatch(&w_instance, dispatch_eval_cb, dargs);
        }
    } else {
        fprintf(out, "%s [pid %-6d] %-6s %s\n", ts, pid, tag, payload);
        fflush(out);
    }
}

static void send_processes_to_ui(struct webview *w) {
    DIR *dir = opendir("/proc");
    if (!dir) return;
    
    size_t cap = 256 * 1024;
    char *json = malloc(cap);
    size_t offset = snprintf(json, cap, "[");
    struct dirent *de;
    int first = 1;

    while ((de = readdir(dir))) {
        if (!isdigit(de->d_name[0])) continue;
        int pid = atoi(de->d_name);
        if (pid <= 1) continue;
        
        char comm_path[64], comm[256] = {0};
        snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
        
        char user_name[64] = {0};
        get_process_owner_name(pid, user_name, sizeof(user_name));

        int fd = open(comm_path, O_RDONLY);
        if (fd >= 0) {
            ssize_t r = read(fd, comm, sizeof(comm) - 1);
            if (r > 0) {
                while (r > 0 && (comm[r-1] == '\n' || comm[r-1] == '\r')) comm[--r] = '\0';
            }
            close(fd);
        }
        if (strlen(comm) == 0) continue;

        char clean_name[256] = {0};
        char *cdst = clean_name;
        for (char *csrc = comm; *csrc && (cdst - clean_name < 250); csrc++) {
            if (*csrc == '"' || *csrc == '\\') { *cdst++ = ' '; }
            else { *cdst++ = *csrc; }
        }

        if (!first) { offset += snprintf(json + offset, cap - offset, ","); }
        first = 0;
        offset += snprintf(json + offset, cap - offset, "{\"pid\":%d,\"name\":\"%s\",\"owner\":\"%s\"}", pid, clean_name, user_name);
    }
    closedir(dir);
    snprintf(json + offset, cap - offset, "]");

    struct eval_dispatch_args *dargs = malloc(sizeof(struct eval_dispatch_args));
    if (dargs) {
        dargs->script = malloc(cap + 1024);
        snprintf(dargs->script, cap + 1024, "if(window.renderProcesses){ window.renderProcesses(%s); }", json);
        webview_dispatch(w, dispatch_eval_cb, dargs);
    }
    free(json);
}

struct trace_thread_args {
    pid_t pid;
};

static void *trace_worker_thread(void *arg) {
    struct trace_thread_args *targs = (struct trace_thread_args *)arg;
    trace_loop(targs->pid);
    free(targs);
    return NULL;
}

static void external_invoke_handler(struct webview *w, const char *arg) {
    if (strcmp(arg, "refresh") == 0) {
        send_processes_to_ui(w);
    } 
    else if (strncmp(arg, "trace:", 6) == 0) {
        int target_pid = atoi(arg + 6);
        if (target_pid > 0) {
            if (attach_target(target_pid) == 0) {
                pthread_t tid;
                struct trace_thread_args *targs = malloc(sizeof(struct trace_thread_args));
                if (targs) {
                    targs->pid = target_pid;
                    pthread_create(&tid, NULL, trace_worker_thread, targs);
                    pthread_detach(tid);
                    log_line(target_pid, "SYSTEM", "Async tracking loop deployed successfully.");
                }
            }
        }
    }
}

const char *html_ui = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset=\"UTF-8\">"
"<style>"
"  body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; background-color: #1e1e24; color: #e3e3e6; padding: 20px; margin: 0; }"
"  h2 { font-size: 18px; color: #ffffff; margin-top: 0; margin-bottom: 12px; font-weight: 500; }"
"  .container { max-width: 1000px; margin: 0 auto; }"
"  .card { background-color: #2a2a32; border-radius: 6px; padding: 20px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin-bottom: 20px; }"
"  .header-actions { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }"
"  input.search { background: #121214; border: 1px solid #444; color: white; padding: 8px 12px; border-radius: 4px; width: 250px; font-size: 14px; }"
"  button { background-color: #03dac6; color: #000; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; font-weight: bold; font-size: 13px; }"
"  button:hover { background-color: #01bfa5; }"
"  #process-list { max-height: 220px; overflow-y: auto; background-color: #121214; border: 1px solid #444; border-radius: 4px; }"
"  .proc-item { display: flex; justify-content: space-between; padding: 10px 15px; border-bottom: 1px solid #222; cursor: pointer; font-size: 14px; }"
"  .proc-item:hover { background-color: #2a2a32; color: #03dac6; }"
"  .proc-left { display: flex; gap: 15px; }"
"  .proc-pid { color: #888; font-family: monospace; width: 70px; }"
"  .proc-owner { color: #ffb74d; font-size: 12px; padding: 1px 6px; background: #332211; border-radius: 3px; font-family: monospace; }"
"  #logs { background-color: #121214; color: #a5d6ff; height: 280px; overflow-y: scroll; padding: 15px; font-family: monospace; font-size: 13px; white-space: pre-wrap; border: 1px solid #444; border-radius: 4px; margin-top: 10px; }"
"</style>"
"<script>"
"  if (!window.external) { window.external = {}; }"
"  if (!window.external.invoke) {"
"    window.external.invoke = function(arg) {"
"      if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.external) {"
"        window.webkit.messageHandlers.external.postMessage(arg);"
"      }"
"    };"
"  }"
""
"  let allProcesses = [];"
"  window.appendLogEntry = function(msg) {"
"    const el = document.getElementById('logs');"
"    if (el) {"
"      el.textContent += msg + '\\n';"
"      el.scrollTop = el.scrollHeight;"
"    }"
"  };"
"  function requestRefresh() {"
"    window.external.invoke('refresh');"
"  }"
"  window.renderProcesses = function(procsList) {"
"    try {"
"      allProcesses = procsList || [];"
"      filterAndRender();"
"    } catch(e) { window.appendLogEntry('[UI ERROR] Failed parsing: ' + e); }"
"  };"
"  function filterAndRender() {"
"    const query = document.getElementById('search-bar').value.toLowerCase();"
"    const list = document.getElementById('process-list');"
"    if (!list) return;"
"    list.innerHTML = '';"
"    const filtered = allProcesses.filter(p => "
"      p.name.toLowerCase().includes(query) || "
"      p.pid.toString().includes(query) ||"
"      (p.owner && p.owner.toLowerCase().includes(query))"
"    );"
"    if(filtered.length === 0) {"
"      list.innerHTML = '<div style=\"padding:15px; color:#666;\">No matching processes discovered</div>';"
"      return;"
"    }"
"    filtered.forEach(p => {"
"      const item = document.createElement('div');"
"      item.className = 'proc-item';"
"      item.innerHTML = '<div class=\"proc-left\"><span class=\"proc-pid\">PID: ' + p.pid + '</span><span>' + p.name + '</span></div><span class=\"proc-owner\">' + (p.owner || 'unknown') + '</span>';"
"      item.onclick = function() { startTrace(p.pid, p.name); };"
"      list.appendChild(item);"
"    });"
"  }"
"  function startTrace(pid, name) {"
"    window.appendLogEntry('[SYSTEM] Requesting trace attachment to ' + name + ' (PID: ' + pid + ')...');"
"    window.external.invoke('trace:' + pid);"
"  }"
"  window.onload = function() { "
"    window.appendLogEntry('[SYSTEM] Frontend UI mounted completely.');"
"    setTimeout(requestRefresh, 400);"
"  };"
"</script>"
"</head>"
"<body>"
"  <div class=\"container\">"
"    <div class=\"card\">"
"      <div class=\"header-actions\">"
"        <h2>Active System Processes</h2>"
"        <div>"
"          <input type=\"text\" id=\"search-bar\" class=\"search\" placeholder=\"Filter by name, PID, or owner...\" oninput=\"filterAndRender()\" />"
"          <button onclick=\"requestRefresh()\" style=\"margin-left: 8px;\">Refresh</button>"
"        </div>"
"      </div>"
"      <div id=\"process-list\"><div style=\"padding:15px; color:#666;\">Scanning system target tree...</div></div>"
"    </div>"
"    <div class=\"card\">"
"      <h2>Live Trace Activity Streams</h2>"
"      <div id=\"logs\">[Engine Dashboard Frame Painted]</div>"
"    </div>"
"  </div>"
"</body>"
"</html>";

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

    setvbuf(out, NULL, _IOLBF, 0);

    if (attach_pid > 0 || i < argc) {
        if (attach_pid > 0) {
            if (attach_target(attach_pid) < 0) return 1;
        } else {
            launch_target(&argv[i]);
        }
        trace_loop(attach_pid > 0 ? attach_pid : 0);
    } 
    else {
        is_gui_mode = 1;
        
        memset(&w_instance, 0, sizeof(w_instance));
        w_instance.title = "IC Monitor Hub";
        w_instance.width = 950;
        w_instance.height = 700;
        w_instance.resizable = 1;
        w_instance.external_invoke_cb = external_invoke_handler;
        w_instance.url = "about:blank";

        if (webview_init(&w_instance) != 0) {
            return 1;
        }
        
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(w_instance.priv.webview), html_ui, NULL);
        
        while (webview_loop(&w_instance, 1) == 0) {}
        webview_exit(&w_instance);
    }

    if (out != stdout && out != NULL) fclose(out);
    return 0;
}

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s [-o outfile] [-p PID]\n", prog);
}
