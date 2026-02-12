# dual_cap

Coordinate two instances of dual_cap running on different hosts so that
both exit at the same time, triggered by new output appearing in a
monitored log file on either host. Packet capture integration is planned
but not yet implemented.


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

Rules:

- Exactly one of `init_ip` or `listen_port` must be present.
- `init_port` must be present if and only if `init_ip` is present.
- `mon_file` is always required.

### Example

Listener config (`listener.cfg`):

    listen_port=9877
    mon_file=/var/log/myapp.log

Initiator config (`initiator.cfg`):

    init_ip=192.168.1.10
    init_port=9877
    mon_file=/var/log/myapp.log


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


## Error Handling

The `E()` macro is used throughout. It takes an expression that
evaluates to true (non-zero) if there is an error. On error, it prints
the source file, line number, and the expression text, then exits.
The intent is minimal clutter, not user-friendly diagnostics.


## Known Limitations

- `volatile int exiting` is used for cross-thread signaling. This works
  on x86/x64 but is not formally correct per the C11 memory model.
  `_Atomic int` would be the proper alternative.
- Packet capture is not yet implemented.


## Testing

    ./tst.sh
