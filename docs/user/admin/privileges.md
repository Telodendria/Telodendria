# Administrator API: Privileges

This document describes the privilege model and the API endpoints that
allow administrators to modify privileges for users.

## List Of Privileges

A local user can have any of the following privileges. Unless otherwise
indicated, these privileges only grant access to certain parts of the
administrator API; the regular Matrix API is unaffected.

- **DEACTIVATE:** Allows a user to deactivate any other local users.
- **ISSUE_TOKENS:** Allows a user to create, modify, and delete
registration tokens.
- **CONFIG:** Allows a user to modify the Telodendria server daemon's
configuration.
- **GRANT_PRIVILEGES:** Allows a user to modify his or her own
privileges or the privileges of other local users.
- **ALIAS:** Allows a user to modify and see room aliases created by 
other users. By default, users can only manage their own room aliases, 
but an administrator may wish to take over an alias or remove an 
offensive alias.
- **PROC_CONTROL:** Allows a user to get statistics on the running
process, as well as shutdown and resetart the Telodendria daemon
itself. Typically this will pair well with **CONFIG**, because there
are certain configuration options that require the process to be
restarted to take full effect.

There is also a special "pseudo-privilege":

- **ALL:** Grants a user all of the aforementioned privileges, as well
as privileges that do not yet exist. That is, if an update to
Telodendria adds more privileges, users with this privilege will
automatically gain those new privileges in addition to having all the
existing privileges. This privilege should only be used with
fully-trusted users. It is typical for a server administrator to not
fully trust anyone else, and be the only one that holds an account with
this privilege level.

## API Endpoints

The following API endpoints are implemented for managing privileges.

### **GET** `/_telodendria/admin/v1/privileges/[localpart]`

Retrieve the permissions for a user. If the localpart is omitted, then
retrieve the privileges for the user that owns the access token being 
used. Note that the owner of the access token must have the
**GRANT_PRIVILEGES** privilege to use this endpoint.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The privileges were successfully retrieved.|

#### 200 Response Format

| Field | Type | Description |
|-------|------|-------------|
| `privileges` | `Array` | An array of privileges, as described above. The privileges are encoded as JSON strings.|

### **POST** `/_telodendria/admin/v1/privileges/[localpart]`

Update the privileges of a local user by replacing the privileges array
with the one specified in the request. Like the **GET** version of this
endpoint, the localpart can be omitted to operate on the user that
owns the access token.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The privileges were successfully replaced.|

#### Request Format

| Field | Type | Description |
|-------|------|-------------|
| `privileges` | `Array` | An array of privileges, as described above. The privileges are encoded as JSON strings.|

#### 200 Response Format

| Field | Type | Description |
|-------|------|-------------|
| `privileges` | `Array` | An array of privileges, as described above. The privileges are encoded as JSON strings.|

### **PUT** `/_telodendria/admin/v1/privileges/[localpart]`

Update the privileges of a local user by adding the privileges
specified in the request to the users existing privileges.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The privileges were successfully added.|

#### Request Format

| Field | Type | Description |
|-------|------|-------------|
| `privileges` | `Array` | An array of privileges, as described above. The privileges are encoded as JSON strings.|

#### 200 Response Format

| Field | Type | Description |
|-------|------|-------------|
| `privileges` | `Array` | An array of privileges, as described above. The privileges are encoded as JSON strings.|

### **DELETE** `/_telodendria/admin/v1/privileges/[localpart]`

Update the privileges of a local user by removing the privileges
specified in the request from the user's existing privileges.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The privileges were successfully removed.|

#### Request Format

| Field | Type | Description |
|-------|------|-------------|
| `privileges` | `Array` | An array of privileges, as described above. The privileges are encoded as JSON strings.|

#### 200 Response Format

| Field | Type | Description |
|-------|------|-------------|
| `privileges` | `Array` | An array of privileges, as described above. The privileges are encoded as JSON strings.|

