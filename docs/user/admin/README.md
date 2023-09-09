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

## API Conventions

Unless otherwise indicated, HTTP response codes that are not `200 Ok`
will be accompanied by a standard Matrix API error. Consult the Matrix
specification for the format of these errors. The following error
conditions are assumed to be possible for all API endpoints listed
in the Administrator API documentation:

| Response Code | Description |
|---------------|-------------|
| 400           | The user is not authenticated, did not provide a valid JSON object, or provided a JSON object with invalid or missing parameters.|
| 403           | The user does not have the privileges necessary to carry out the requested action.|
| 500           | A fatal server error occurred. Check the logs for more information.|

