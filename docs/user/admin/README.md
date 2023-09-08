# Administrator API

Telodendria provides an administrator API as an extension to the
Matrix specification that allows for administrator control over the
homeserver. This includes profiling and examining the state of
running instances, as well as managing users and media.

Like Synapse, Telodendria supports designating specific local users as
administrators. However, unlike Synapse, Telodendria uses a more
fine-grained privilege model that allows a server administrator to
delegate specific administration tasks to other users while not
compromising and granting them full administrative access to the server.

To authenticate with the administrator API, simply use your login
access token just like you would authenticate any other Matrix client
request.

- [Privileges](privileges.md)
- [Configuration](config.md)
- [Server Statistics](stats.md)
- [Process Control](proc.md)

