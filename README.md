# xv6-riscv — OS labs

A personal working fork of [MIT's xv6-riscv](https://github.com/mit-pdos/xv6-riscv) — a
teaching re-implementation of Unix Version 6 for a modern RISC-V multiprocessor — extended
with a series of classic OS lab exercises (shell utilities, paging, syscalls, copy-on-write,
a NIC driver, fine-grained locking, a larger filesystem, and mmap).

## Repository layout

Each lab is developed on its own branch, checked out from separate upstream starter code
and built on top of it. The `riscv` branch is the unmodified upstream base.

| Branch | Topic | Commit |
|---|---|---|
| [`riscv`](../../tree/riscv) | Base xv6-riscv (unmodified) | — |
| [`lab1`](../../tree/lab1) | Setup and Shell Utilities | [`99e6a35`](../../commit/99e6a35) |
| [`lab2`](../../tree/lab2) | Digging into Memory | [`6cf4d2f`](../../commit/6cf4d2f) |
| [`lab3`](../../tree/lab3) | Syscalls, Handle with Care | [`6516ee3`](../../commit/6516ee3) |
| [`lab4`](../../tree/lab4) | Copy on Write | [`2feaef1`](../../commit/2feaef1) |
| [`lab5`](../../tree/lab5) | Network Drivers | [`9729fc5`](../../commit/9729fc5) |
| [`lab6`](../../tree/lab6) | Locks | [`13c73eb`](../../commit/13c73eb) |
| [`lab7`](../../tree/lab7) | Filesystem | [`fe2383d`](../../commit/fe2383d), [`38eed02`](../../commit/38eed02) |
| [`lab8`](../../tree/lab8) | File Memory Mapping | [`2b2d1d1`](../../commit/2b2d1d1) |

## Building and running

You need a RISC-V toolchain and an emulator:

```bash
# Arch Linux
sudo pacman -S riscv64-linux-gnu-gcc qemu-system-riscv

# Debian/Ubuntu
sudo apt install gcc-riscv64-linux-gnu qemu-system-riscv
```

Then, on the branch of the lab you want to run:

```bash
git checkout labN
make qemu        # build and boot xv6 in QEMU
```

Exit QEMU with `Ctrl-A` then `X`. Written answers for each lab (where required) are in
`answers-labN.txt`/`.md` at the repository root of that branch.

## Labs

### Lab 1 — Setup and Shell Utilities
Toolchain/QEMU setup, then three user-space programs written from scratch:
- `watch <cmd>` — re-runs a command every *n* seconds (`-n` option to set the interval).
- `ls -R` — recursive directory listing (DFS).
- `sort` — sorts whitespace/newline-separated tokens with `strcmp`, building the result
  as a doubly linked list while reading; supports both a file argument and stdin (pipes).

### Lab 2 — Digging into Memory
Debugging xv6 with GDB (breakpoints, TUI mode, inspecting/patching live memory), then
diving into RISC-V page tables:
- `vmprint()` / the `kpgtbl` syscall — dumps a page table tree (address, PTE, physical
  address) in the same format as `freewalk`.
- **Superpages**: `sbrk()` requests of ≥ 2 MiB that land on a 2 MiB-aligned region are
  backed by a single RISC-V megapage instead of 512 regular 4 KiB pages, with
  `superalloc()`/`superfree()` in `kalloc.c` and support for promotion/demotion in
  `uvmcopy()`/`uvmunmap()`.

### Lab 3 — Syscalls, Handle with Care
- **Fast `getpid()`**: a read-only page mapped at `USYSCALL` in every process, holding a
  `struct usyscall` with the PID, so `ugetpid()` avoids a kernel crossing entirely.
- **`interpose(mask, path)`**: a new syscall that lets a process block a set of syscalls
  (by bitmask) for itself and its children, with an optional pathname allow-list for
  `open`/`exec` so e.g. only one specific file may be opened while everything else is
  rejected. Used by `user/sandbox.c`.

### Lab 4 — Copy-on-Write Fork
`fork()` no longer copies the parent's memory eagerly. `uvmcopy()` maps the parent's
physical pages read-only into the child instead; a write triggers a page fault, handled
in `vmfault()`, which allocates a fresh page, copies the data, and re-enables `PTE_W`.
Physical pages are reference-counted so they're freed only once the last page table
referencing them drops it. `copyout()` is updated to respect the same COW scheme.
Passes `cowtest` and `usertests -q`.

### Lab 5 — Network Drivers
- **E1000 NIC driver** (`kernel/e1000.c`): `e1000_transmit()`/`e1000_recv()` implementing
  DMA-based packet TX/RX over the driver's descriptor rings.
- **UDP/IP receive path** (`kernel/net.c`): `bind()`, `recv()` and `ip_rx()`, so user
  processes can bind a UDP port and block in `recv()` until a matching datagram arrives
  (per-port queue, capped at 16 packets, byte-order handling for header fields).
- Verified against `nettest` (`txone`, `rxone`, ping/DNS scenarios) with `tcpdump` traces
  over the QEMU-emulated LAN.

### Lab 6 — Locks
Reducing lock contention on multicore xv6:
- **Per-CPU memory allocator**: `kalloc.c` now keeps one free list + lock per CPU instead
  of a single global `kmem` lock, stealing from another CPU's list only when a core's own
  list is empty. Verified with `kalloctest` (contention counters) and `usertests sbrkmuch`.
- **Reader/writer spinlock** (`kernel/spinlock.c`/`.h`): `initrwlock`/`read_acquire`/
  `read_release`/`write_acquire`/`write_release`, with writer priority so pending writers
  aren't starved by a steady stream of readers. Used to replace the single `tickslock`
  around the `ticks` counter. Verified with `rwlktest`.

### Lab 7 — Filesystem
- **Large files**: added a doubly-indirect block to the inode layout (11 direct + 1
  singly-indirect + 1 doubly-indirect), raising the max file size from 268 to 65803
  blocks (`bmap()`/`itrunc()` in `kernel/fs.c`). Verified with `bigfile`.
- **Symbolic links**: new `symlink(target, path)` syscall, a `T_SYMLINK` inode type, and
  `O_NOFOLLOW` for `open()`. `open()` transparently follows symlink chains (bounded depth
  to detect cycles), while `link`/`unlink` operate on the link itself. Verified with
  `symlinktest`.

### Lab 8 — File Memory Mapping
`mmap()`/`munmap()` for memory-mapped files, backed by a per-process table of VMAs
(`kernel/proc.h`/`.c`). Mapping is lazy: `mmap()` only reserves address space, and actual
page-in happens on first page fault (reading the file via `readi()` and installing the
page with the right permissions). `MAP_SHARED` pages are written back to the file on
`munmap()` and on process exit; `fork()` duplicates the parent's VMAs (and the mapped
file's refcount) into the child. Verified with `mmaptest` and `usertests -q`.

---

## Acknowledgments

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix Version 6 (v6),
implemented for a modern RISC-V multiprocessor using ANSI C, developed and maintained by
MIT PDOS. It is inspired by John Lions's Commentary on UNIX 6th Edition (Peer to Peer
Communications; ISBN: 1-57398-013-7; 1st edition (June 14, 2000)). See also
<https://pdos.csail.mit.edu/6.1810/> for on-line resources for v6, and the
[upstream repository](https://github.com/mit-pdos/xv6-riscv) for the full list of
contributors.

These lab exercises are based on materials from
[MIT 6.1810](https://pdos.csail.mit.edu/6.1810/) (Operating Systems Engineering),
distributed under the [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) license.
