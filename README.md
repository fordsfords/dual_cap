# dual_cap

Coordinate two instances of dual_cap running on different hosts so that
both exit at the same time, triggered by new output appearing in a
monitored log file on either host.  Each instance can optionally run a
packet capture command (e.g. `tshark`) in the background, which is
stopped shortly after the trigger fires.


## Overview

Two instances form a pair: one listens for a TCP connection, the other
initiates it. Each instance monitors a local log file. When either
instance detects a new line in its log file, it signals the other
instance over the TCP connection, and both exit.

Typical deployment: start the listener first, then the initiator.
The listener blocks until the initiator connects.


## Building

Unix (including WSL2):

    ./bld.sh

Windows: compile `dual_cap.c` and `plat_win.c` together, linking
against `ws2_32.lib`. For MSVC:

    cl dual_cap.c plat_win.c ws2_32.lib

## Usage

    ./dual_cap <config_file>

The config file is a text file with `key=value` lines.
Blank lines and lines starting with `#` are ignored.
Unknown keys are rejected.

### Config Keys

| Key | Type | Description |
|---|---|---|
| `init_ip` | IPv4 address | Remote listener address (initiator only) |
| `init_port` | integer | Remote listener port (initiator only) |
| `listen_port` | integer | Port to listen on (listener only) |
| `mon_file` | file path | Log file to monitor for new output |
| `cap_cmd` | command line | Capture command to run in background (optional) |
| `cap_linger_ms` | integer | Milliseconds to keep capturing after trigger (optional, default 0) |

Rules:

- Exactly one of `init_ip` or `listen_port` must be present.
- `init_port` must be present if and only if `init_ip` is present.
- `mon_file` is always required.
- `cap_cmd` is optional. If present, the command is launched before the
  peer connection is established and killed after the trigger fires
  (plus any `cap_linger_ms` delay).
- `cap_linger_ms` is optional. Only meaningful if `cap_cmd` is present.

### Example

Listener config (`listener.cfg`):

    listen_port=9877
    mon_file=/var/log/myapp.log
    cap_cmd=tshark -i eth0 -w /tmp/caps/listener.pcapng -b filesize:10240 -b files:5 -q
    cap_linger_ms=1000

Initiator config (`initiator.cfg`):

    init_ip=192.168.1.10
    init_port=9877
    mon_file=/var/log/myapp.log
    cap_cmd=tshark -i eth0 -w /tmp/caps/initiator.pcapng -b filesize:10240 -b files:5 -q
    cap_linger_ms=1000


## File Structure

| File | Purpose |
|---|---|
| `dual_cap.c` | Application: config parsing, file monitoring thread, peer communication thread, main |
| `plat.h` | Platform abstraction: typedefs, function declarations, platform-specific headers |
| `plat_unix.c` | Unix implementations of platform functions |
| `plat_win.c` | Windows implementations of platform functions |
| `bld.sh` | Unix build |
| `tst.sh` | Basic integration test (Unix) |
| `clean.sh` | Remove test files (Unix) |

## Platform Notes

The code is written to be portable between Unix and Windows. Platform
differences are isolated in `plat_unix.c` and `plat_win.c`, with one
exception: `SIGPIPE` is ignored on Unix in `main()` via an `#ifdef`
block, since Windows does not generate `SIGPIPE` for sockets.

### Windows-Specific Concerns

**File monitoring.** Some Windows C runtimes cache file metadata after
hitting EOF, causing the tail-f pattern (`clearerr` + `fgets`) to miss
new data. The code works around this with `fseek(fp, 0, SEEK_CUR)` to
force the runtime to recheck the file. This call is a no-op on Unix.

**`SO_REUSEADDR`** has different semantics on Windows than on Unix. On
Unix it allows binding to a port in TIME_WAIT. On Windows it allows
multiple sockets to bind the same port, which is a security risk. The
current code uses `SO_REUSEADDR` unconditionally. If this becomes a
concern, it should be made platform-conditional.

**`select()` first argument.** Windows ignores the first argument to
`select()`. On 64-bit Windows, `SOCKET` is 64-bit, so the cast
`(int)(peer + 1)` may produce a compiler warning. This is harmless but
could be cleaned up with a platform macro.


## Capture Integration

The `cap_cmd` config key specifies a packet capture command (typically
`tshark` or `tcpdump`) to run in the background.  The capture starts
before the peer connection is established so it is already running when
application traffic begins.  After the trigger fires, the capture
continues for `cap_linger_ms` milliseconds to catch trailing packets,
then the capture process is killed.

Recommended `tshark` options for long-running captures:

- `-b filesize:<KB> -b files:<N>` creates a ring buffer of N files,
  bounding disk usage.
- `-q` suppresses per-packet output to stdout.

**Shutdown behavior.** On Unix, the capture process group receives
`SIGTERM`, which causes `tshark` and `dumpcap` to flush and exit
cleanly.  On Windows, `TerminateProcess` is used (a hard kill).  With
ring-buffer files, only the currently-writing file may be truncated;
already-rotated files are intact on disk.


## Error Handling

The `E()` macro is used throughout. It takes an expression that
evaluates to true (non-zero) if there is an error. On error, it prints
the source file, line number, and the expression text, then exits.
The intent is minimal clutter, not user-friendly diagnostics.


## Known Limitations

- `volatile int exiting` is used for cross-thread signaling. This works
  on x86/x64 but is not formally correct per the C11 memory model.
  `_Atomic int` would be the proper alternative.
- On Windows, `TerminateProcess` is used to stop the capture command.
  This is a hard kill; the currently-writing capture file may be
  truncated.  Use ring-buffer mode (`-b filesize:... -b files:...`)
  to limit exposure.


## Testing

    ./tst.sh
