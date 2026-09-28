# xv6-riscv

My fork of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv), the small Unix-like teaching kernel
from MIT, with a set of OS labs done on top of it. Each lab lives on its own branch, `riscv` is
plain upstream. Everything is C and runs on RISC-V under QEMU.

## What's on each branch

### [`lab1`](../../tree/lab1): user-space utilities
Three programs written against xv6's tiny libc and syscall set: `watch` (reruns a command every
N seconds, `-n` to change the interval), a recursive `ls -R`, and `sort`, which inserts tokens into
a doubly linked list as it reads them and works both on a file and on a pipe.

### [`lab2`](../../tree/lab2): page tables and superpages
`vmprint()` walks the three-level Sv39 page table and prints every valid PTE with its virtual
address. The bigger part is superpage support: when `sbrk` grows the heap by 2 MB or more across
an aligned boundary, the kernel maps one 2 MB megapage instead of 512 small pages. For that I added
a separate pool of 2 MB physical chunks (`superalloc`/`superfree`), taught `mappages`, `uvmcopy` and
`uvmunmap` about level-1 leaf PTEs, and wrote `demote()`, which splits a megapage into 4 KB mappings
over the same physical memory when only part of it gets freed.

### [`lab3`](../../tree/lab3): system calls
`getpid()` without entering the kernel: every process gets a read-only page mapped at `USYSCALL`
that holds its PID, created and destroyed together with the trapframe. There's also a new
`interpose(mask, path)` syscall for sandboxing. Syscalls in the bitmask are rejected in the
dispatcher, the restriction is inherited across `fork`, and `open`/`exec` can still be allowed for
one whitelisted path.

### [`lab4`](../../tree/lab4): copy-on-write fork
`fork()` shares the parent's physical pages instead of copying them. Shared PTEs lose their write
bit, and I use the two RSW (reserved for software) bits to mark a page as COW and to remember
whether it was writable in the first place. A write fault copies the page. `copyout()` does the
same for COW pages, since the kernel writes into user memory without taking a fault. Physical
pages are reference counted, so a page goes back to the free list only when its last mapping is
gone.

### [`lab5`](../../tree/lab5): network driver
A driver for the Intel E1000 NIC as emulated by QEMU: transmit and receive over DMA descriptor
rings, driven through the card's memory-mapped registers. On top of it sits the receive half of a
UDP stack: `bind`/`unbind`/`recv` syscalls, per-port queues capped at 16 packets so one busy port
can't eat all memory, blocking `recv` via sleep/wakeup, and byte order conversion of the headers.

### [`lab6`](../../tree/lab6): lock contention
Split the kernel page allocator's single global free list into per-CPU free lists. A CPU whose
list runs dry steals from the others, taking the two locks in a fixed order to avoid deadlock.
I also implemented a reader/writer spinlock on GCC atomics with writer priority: once a writer is
waiting, new readers back off, so a steady stream of readers can't starve it.

### [`lab7`](../../tree/lab7): filesystem
Added a doubly-indirect block to the inode, which raises the maximum file size from 268 blocks to
65,803 without changing the size of the on-disk inode. Also symbolic links: a `symlink` syscall
and inode type, `open()` following chains of links (it gives up after 20 hops, so cycles fail
cleanly), and `O_NOFOLLOW` to open the link itself.

### [`lab8`](../../tree/lab8): mmap
`mmap`/`munmap` for files, with each process keeping its mappings in a small VMA table. Regions
are placed top-down below the trapframe and filled lazily: nothing is read until the first page
fault, when the handler loads that page from the file. Writable `MAP_SHARED` regions are written
back on `munmap` and on exit, a region can be unmapped partially from either end, and `fork`
copies the VMA table to the child.

## Building

You need a riscv64 cross compiler and QEMU:

```bash
sudo apt install gcc-riscv64-linux-gnu qemu-system-riscv      # Debian/Ubuntu
sudo pacman -S riscv64-linux-gnu-gcc qemu-system-riscv        # Arch
```

Then pick a branch and run it:

```bash
git checkout lab7
make qemu
```

`Ctrl-A X` quits QEMU. Branches that come with a grade script can be tested with `make grade`.
Written answers, where there were any, are in `answers-labN.txt`.

## Credits

xv6 is written and maintained by MIT PDOS, see the [upstream repo](https://github.com/mit-pdos/xv6-riscv)
for the full list of contributors. The lab exercises are based on
[MIT 6.1810](https://pdos.csail.mit.edu/6.1810/) materials (CC BY 4.0).
