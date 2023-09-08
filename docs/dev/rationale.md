# Rationale

This document seeks to answer the question of "why Telodendria?" from
a technical perspective by comparing it to existing Matrix homservers.
Telodendria is written entirely from scratch in ANSI C. It is designed
to be lightweight, simple, and functional. Telodendria differentiates
itself from other homeserver implementations because it:

- Is written C, a stable, low-level programming language with a long
history, low build and runtime overhead, and wide compatibility.
- Is written with minimalism as a primary design goal. Whenever possible
and practical, no third-party libraries are pulled into the code.
Everything Telodnedria needs is custom written. As a result, Telodendria
depends only on a standard C compiler and a POSIX C library to be
built, both of which should come with any good Unix-style operating
system already, which means you shouldn't have to install anything
additional to use Telodendria.
- Uses a flat-file directory structure to store data instead of a
real database. This has a number of advantages:
    - It make setup and mainenance much easier.
    - It allows Telodendria to run on systems with fewer resources.
- Is packaged as a single small, statically-linked and highly-optimized
binary that can be run just about anywhere. It is designed to be
extremely easy to set up and consume as few resources as possible.
- Is permissively licensed. Telodendria is licensed under a modified
MIT license, which imposes very few restrictions on what you can do
with it.

## What about [Conduit](https://conduit.rs)?

At this point, you may be wondering why one would prefer Telodendria
over Conduit, a Matrix homeserver that could also say pretty much
everything this document has said so far. After all, Conduit is older
and thus better established, and written in Rust, a Memory Safe&trade;
programming language.

In this section, we will discuss some additional advantages of
Telodendria that Conduit lacks.

### Small Dependency Chain

**TODO:** See #30.

### Standardized

**TODO:** See #30.

### Portable

**TODO:** See #30.
