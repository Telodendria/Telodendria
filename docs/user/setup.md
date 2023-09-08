# Initial Set Up

While Telodendria strives to be extremely simple to deploy and run,
in most circumstances a few basic setup steps will be necessary.
Telodendria does not have a traditional configuration file like most
daemons. Instead, its configuration lives in its database; as such,
all configuration happens through the administrator API. This design
decision makes Telodendria extremely flexible, because it is possible
to re-configure Telodendria without having to manually edit files on
the filesystem, thus allowing administrators to secure their server
better.

Please follow the instructions followed here carefully in the order
they are presented for the best results.

This document assumes that you have installed Telodendria using any
of the instructions found in [Install](install.md). After installation,
follow these steps:

1. Start Telodendria. If you installed it via a package or container,
consult your operating system or container system's documentation. If
you are running Telodendria from a release binary or have built it from
source, execute the binary directly. If needed, consult the
[Usage](usage.md) page for details on how to run Telodendria.
1. Assuming that Telodendria started properly, it will spin up and
initialize its database directly with a simple&mdash;and, importantly,
safe&mdash;default configuration, as well as a randomly generated,
single-use registration token that grants a user all privileges
documented in the [Administrator API](admin/README.md) documentation.
Consult the log file for this administrator registration token. By
default, the log file is located in the data directory, and is named
`telodendria.log`.
1. Use the registration token to register for an account on the
server. This account will be the administrator account. You can do this
using the client of your choice, or using tools such as `curl` or
`http`, following the Matrix specification for registering accounts.
The administrator account behaves just like a normal local account
that an ordinary user would have registered on the server, except that
it also has all privileges granted to it, so it can make full use of
the Administrator API.
1. Using the access token granted for the administrator account via
the login process, configure Telodendria as descibed in
[Configuration](config.md). See the [Administrator API](admin/README.md)
documentation for the configuration endpoint details.

This is the recommended way to set up Telodendria. However, if you
wish to bypass the account creation step and want to configure
Telodendria by directly writing a configuration file instead of using
the administrator API, you can manually create the configuration file
in the database before starting Telodendria. Simply create `config.json`
following the description in [Configuration](config.md), then start
Telodendria.

While this alternative method may seem simpler and more convenient
to some administrators, it is only so in the short-term. Note that this
method is not supported, because it gives no access to the
administrator API whatsoever, unless you manually modify the database
further to give a user admin privileges, which is error-prone and
bypasses some of Telodendria's safety mechanisms.

