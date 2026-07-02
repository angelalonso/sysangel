# ic

A lightweight, real-time disk/network activity monitor for Linux (x86_64).

It uses `ptrace(2)` to attach to a program (or launch one directly) and
prints every syscall that touches disk or network resources as it happens:
file opens/reads/writes/closes, deletes, renames, mkdirs, socket creation,
connects, binds, accepts, and sends/receives (with byte counts).

Everything else (CPU-only work, unrelated syscalls) is filtered out, so the
output stays focused on "what is this program actually touching on disk or
over the network."

## Build

    make build

Produces `bin/ic`.

## Run against a program

    make run ARGS="curl -s https://example.com"

or directly:

    ./bin/ic -- /path/to/program --its --args

## Attach to a running process

    ./bin/ic -p <PID>

(You need permission to ptrace the target: same user, or root, and
`/proc/sys/kernel/yama/ptrace_scope` must allow it.)

## Options

    -o outfile   write output to a file instead of stdout
    -q           hide filesystem metadata ops (unlink/rename/mkdir)

## Test

    make test

Builds `ic` plus a small bundled demo program (`tests/demo_target.c`)
that writes/reads/deletes a temp file and attempts a TCP connect, then runs
ic against it so you can see real output immediately.

## Notes / limitations

- Linux x86_64 only (reads syscall arguments from `user_regs_struct` and
  uses raw syscall numbers from `<sys/syscall.h>`).
- Tracks forked/cloned children and execs via `PTRACE_O_TRACECLONE` /
  `TRACEFORK` / `TRACEVFORK` / `TRACEEXEC`, so multi-process programs are
  followed automatically.
- File descriptor state (path/socket info, byte counters) is tracked
  per-process from the moment ic starts watching; fds inherited from
  before that point are only identified the first time they are used.
- This is not a security sandbox: it observes, it does not block anything.
