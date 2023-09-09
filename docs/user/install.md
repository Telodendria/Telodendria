# Installation

There are multiple methods of installing Telodendria. Choose the one
best suited to your use case.

## Package Manager Or System Ports

This is the recommended way to install Telodendria. If your operating
system has an official package or port of Telodendria, you should
prefer to use that instead of the other methods documented here,
because your operating system or software distribution will have
already figured out how to best integrate Telodendria with your system.

Consult your operating system or software distribution's system
manual for instructions on how to install packages. Also consult the
official repository of your distribution to see if a package is
available. If a package exists but it is too out of date for your
tastes, please contact the package's maintainer to notify them, or
offer to update the package yourself using the
[porting instructions](../dev/ports.md).

If you are maintaining a port or package for an operating system or
software distribution, open a pull request to include your
platform-specific instructions as a subheader of this section.
Eventually, this section should contain basic instructions for the
operating systems that have packages or ports.
See [Ports](../dev/ports.md) for the project's distribution
philosophy.

## Container

At this time, Telodendria does not have any officially recommended
procedure for running in a container such as Docker or Vagrant. You
may find helpful files in the [`contrib/`](../../contrib) directory,
however.

If you are publishing container images, please open a pull request to
add your source files to `contrib/`, as well as to add documentation
under this section explaining how to get set started.

## Release Binary

At this time, Telodendria does not publish any official binaries that
can be downloaded. The tentative plan is to eventually provide binaries
with each release for a number of supported platforms. When that
happens, instructions will be provided here for dealing with the
binaries.

## From Source

If you would like to build Telodendria from source, you can download
the latest release code from the
[Releases](/Telodendria/telodendria/releases) page. After extracting
the tarball, read
[Contributing &rightarrow; Developing &rightarrow; Building &amp; Running](../CONTRIBUTING#building-and-running)
for details on how to build Telodendria.

If all goes well, you will find the server binary in the `build/`
directory. If an error occured, and you didn't modify the code,
please open an issue.
