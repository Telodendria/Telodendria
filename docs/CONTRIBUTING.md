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
[New Issue](/Telodendria/telodendria/issues/new/choose) and follow the
instructions.

> **Note:** GitHub issues are not accepted. Issues may only be
> submitted to the official [Forgejo](https://git.telodendria.org)
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
- [Cytoplasm](/Telodendria/Cytoplasm), a simple C library written by
the Telodendria developers for the purpose of supporting Telodendria
in a modular way.

Optionally, you may also find these tools helpful:

- `indent` for formatting code.
- `valgrind` for debugging particularly nasty issues.

### Getting The Code

Telodendria is developed using Git. The easiest way to contribute
changes is to fork the main repository, and then creating a pull
request to ask us to pull your changes into our repo.

1. If you don't have an account on the
[Forgejo instance](https://git.telodendria.org), create one and sign in.
1. Fork this repository.
1. In your development environment, clone your fork:
   ```shell
   git clone https://git.telodendria.org/[YOUR_USERNAME]/Telodendria.git
   cd Telodendria
   ```

   Please base your changes on the `master` branch. If you need help
   getting started with Git, that is beyond the scope of this
   document, but you can find many good tutorials on the web.

### Building &amp; Running

Telodendria uses the `make` build system. Because it aims at maximum
portability, it targets POSIX `make` and should thus run on any POSIX
system that provides a `make`, be it GNU, BSD, or something different
entirely. To facilitate this, Telodendria provides a `configure` script
which generates the `Makefile`, because the `Makefile` would be far too
verbose and tedious to maintain in a POSIX-compatible way otherwise.
This is similar to how other C programs and libraries are built, although
note that Telodendria's `configure` script is not nearly as advanced as
an `autoconf` script, for example.

Please follow the build and installation directions for
[Cytoplasm](/Telodendria/Cytoplasm) first before attempting to build
Telodendria, because Telodendria depends on Cytoplasm and assumes it is
installed in the standard location for your system. For the best results,
it is recommended to take the time to enable TLS, unless you plan on
running Telodendria behind a reverse proxy.

To build Telodendria, simply run `configure`, then `make`:

```
$ ./configure
$ make
```

You may find some of the following options for `configure` helpful:

- `--prefix=<path>`: Set the install prefix to set by default in the `Makefile`. This defaults to `/usr/local`, which should be appropriate for most Unix-like systems.
- `--(enable|disable)-ld-extra`: Control whether or not to enable additional linking flags that create a more optimized binary. For large compilers such as GCC and Clang, these flags should be enabled. However, if you are using a small or more obscure compiler, then these flags may not be supported, so you can disable them with this option.
- `--(enable|disable)-debug`: Control whether or not to enable debug mode. This sets the optimization level to 0 and builds with debug symbols. Useful for running with a debugger.
- `--static` and `--no-static`: Controls whether static binaries are built by default. On BSD systems, `--static` is perfectly acceptable, but on GNU systems, `--no-static` is often desirable to silence warnings about static binaries emitted by the GNU linker.

Telodendria can be customized with the following options:

- `--bin-name=<name>`: The output name of the server binary. This defaults to `telodendria`. Common alternatives are `matrix-telodendria` or `telodendria-server`.
- `--version=<version>`: The version string to embed in the binary. This can be used to indicate build customizations or non-release versions of Telodendria.

The following recipes are available in the generated `Makefile`:

- `all`: This is the default target. It builds everything.
- `telodendria`: Build the `telodendria` binary. If you specified an alternative `--bin-name`, then this target will be named after that.
- `docs`: Generate the header documentation as `man` pages.
- `tools`: Build the supplemental tools which may be useful for development.
- `clean`: Remove the build and output directories. Telodendria builds are out-of-tree, which greatly simplifies this recipe compared to in-tree builds.

If you're developing Telodendria, these recipes may also be helpful:

- `format`: Format the source code using `indent`. This may require a BSD `indent` because last time I tried GNU `indent`, it didn't like the flags in `indent.pro`. Your mileage may vary.
- `license`: Update the license headers in all source code files with the contents of the `LICENSE.txt`.

To install Telodendria to your system, the following recipes are available:

- `install`: This installs Telodendria under the prefix set with `./configure --prefix=<dir>` or with `make PREFIX=<dir>`. By default, the `make` `PREFIX` is set to whatever was set with `configure --prefix`.
- `uninstall`: Uninstall Telodendria from the same prefix as specified above.

After a build, you can find the object files in `build/` and the output binary in `out/bin/`.

### Pull Requests

> **Note:** Telodendria does not accept GitHub pull requests at this
> time. Please submit your pull requests via Gitea.

Telodendria follows the standard pull request procedures. Once you have
made your changes, committed them, and pushed to your fork, you should
be able to open a pull request on the main repository. When you do, you
will be prompted to write a description. Be sure to include the
related issue that you are closing in your description.

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

### Be Recognized!

If your pull request gets approved, you should be recognized for your
contributions to the project!

To have your work recognized, add your information to the `CONTRIBUTORS.txt`
file in the root of the Telodendria repository if it isn't there already.
You should do this as a part of your pull request so that when it is merged,
your information will be automatically added to the repository.

The `CONTRIBUTORS.txt` file loosely follows the Linux kernel's
[CREDITS](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/CREDITS)
file format. It is designed to be human-readable, but also parsable by
scripts.

The following fields are available:

```
  (N) Name
  (E) Email
  (M) Matrix ID
  (W) Website
  (D) Description of contribution
  (L) Physical location
```

Here are the rules:

* All fields are optional. If you don't want to include a field, that's
  okay, simply omit it.
* All fields identify you however you wish. The goal is to recognize you for
  your contribution, but if you wish to remain anonymous, you don't have to
  use your real information.
* All fields can be specified multiple times. For example, if you have
  multiple email addresses, websites, or Matrix IDs and you want to include
  all of them, you absolutely may. Likewise, if you have made multiple
  contributions, you can add multiple description entries.
* You can make up your own fields if you want. Just add their description
  above.
* Leave exactly one blank like between entries in this file.

