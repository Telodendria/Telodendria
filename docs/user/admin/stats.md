# Administrator API: Server Statistics

The administrator API allows users with the proper privileges to get
information about how the server process is performing.

## API Endpoints

### **GET** `/_telodendria/admin/stats`

Retrieve basic statistics about the currently running Telodendria
process.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
| 200           | The server statistics were successfully retrieved.|

#### 200 Response Format

| Field | Type | Description |
| `memory_allocated` | `Integer` | The total amount of memory allocated, measured in bytes.|
| `version` | `String` | The current version of Telodendria.|
