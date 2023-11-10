# Administrator API: Registration Tokens

Telodendria implements registration tokens as specified by the Matrix
specification. These tokens can be used for registration using the
`m.login.registration_token` login type. This API provides a Telodendria
administrator with a mechanism for generating and managing these tokens,
which allows controlled registration on the homeserver.

It is generally safer than completely open registration to use
registration tokens that either expire after a short period of time, or
have a limited number of uses.

## Registration Token

A registration token is represented by the following `RegToken` JSON
object:

| Field | Type | Description |
|-------|------|-------------|
| `name` | `String` | The token identifier; what is used when registering. |
| `created_by` | `String` | The localpart of the user that created this token. |
| `created_on` | `Integer` | A timestamp of when the token was created. |
| `expires_on` | `Integer` | An expiration stamp, or 0 if the token never expires. |
| `used` | `Integer` | The number of times the token has been used. |
| `uses` | `Integer` | The total number of allowed uses, or -1 for unlimited. |
| `grants` | `[String]` | An array of privileges to grant users that register with this token as described in [Privileges](privileges.md). |

All endpoints in this API will operate on some variation of this
structure. The remaining number of uses can be computed by performing
the subtraction: `uses - used`. `used` should never be greater than
`uses` or less than `0`.

Example:

```json
{
    "name": "q34jgapo8uq34hg",
    "created_by": "admin",
    "created_on": 1699467640000,
    "expires_on": 0,
    "used": 3,
    "uses": 5
}
```

## API Endpoints

### **GET** `/_telodendria/admin/v1/tokens`

Get a list of all registration tokens and information about them.

#### 200 Response Format

| Field | Type | Description |
|-------|------|-------------|
| `tokens` | `[RegToken]` | An array of registration tokens. |

### **GET** `/_telodendria/admin/v1/tokens/[name]`

Get information about the specified registration token.

#### Request Parameters

| Field | Type | Description |
|-------|------|-------------|
| `name` | `String` | The name of the token, as it would be used to register a user. |

#### 200 Response Format

This endpoint returns a `RegToken` object that represents the server's
record of the registration token.

### **POST** `/_telodendria/admin/v1/tokens`

Create a new registration token.

#### Request Format

This endpoint accepts a `RegToken` object, as described above. If no
`name` is provided, one will be randomly generated. Note that the fields
`created_by`, `created_on`, and `used` are ignored and set by the server
when this request is made. All other fields may be set by the request
body.

#### 200 Response Format

If the creation of the registration token was successful, a `RegToken`
that represents the server's record of it is returned.

### **DELETE** `/_telodendria/admin/v1/tokens/[name]`

Delete the specified registration token. It will no longer be usable for
the registration of users. Any users that have completed the
`m.login.registration_token` step but have not yet created their account
should still be able to do so until their user-interactive auth session
expires.

#### Request Parameters

| Field | Type | Description |
|-------|------|-------------|
| `name` | `String` | The name of the token, as it would be used to register a user. |

#### 200 Response Format

On success, this endpoint returns an empty JSON object.