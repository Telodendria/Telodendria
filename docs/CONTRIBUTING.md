# Contributing

Telodendria is a fully open source project. As such, it welcomes
contributions. There are many ways you can contribute, and any way you
can is greatly appreciated. This document details the ways you can
contribute, and how to go about contributing.

## Sponsoring Telodendria

If you would like to sponsor Telodendria, see the
[Sponsorship](../README.md#sponsorship) section on the main project
page. Donations of any size are greatly appreciated.

## Reporting Issues

An important way to get involved is to just report issues you find with
Telodendria during experimentation or normal use. To report an issue,
go to [Issues](/Telodendria/telodendria/issues) &rightarrow;
[New Issue](/Telodendria/telodendria/issues/new) and follow the
instructions.

> **Note:** GitHub issues are not accepted. Issues may only be
> submitted to the official [Gitea](https://git.telodendria.io)
> instance.

### Feature Requests

Feature requests are allowed, but note that they are low-priority in
comparison to existing issues and features. That being said, don't
hesitate to submit feature requests. Just select the "Feature Request"
option when submitting an issue.

## Developing

If you want to write code for Telodendria, either to fix an issue or
add a new feature, you're in the right place. Please follow all the
guidelines in this document to ensure the contribution workflow goes
as smoothly as possible.

### Who can develop Telodendria?

Everyone is welcome to contribute code to Telodendria, provided that
they are willing to license their contributions under the same license
as the project itself.

The primary language used to write Telodendria code is ANSI C. Other
languages you'll find in the Telodendria repository include shell
scripts, `mdoc`, a little bit of HTML and CSS, and `Makefiles`.
Experience with any of these is preferred, but if you want to use
Telodendria to learn, that's okay too! Telodendria's code base should
hopefully be a good learning tool, and if you are serious about
submitting quality work, we'll guide you through the process and
offer suggestions.

### What do I need?

You'll need a couple of things to develop Telodendria:

- A Unix-like operating system that provides standard POSIX behavior,
or the Windows Subsystem for Linux (WSL), Cygwin, or Msys2 if you are
running Windows.
- A C compiler capable of compiling ANSI C89 code (pretty much all of
them do&mdash;pick your favorite, and if you find it doesn't work,
open an issue!).
- `make` for building the project.
- `git` for managing your changes.

Optionally, you may also find these tools helpful:

- `indent` for formatting code.
- `valgrind` for debugging particularly nasty issues.

### Getting The Code

Telodendria is developed using Git. The easiest way to contribute
changes is to fork the main repository, and then creating a pull
request to ask us to pull your changes into our repo.

1. If you don't have an account on the
[Gitea instance](https://git.telodendria.io), create one and sign in.
1. Fork this repository.
1. In your development environment, clone your fork:
   ```shell
   git clone https://git.telodendria.io/[YOUR_USERNAME]/telodendria.git
   cd telodendria
   ```

   Please base your changes on the `master` branch. If you need help
   getting started with Git, that is beyond the scope of this
   document, but you can find many good tutorials on the web.

### Building &amp; Running

Telodendria uses the `make` build system.

**TODO:** Update this section before #19 is closed. Provide quick
make, run, and install directions (maybe just redirect to Porting for
install directions), then list all the `make` recipes. Shouldn't be
as many as were in `td`.

### Pull Requests

> **Note:** Telodendria does not accept GitHub pull requests at this
> time. Please submit your pull requests via Gitea.

Telodendria follows the standard pull request procedures. Once you have
made your changes, committed them, and pushed to your fork, you should
be able to open a pull request on the main repository. When you do, you
will be prompted to write a description

### Code Style

In general, these are the conventions used by the code base. This
guide may be slightly outdated or subject to change, but it should be
a good start. The source code itself is always the absolute source of
truth, so as long as you make your code look like the code surrounding
it, you should be fine.

- All function, enumeration, structure, and header names are
`CamelCase`. This is preferred to `snake_case` because it is more
compact.
- All variable names are `lowerCamelCase`. This is preferred to
`snake_case` because it is more compact. One exception to this rule is
if a variable name, such as a member of a struct, directly represents
a JSON key in an object specified by the Matrix specification, which
may be in `snake_case`.
- Enumerations and structures are always `typedef`-ed to their same
name. The `typedef` should occur in the public API header, and the
actual declaration should live in the implementation file, unless
the enumeration or structure is intended to be made fully public.
- A feature of the code base lives in a single C source file that has a
matching header. The header file should only export public symbols;
everything else in the C source should be static.
- Except where absolutely necessary, global variables are forbidden
to prevent problems with threads and whatnot. Every variable a
function needs should be passed to it either through a structure, or
as a separate argument.
- Anywhere that C allows curly braces to be optional, there still must
be curly braces. This makes it easier to read the code by making it
less ambiguous, and it makes it easier to add on to the code later.

As far as actually formatting the code goes, such as where to put
brackets, and whether or not to use tabs or spaces, use `indent` to
take care of that. The repository contains a `.indent.pro` that should
automatically be loaded by `indent` to set the correct rules. If you
don't have a working `indent`, then just indicate in your pull
request that I should run my `indent` on the code.

### Documentation

This project places a strong emphasis on documentation. Well-documented
code is fundamental to a successful project, so when you are writing
code, please also make sure that it is documented appropriately.

- If you are adding a header, make sure you add the necessary comments
detailing the header and the functions in it.
- If you are adding a function, make sure you add the necessary
comments to the appropriate header.

If your pull request does not also include proper documentation, it
will likely be rejected.
