# Usage

This document provides general documentation on how to use the
`telodendria` server binary, as well as details on how it behaves.
The details here will be useful for setting up init systems, running
Telodendria in a container, or manually executing the binary for
testing or debugging purposes.

## Command Line Options

Typically, Telodendria is controlled via the
[Administrator API](admin.md), but the Telodendria binary does include
a few command line options, which can be used in init scripts or for
debugging purposes.

The command line arguments are as follows:

- **`-d <dir>`** Specify the data directory to use. All persistent
storage that Telodendria requires is saved to and loaded from here.
- **`-V`** Only print the version information header and then quit
with a success exit code.
- **`-v`** Verbose mode. This overrides the configuration and sets the
log level to `debug`. It also enables additional logging of memory
operations, which can be useful for debugging.

Before proposing additional command line arguments, consider whether or
not the functionality requested can be provided via a (potentially new
and as of yet uncreated) administrator API endpoint.

## Environment

Telodendria does not read any environment variables. All configuration
should be done via the [Configuration API](config.md).

## Signals

Telodendria recognizes and responds to a number of signals:

- **`PIPE`:** This signal is ignored, because all I/O errors should
already be handled properly.
- **`USR1`:** Perform a soft restart by shutting down the HTTP servers
and resetting the program state. Note that the daemon process does
not exit.
- **`TERM`:** Perform a clean shutdown after all existing connections
are closed.
- **`INT`:** Same as `TERM`.

Any other signals are not explicitly handled, so they have the
default behavior as defind by the operating system.

## Exit Status

Telodendria exits with a non-0 exit code if the configuration file is
invalid, or one or more of required paths or files is inaccessible.
Telodendria will print an error to the log and then terminate
abnormally.

Telodendria exits with a code of 0 if the configuration file is valid,
all paths and files required are accessible, and the HTTP listener
starts as intended. If Telodendria is sent a signal that it catches
after it begins servicing requests, it will still exit normally after
it safely shuts down, because the bootstrap process completed
successfully, and by all accounts, it ran normally and exitted
normally.
