Cytoplasm (libcytoplasm)
========================================================================

Cytoplasm is a general-purpose C library and runtime stub for
creating high-level (particularly networked and multi-threaded) C
applications. It allows applications to take advantage of the speed,
flexibility, and simplicity of the C programming language, while
providing helpful code to perform various complex tasks. Cytoplasm
provides high-level data structures, a basic logging facility, an
HTTP client and server, and more.

Cytoplasm aims not to only do one thing well, but to do many things
good enough. The primary target of Cytoplasm is simple, yet higher
level C applications that have to perform relatively complex tasks,
but don't want to pull in a large number of dependencies.

Cytoplasm is extremely opinionated on the way programs using it are
written. It strives to create a comprehensive and tightly-integrated
programming environment, while also maintaining C programming
correctness. It doesn't do any macro magic or make C look like
anything other than C. It is written entirely in C89, and depends
only on a POSIX environment. This differentiates it from other
general-purpose libraries that often require modern compilers and
non-standard language and environment features. Cytoplasm is intended
to be extremely portable and simple, while still providing some of
the functionality expected in higher-level programming languages
in a platform-agnostic manner. In the case of TLS, Cytoplasm wraps
low-level TLS libraries to offer a single, unified interface to TLS
so that programs do not have to care about the underlying
implementation.

Cytoplasm is probably not suitable for embedded programming. It makes
liberal use of the heap, and while data structures are designed to
conserve memory where possible and practical, minimal memory usage
is not really a design goal for Cytoplasm, although Cytoplasm takes
care not to use any more memory than it absolutely needs. Cytoplasm
also wraps a few standard libraries with additional logic and
checking. While this ensures better runtime safetly, this inevitably
adds a little overhead. 

Originally a part of Telodendria (https://telodendria.io), a Matrix
homeserver written in C, Cytoplasm was split off into its own project
due to the desire of some Telodendria developers to use Telodendria's
code in other projects. Cytoplasm is still a Telodendria project,
and is maintained along side of Telodendria itself, even living in
the same CVS module, but it is designed specifically to be distributed
and used totally independent of Telodendria.

The name "Cytoplasm" was chosen for a few reasons. It plays off the
precedent set up by the Matrix organization in naming projects after
the parts of a neuron. It also speaks to the function of Cytoplasm.
The cytoplasm of a cell is the supporting material. It is what gives
the cell its shape, and it facilitates the movement of materials
to the other cell parts. Likewise, Cytoplasm aims to provide a
support mechanism for C applications that have to perform complex
tasks.

Cytoplasm also starts with a C, which I think is a nice touch for C
libraries. It's also fun to say and unique enough that searching for
"libcytoplasm" should bring you to this project and not some other
one.

Building
------------------------------------------------------------------------

If your operating system or software distribution provides a pre-built
package of Cytoplasm, you should prefer to use that instead of
building it from source.

Cytoplasm aims to have zero dependencies beyond what is mandated
by POSIX. You only need the standard math and pthread libraries to
build it. TLS support can optionally be enabled by setting the
TLS_IMPL environment variable. The supported TLS implementations
are as follows:

    * OpenSSL
    * LibreSSL

If TLS support is not enabled, all APIs that use it should fall
back to non-TLS behavior in a sensible manner. For example, if TLS
support is not enabled, then the HTTP client API will simply return
an error if a TLS connection is requested.

Cytoplasm uses a custom build script instead of Make, for the sake
of portability. To build everything, just run the script:

        $ sh make.sh

This will produce the following out/ directory:

    out/
        lib/
            libcytoplasm.so - The Cytoplasm shared library.
            libcytoplasm.a - The Cytoplasm static library.
            cytoplasm.o - The Cytoplasm runtime stub.
        bin/
            hdoc - The documentation generator tool.
        man/ - All Cytoplasm API documentation.

Usage
------------------------------------------------------------------------

Cytoplasm provides the typical .so and .a files, which can be used
to link programs with it in the usual way. However, Cytoplasm also
provides a minimal runtime environment that is intended to be used
with the library. As such, it also provides a runtime stub, which
is intended to be linked in with programs using Cytoplasm. This
stub is responsible for setting up and tearing down some Cytoplasm
APIs. While it isn't required by any means, it makes Cytoplasm a
lot easier to use for programmers by abstracting out all of the
common logic that most programs will want to use.

Here is the canonical Hello World written with Cytoplasm:

	#include <Log.h>

	int Main(void)
	{
		Log(LOG_INFO, "Hello World!");
		return 0;
	}

If this file is Hello.c, then you can compile it by doing something
like this:

	$ cc -o hello Hello.c cytoplasm.o -lcytoplasm

This command assumes that the runtime stub resides in the current
working directory, and that libcytoplasm.so is in your library path.
If you're using the version of Cytoplasm installed by your operating
system or software distribution, consult the documentation for the
location of the runtime stub. It may be located in
/usr/local/libexec or some other similar location. If you've built
Cytoplasm from source and wish to link to it from there, you may wish
to do something like this:

	$ export CYTOPLASM=/path/to/Cytoplasm/out/lib
	$ cc -o hello Hello.c "${CYTOPLASM}/cytoplasm.o" \
		"-L${CYTOPLASM}" -lcytoplasm

As you may have noticed, C programs using Cytoplasm's runtime stub
don't write the main() function. Instead, they use Main(). The main()
function is provided by the runtime stub. The full form of Main()
expected by the stub is as follows:

	int Main(Array *args, HashMap *env);

The first argument is a Cytoplasm array of the command line
arguments, and the second is a Cytoplasm hash map of environment
variables. Most linkers will let programs omit the env argument,
or both arguments if you don't need either. The return value of
Main() is returned to the operating system.

Note that both arguments to Main may be treated like any other
array or hash map. However, do not invoke ArrayFree() or HashMapFree()
on the passed pointers, because memory is cleaned up after Main()
returns.

