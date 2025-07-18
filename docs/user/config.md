# Configuration

Telodendria is designed to be configurable. It is configured using
JSON, which is intended to be submitted to the [Administrator
API](admin/README.md). This document details Telodendria's configuration
JSON format, which is used in both the administrator API and on-disk
in the database. The configuration file on the disk in the databsae
is `config.json`, though that file should not be edited by hand.
Use the API described in
[Administrator API &rightarrow; Configuration](admin/config.md).

## JSON Format

Telodendria's configuration is just a JSON object in the standard
key-value form:

```json
{
    "serverName": "telodendria.org",
    "listen": [
        {
            "port": 8008
        }
    ]

    /* ... */
}
```

Some keys, called *directives* in this document, have values that are
objects themselves.

## Directives

Here are the top-level directives:

- **listen:** `Array`
  
  An array of listener description objects. Telodendria supports
  listening on multiple ports, and each port is configured
  independently of the others. A listener object looks like this:

  - **port:** `integer`

    The port to listen on. Telodendria will bind to all interfaces,
    so it is recommended to configure your firewall to only allow
    access on the desired interfaces. Note that Telodendria offers all
    APIs over each port, including the administrator APIs; there is no
    way to control which APIs are made available over which ports. If
    this is a concern, a reverse-proxy such as `relayd` can be placed
    in front of Telodendria to block access to undesired APIs.

  - **tls:** `Object`

    Telodendria can be compiled with TLS support. If it is, then a
    particular listener can be set to use TLS for connections. If
    **tls** is not `null` or `false`, then it can be an object with
    the following directives:

    - **cert:** `String`

      The full path&mdash;or path relative to the data
      directory&mdash;of the certificate file to load. The certificate
      file should be in the format expected by the platform's TLS
      library.

    - **key:** `String`

      Same as **cert**, but this should be the private key that matches
      the certificate being used.

  - **threads:** `Integer`
    
    How many worker threads to spin up to handle requests for this
    listener. This should generally be less than the total CPU core
    count, to prevent overloading the system. The most efficient number
    of threads ultimately depends on the configuration of the machine
    running Telodendria, so you may just have to play around with
    different values here to see which gives the best performance.

    Note that this can be set as low as 0; in that case, the listener
    will never respond to requests. Each listener needs to have at
    least one thread to be useful. Also note that Telodendria may spin
    up additional threads for background work, so the actual total
    thread count at any given time may exceed the sum of threads
    specified in the configuration.

    This directive is optional. The default value is `4` in the upstream
    code, but your software distribution may have patched this to be
    different.

  - **maxConnections:** `Integer`

    The maximum number of simultanious connections to allow to the
    daemon. This option prevents the daemon from allocating large
    amounts of memory in the event that it undergoes a denial of
    service attack. It is optional, defaults to `32`, and typically
    does not need to be adjusted.

- **serverName:** `String`

  Configure the domain name of your homeserver. Note that Matrix
  servers cannot be migrated to other domains, so once this is set,
  it should never change unless you want unexpected things to happen
  or you want to start over. **serverName** should be a DNS name that
  can be publicly resolved. This directive is required.

- **pid:** `String`
  Configure the file Telodendria writes its PID to.

- **baseUrl:** `String`

  Set the server's base URL. **baseUrl** should be a valid URL,
  complete with the protocol. It does not need to be the same as the
  server name; in fact, it is common for a subdomain of the server name
  to be the base URL for the Matrix homeserver.

  This URL is the URL at which Matrix clients will connect to the
  server, and is thus served as a part of the `.well-known`
  manifest.

  This directive is optional. If unspecified, it is automatically
  deduced from the server name.

- **identityServer:** `String`

  The identity server that clients should use to perform identity
  lookups. **identityServer** folows the same rules as **baseUrl**.
  It also is optional, and is set to be the same as the **baseUrl**
  if left unspecified.

- **runAs:** `Object`
  
  The effective Unix user and group to drop to after binding to the
  socket and completing any setup that may potentially require
  elevated privileges. This directive only takes effect if
  Telodendria is started as the root user, and is used as a security
  mechanism. If this option is set and Telodendria is started as a
  non-privileged user, then a warning is printed to the log if that
  user and group do not match what's specified here. This directive
  is optional, but should be used as a sanity check even if not
  running as `root`, just to make sure you have your permissions
  working properly.

  This directive takes an object with the following directives:

  - **uid:** `String`

    The Unix username to switch to. If **runAs** is specified, this
    directive is required.

  - **gid:** `String`

    The Unix group to switch to. This directive is optional; if left
    unspecified, then the value of **uid** is copied.

- **federation:** `Boolean`

  Whether or not to enable federation with other Matrix homeservers.
  Matrix by its very nature is a federated protocol, but if you just
  want to rn your own internal chat server with no contact with the
  outside, then you can use this option to disable federation. It is
  highly recommended to set this to `true`, however, if you wish to
  be able to communicate with users on other Matrix servers. This
  directive is required.

- **registration:** `Boolean`

  Whether or not to enable new user registration or not. For security
  and anti-spam reasons, you can set this to `false`. If you do, you
  can still allow only certain users to be registered using
  registration tokens, which can be managed via the administrator API.
  This directive is required.

  In an ideal world, everyone would run their own Matrix homeserver,
  so no public registration would ever be required. Unfortunately,
  not everyone has the means to run their own homeserver, especially
  because of the fact that IPv4 addresses are becoming increasingly
  hard to come by. If you would like to provide a service to those
  that are unable to run their homeserver, then set this to `true`,
  thereby allowing anyone to create an account.

  Telodendria *should* be capable of handling a large amount of users
  without difficulty, but it is targetted at smaller deployments.

- **log:** `Object`

  The logging configuration. Telodendria uses its own logging
  facility, which can output logs to standard output, a file, or the
  syslog. This directive is required, and it takes an object with the
  following directives:

  - **output:** `Enum`

    The log output destination. This can either be `stdout`, `file`,
    or `syslog`. If set to `file`, Telodendria will log to
    `telodendria.log` inside the data directory.

  - **level:** `Enum`

    The level of messages to log. Each level shows all the levels above
    it. The levels are as follows:

    - `error`
    - `warning`
    - `notice`
    - `message`
    - `debug`

    For example, setting the level to `error` will show only errors,
    while setting the level to `warning` will show both warnings
    *and* errors. The `debug` level shows all messages.

  - **timestampFormat:** `Enum`

    If you want to customize the timestamp format shown in the log,
    or disable it altogether, you can do so via this option. Acceptable
    values are `none`, `default`, or a formatter string as described
    by your system's `strftime()` documentation. This option only
    applies if **log** is `stdout` or `file`.

  - **color:** `Boolean`

    Whether or not to enable colored output on TTYs. Note that ANSI
    color sequences will not be written to a log file, only a real
    terminal, so this option only applies if the log is being written
    to a standard output which is connected to a terminal.

- **maxCache:** `Integer`

  The maximum size of the cache. Telodendria relies heavily on caching
  for performance reasons. The cache grows as data is loaded from the
  data directory. All cache is stored in memory. This option limits the
  size of the memory cache. If you have a system with a lot of memory
  to spare, you'll get better performance if this option is set higher.
  Otherwise, this value should be lowered on systems that have a
  minimal amount of memory available.


## Examples

A number of example configuration files are shipped with Telodendria's
source code. They can be found in the `contrib/` directory if you are
viewing the source code directly. Otherwise, if you installed
Telodendria from a package, it is possible that the example
configurations were placed in the default locations for such files on
your operating system.

