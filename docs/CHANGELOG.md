# Telodendria Change Log

This document contains the complete change log for every official release of Telodendria.
It is intended to be updated with every commit that makes a user-facing change worth
reporting in the change log. As such, it changes frequently between releases. Final
change log entries are published as [Releases](releases).

## v1.7.0-alpha4

**Not Released Yet.**

This release brings filters, rooms, and events! The core of the Matrix
protocol architecture is now in place.

Note that the versioning scheme has changed from `v0.X.0` to
`v1.7.0-alphaX`. This is so that Telodendria releases correspond to the
Matrix specification that they implement, in accordance with
[this blog post](https://telodendria.io/blog/on-matrixs-release-cadence-and-state-resolution-v1).
This versioning scheme change does not indicate a drastic leap forward
in Telodendria's development&mdash;the `-alpha4` suffix indicates that
this is the 4th pre-release, with the target being a stable `v1.7.0`.
Note also that we still have a *long* way to go before we reach that
stable release.

### Matrix Specification

The following endpoints were added:

- **POST** `/_matrix/client/v3/user/{userId}/filter`
- **GET** `/_matrix/client/v3/user/{userId}/filter/{filterId}`

### Bug Fixes & General Improvements

- Use `j2s` for parsing the configuration
- Fixed a double-free in `RouteUserProfile()` that would cause errors
with certain Matrix clients. (#35)
- Improved compatibility with NetBSD on various platforms.
- Moved [Cytoplasm](/Telodendria/Cytoplasm) to its own repository. It
will now be maintained separately and have its own releases as well.
- Use a `configure` script and `make` to build Telodendria instead of
custom scripts.
- Greatly simplified some endpoint code by using Cytoplasm's `j2s` for
parsing request bodies.

### New Features

- Implemented a `"pid"` option in the configuration, allowing Telodendria 
to write its process ID to a specified file.
- Moved all administrator API endpoints to `/_telodendria/admin/v1`,
because later revisions of the administrator API may break clients, so
we want a way to give those breaking revisions new endpoints.
- Implemented `/_telodendria/admin/v1/deactivate/[localpart]` for admins
to be able to deactivate users.
- Added a **PUT** option to `/_telodendria/admin/v1/config` that gives
the ability to change only a subset of the configuration.
- Implemented the following APIs for managing registration tokens:
    - **GET** `/_telodendria/admin/tokens`
    - **GET** `/_telodendria/admin/tokens/[token]`
    - **POST** `/_telodendria/admin/tokens`
    - **DELETE** `/_telodendria/admin/tokens/[token]`

## v0.3.0

**Saturday, June 10, 2023**

Introducing a new configuration API and Cytoplasm, a general-purpose C library that
supports source/sink-agnostic I/O, TLS, an HTTP client, and more! The third major
release of Telodendria packs a lot of architectural improvements on top of supporting
more of the Matrix specification.

### Matrix Specification

Added support for the following endpoints:

- `/_matrix/client/v3/account/whoami`
- `/_matrix/client/v3/account/password`
- `/_matrix/client/v3/account/deactivate`
- `/_matrix/client/v3/profile/*`
- `/_matrix/client/v3/capabilities`
- `/_matrix/client/v3/auth/*/fallback/web`

There is also support for token-based user registration. Note that there is as of
yet no admin-facing way to create these registration tokens, but the APIs are in
place.

### New Features

- Added a new `HttpClient` API for making HTTP requests. This will eventually be
used for federating with other Matrix homeservers.
- Added support for pretty-printing JSON in `Json`. Telodendria itself does not
pretty-print JSON, but this is useful for debugging and building useful tools.
- Added a handful of new development tools built on the Telodendria APIs. New
tools include `http`, a command line tool for making HTTP requests, similar to
`curl`, `json`, a command line tool for working with JSON, similar to `jq`, and
`http-debug-server`, a simple HTTP server that just prints requests out to standard
output and returns an empty JSON object. `http` and `json` are replacements for
`curl` and `jq` that build on the `HttpClient` and `Json` APIs. They exist mainly
to test those APIs, but also to reduce the number of dependencies that Telodendria
has. `http-debug-server` exists to test the `HttpServer` and `HttpClient` APIs.
- Replaced all usage of `jq` with the new `json` tool. `jq` is no longer a development
dependency.
- Replaced all usage of `curl` with the new `http` tool. `curl` is no longer a
required development dependency.
- Added a new `tt` script for easily making Matrix requests against Telodendria
in development.
- Added TLS support to both the HTTP client and server. Currently, Telodendria
supports LibreSSL and OpenSSL, but other TLS libraries should be extremely easy
to add support for.
- Added support for spinning up multiple HTTP servers. This is useful for having
a TLS port and a non-TLS port, for example.
- Moved all program configuration to the data directory and added an administrator
API endpoint to manage it. It is now no longer recommended to manually update the
configuration file. Consult the [Administrator API](user/admin/README.md) documentation
and the [Configuration](user/config.md) documentation.
- Added an administrator API endpoint for process control. Telodendria can now be
restarted or shutdown via API endpoint.
- Added an administrator API endpoint for getting statistics about the running
Telodendria process.
- Added support for user privileges, a way to have fine-grained control over what
users are allowed to do with the administrator API. Administrator APIs for setting
and getting privileges is now supported, and registration tokens have privileges
associated with them so that users created with a token will automatically be given
the specified privileges.

### Fixes & General Improvements

- Fixed a few warnings that were generated on some obscure compilers.
- Moved the `main()` function to its own file to make it easier to link other
programs with the Telodendria APIs.
- Fixed the development tools environment script. Apparently using a hyphen as a bullet
point is not very portable, because some shell implementations of `printf` interpret it
as a flag. Switched to an asterisk.
- Fixed some intermittent I/O errors that would occur as a result of race conditions in
`JsonConsomeWhitespace()`. This function, and a few others, expect I/O to be blocking,
but the `HttpServer` sets up I/O to be non-blocking, leading to occasional failures in
JSON parsing.
- Abstracted all I/O into the new `Io` and `Stream` APIs, which provide an input- and
output- agnostic stream processing interface. This allows for a simple implementation of
proxies, TLS, and other stream filters without having to change any of the existing
code.
- Remove all non-POSIX function calls, including the call to `chroot()` and, on
OpenBSD, `pledge()` and `unveil()`. This may seem like a downgrade in security, but
these are platform-specific system calls that should be patched in by package maintainers
if they are desired. They also caused problems when implementing other features, because
some library calls need to be able to access files on the filesystem.
- Fixed the build script to supply `LDFLAGS` after the object files when linking.
Apparently the order in which libraries are passed matters to some compilers.
-  Added the response status of a request to the log output. This means that requests are
logged after they have completed, not before they are started.
- Memory allocations, reallocations, and frees are no longer loged when the log level
is set to debug in the configuration file. To enable the logging of memory operations,
pass the `-v` flag.
- Implemented a proper HTTP request router with POSIX regular expression support.
Previously, a series of nested `if`-statements were used to route requests, but this
approach quickly becamse very messy. While the HTTP request router incurs a small memory
and runtime speed penalty, the code is now much more maintainable and easier to follow.
- Fixed some memory bugs in `Db` that were related to caching data. Caching should
now work as expected.
- Fixed a major design flaw in `Db` that would cause deadlock when multiple threads
request access to the same object. Database locking is now in a per-thread basis,
instead of a per-reference basis.
- Telodendria now shuts down cleanly in response to `SIGTERM`.
- Did some general refactoring to make the source code more readable and easier
to maintain.
Fixed a number of memory-related issues, including switching out some unsafe
functions for safer versions, per the recommendations of the OpenBSD linker.
- Moved all code documentation into the C header files to make it more likely
that it will get updated. A simple header file parser and documentation generator
have been added to the code base. See the `hdoc` man pages for documentation.
- Updated the build script to provide static and shared libraries containing
the code for Telodendria to make it easier to statically and dynamically link to
other programs. The idea is that these libraries should be shipped with Telodendria,
or as a separate package, and can be used to provide a high-level programming
environment.
- Updated the `Json` API to calculate the length of a JSON object. This is
used to set the `Content-Length` header in HTTP requests and reponses.
- Added some string functions, including `StrEquals()`, which replaced almost all
uses of `strcmp()`, since `strcmp()` is used almost exclusively for equality
checking. `StrEquals()` provides a standard way to do so, because previously,
multiple different conventions could be found throughout the code base (for example:
`!strcmp(str1, str2)` vs `strcmp(str1, str2) == 0`).

... And many more!

## v0.2.1

**Monday, March 6, 2023**

This is a patch release that fixes a few typos and other minor issues.

## v0.2.0

**Monday, March 6, 2023**

This release is focused on providing a decent amount of the client authentication
API. You can now create accounts on a Telodendria homeserver, and log in to
get access tokens.

### New

- Added the basic form of the user registration API. If registration is enabled
in the configuration file, clients can now register for Matrix accounts.
- Added the basic form of the user login API. Clients can now log in to
their accounts and generate access tokens to be used to authenticate requests.
- Added the basic form of the user interactive authentication API, which can be used
by endpoints that the spec says requires it. Currently, it only implements the
dummy and password stages, but more stages, such as the registration token stage,
will be added in future releases.
- Added a simple landing page that allows those setting up Telodendria to
quickly verify that it is accessible where it needs to be.
- Added the static login page for clients that don't support regular login.

### Changes

- Improved HTTP request logging by removing unnecessary log entries and making
errors more specific.
- Leaked memory is now hexdump-ed out to the log if the log level is set to debug.
This greatly simplifies debugging, because developers can now see exactly what the
contents of the leaked memory are. Note that in some circumstances, this memory
may contain sensitive data, such as access tokens, usernames, or passwords. However,
Telodendria should not be leaking memory at all, so if you encounter any leaks,
please report them.
- Refactored a lot of the code and accompanying documentation to be more readable and
maintainable.

### Bug Fixes

- Fixed a memory leak that would occur when parsing an invalid JSON object.
- Fixed an edge case where HTTP response headers were being sent before they were
properly set, causing the server to report a status of 200 even when that wasn't the
desired status.
- Fixed a few memory leaks in the HTTP parameter decoder that would occur in some
edge cases.
- Fixed an "off-by-one" error in the HTTP server request parser that would prevent
`GET` parameters from being parsed.
- Fixed the database file name descriptor to prevent directory traversal attacks
by replacing special characters with safer ones.
- Fixed a memory leak that would occur when closing a database that contains
cached objects.
- Fixed a memory leak that would occur when deleting database objects.
- Fixed a few non-fatal memory warnings that would show up as a result of passing a
constant string into certain functions.

 ### Misc.

 - Fixed a bug in `td` that caused `cvs` to be invoked in the wrong directory when
 tagging a new release.
 - Added support for environment variable substitution in all site files. This
 makes it easier to release Telodendria versions.
 - Fix whitespace issues in various shell scripts.
 - Fixed the debug log output so that it only shows the file name, not the
 entire file path in the repository.
 - Updated the copyright year in the source code and compiled output.
 - Switched the `-std=c89` flag to `-ansi`, as `-ansi` might be more supported.
 - Fixed the `-v` flag. It now sets the log level to debug as soon as possible to
 allow debugging configuration file parsing if necessary.

 ... And many more bug fixes and feature additions! Too much has changed to make a
 comprehensive change log. A lot of things have been done under the hood to make
 Telodendria easier to develop in the future. Please test the current functionality,
 and report bugs.

The following platforms have been known to compile and run Telodendria:

- OpenBSD
- Linux (GNU and non-GNU)
- Windows (via Cygwin)
- FreeBSD
- NetBSD
- DragonFlyBSD
- Haiku OS
- Android (via Termux)

Telodendria is about being portable; if you compile it on an obscure operating system,
do let us know about it!

## v0.1.0

**Tuesday, December 13, 2022**

This is the first public release of Telodendria so there are no changes to report.
Future releases will have a complete change log entry here.

This is a symbolic release targeted at developers, so there's nothing useful to
ordinary users yet. Stay tuned for future releases though!
