# Administrator API: Configuration

As mentioned in [Setup](../setup.md), Telodendria's configuration is
intended to be managed via the configuration API. Consult the
[Configuration](../config.md) document for a complete list of supported
configuration options. This document simply describes the API used to
update the configuration described in that document.

## API Endpoints

### **GET** `/_telodendria/admin/v1/config`

Retrieve the current configuration.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The current configuration was successfully retrieved.|

### **POST** `/_telodendria/admin/v1/config`

Installs a new configuration. This endpoint validates the request body,
ensuring it is a proper configuration, then it replaces the existing
configuration with the new one.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The new configuration was successfully installed.|

#### 200 Response Format

| Field | Type | Description |
|-------|------|-------------|
| `restart_required` | `Boolean` | Whether or not the process needs to be restarted to finish applying the configuration. If this is `true`, then the restart endpoint should be used at a convenient time to apply the configuration.

### **PUT** `/_telodendria/admin/v1/config`

Update the currently installed configuration instead of completely replacing it. This endpoint
validates the request body, merges it on top of the current configuration, validates the resulting
configuration, then updates it in the database. This is useful when only one or two properties
in the configuration needs to be changed.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The new configuration was successfully installed.|

#### 200 Response Format

| Field | Type | Description |
|-------|------|-------------|
| `restart_required` | `Boolean` | Whether or not the process needs to be restarted to finish applying the configuration. If this is `true`, then the restart endpoint should be used at a convenient time to apply the configuration.
