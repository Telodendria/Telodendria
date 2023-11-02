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

Conduit's dependency chain is quite large. What this means is that
Conduit depends on a lot of code that it does not control, making it
vulnerable to supply chain attacks. A problem with Rust Crates
is that they are developer-published, so they don't go through any sort
of auditing process like a Debian package would, for example.
 If any one of the dependencies is
hijacked or otherwise compromised, then Conduit itself is compromised
and it is likely that this would go unnoticed for quite a while. While
one could argue that this is extremely unlikely to happen, sometimes you
just don't want to take that risk, especially not if you're deploying a
Matrix homeserver, likely for the purpose of secure, private chat.

Telodendria doesn't pull in any packages from developer repositories, so
the risk of supply chain attacks is much lower. It
only uses its own code and code provided by the operating system it is running
on, which has been vetted by a large number of developers and can be trusted
due to the sheer scope of an operating system. A supply chain attack against
Telodendria would be a supply chain attack against the entire operating system;
at that point, end users have much bigger problems.

Minimal dependencies doesn't only mitigate supply chain attacks. It also makes
maintenance much easier. Telodendria can spend more time writing code than
Conduit because Conduit developers have to ensure dependencies stay up to date and
when they inevitably break things, Conduit must pause development to fix those.
Telodendria doesn't suffer from this problem: because most of the code is developed
along side of Telodendria, it can remain as stable or become as volatile as the
developers choose. Additionally, because Telodendria is so low-level, the code on
which it depends is extremely unlikely to be changed in any significant way,
since so many other programs depend on that code.

### Standardized

Conduit is written in Rust, which has no formal standard. This makes it less than
ideal for long-lived software projects, because it changes frequently and often
breaks existing code. Telodendria is written in C, a stable, mature, and standardized
language that will always compile the same code the same way, making it more
portable and sustainable for the future because we don't ever have to worry about
upgrading our toolchain&mdash;using standard tools built into most operating systems
will suffice.

Because the language in which Telodendria is written never changes, Telodendria can
continually optimize and improve the code, instead of having to fix breaking changes.
This ensures that Telodendria's code will last. Rust code becomes obsolete with in a
few years at best&mdash;programs written in Rust last year probably won't compile or run
properly on the latest Rust toolchain. Telodendria, on the other hand, is written in C89,
which compiled and ran the same way in 1989 as it does today and will continue to for the
foreseeable future.

### Fast Compile Times

Rust is well-known for taking an extremely long time to compile moderately-sized
programs. Since a Matrix homeserver is such a large project, the compile times would
be prohibitively large for rapid development. By writing Telodendria in C, we can take
advantage of decades worth of compiler optimizations and speed improvements, resulting
in extremely fast builds.

### Portable

One does not typically think of C as more portable than something like Rust, but
Telodendria is written in such a way that it is. Rust relies on LLVM, which doesn't
support some strange or exotic architectures in the same way that a specialized C
compiler for those architectures will. This allows users to run Telodendria on the
hardware of their choice, even if that hardware is so strange that the modern world
has totally left it behind.

Telodendria doesn't just aim at being lightweight and portable, it aims to empower
people to use common hardware that they already have, even if it is typically thought
of as underpowered.
