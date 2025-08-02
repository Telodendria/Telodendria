# System Requirements

Telodendria is written in C, so it has minimal system requirements. This
document describes what you need to *run* Telodendria. To *build* and *develop*
Telodendria, consult [Contributing &rightarrow; What do I need?](../CONTRIBUTING.md#what-do-i-need).

To run Telodendria, all you need is:

- A Unix-like operating system that provides standard POSIX behavior, or the
  Windows Subsystem for Linux (WSL), Cygwin, or Msys2 if you are running
  Windows.
- A data directory that Telodendria can read from and write to.

It is important to note that you do not need a database, because Telodendria
implements its own database in the data directory. Furthermore, you do not need
root privileges, as Telodendria can run as any user, and on configurable port
numbers (it defaults to high port numbers which don't need root access).

## System Specifications

Though Telodendria aims at being efficient, it is still a Matrix homeserver, and
that means that it is computationally intensive and requires a bit of memory. At
least 1GB of RAM is recommended to comfortably use Telodendria. With more RAM,
you can enable caching to keep database objects in memory. Telodendria does not
have CPU or network throughput requirements, though the general rule of thumb is
that more will always be better.

Telodendria does *not* target embedded systems. Thought it should in theory
run in fairly constrained environments such as a cheap VPS, it is not the goal
of Telodendria to have the smallest possible memory or compute footprint.
Telodendria does use a fair bit of memory, and there's little that can be done
to prevent this due to the Matrix specification itself being fairly large and
complex.
