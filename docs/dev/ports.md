## Ports

Telodendria is distributed primarily as source code, and the project
itself does not offer a convenient install process such as in the form
of a shell script. This is intentional; the Telodendria project is
primarily concerned with developing Telodendria itself, not packaging
it for the hundreds of different operating systems and linux
distributions that exist. It is my firm belief that distributing an
open source project is not the job of the open source developer; that
is the reason software distributions exist: to collect and
*distribute* software.

It would be impossible to single-handedly package Telodendria for
every platform, because each platform has very different expectations
and conventions for software. Even different Linux distributions have
different conventions for where manual pages, binaries, and
configuration files go.

That being said, this document aims to assist those who want to
package Telodendria for their operating system or software
distribution.

---

Before attempting to package Telodendria, make sure that you can build
it and that it builds cleanly on your target platform. See
[Install &rightarrow; From Source](../user/install.md#from-source)
for general build instructions.

To package Telodendria, you should collect the following files, and
figure out where they should be installed on your system:

- The `telodendria` server binary itself.
- An init script. People that wish to install Telodendria on their
system using your package are going to expect it to be integrated
enough that Telodendria can easily be started at boot and otherwise
managed by the system's daemon tools, be it `systemd` or another
init system. Consult your system's documentation for writing an init
script. **Note:** Telodendria *does not* fork itself to the background;
the init script should do that.
- You may also wish to ship the `docs/` directory
so that the user can read the documentation offline, and ensure that
they are reading the correct documentation for the installed version.

You may wish to optionally create a dedicated user under which
Telodendria should run. Telodendria can be directly started as that
user, or start as root and be configured to automatically drop to that
user. Additionally, it might be helpful to provide a default
configuration, which can be placed in the samples directory on your
platform, or in a default location that Telodendria will load from.
A good default directory that you may wish to provide for configuration,
data, and logs could perhaps be `/var/telodendria` or `/var/db/telodendria` on Unix-like systems.

Once you have collected the necessary files and directories that need
to be installed, make sure your package performs the following tasks
on install:

- If necessary and depending on the configuration used, create a new
system user for the Telodnedria daemon to run as.
- If conventional for your system, enable the Telodendria init script
so that Telodendria is started on system boot.
- Instruct the user to carefully read the [Setup](../user/setup.md)
(`docs/user/setup.md`) instructions and the
[Configuration](../user/config.md) (`docs/user/config.md`) instructions
before starting Telodendria.

The goal of a package should be to get everything as ready-to-run as
possible. The user should be able to start Telodendria right away and
begin configuring it.

Remember to publicly document the setup of Telodendria on your platform
if there are additional steps required that are not mentioned in the
official Telodendria documentation. This ensures that users can get
up and running quickly and easily. If you're packaging Telodendria
for a container system such as Docker, you can omit the things that
containers typically do not have, such as the init scripts and
documentation.

Also remember that your port should feel like it belongs on your target
system. Follow all of your system's conventions when placing files
on the filesystem, so your users know what to expect. The goal is not
necessarily to have a unified experience across all operating systems,
rather, you should cater to the opinions of your operating system.
Telodendria is architected in such a way that it does not impose the
developer's opinions of where things should go, and since the
configuration lives in the database, it is fairly self contained.

If there are any changes necessary to the upstream code or build
system that would make your job in porting Telodendria easier, do not
hesitate to get involved by opening an issue and/or submitting a pull
request.

