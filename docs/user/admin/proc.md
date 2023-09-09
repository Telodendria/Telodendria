# Administrator API: Process Control

This document describes the administrator APIs that allow a server
administrator to manage the Telodendria process itself.

## API Endpoints

### **POST** `/_telodendria/admin/restart`

Restart the Telodendria daemon cleanly. This endpoint will respond
immediately after signaling to the daemon that it should be restarted
as soon as possible. Note that the restart wmay not happen
instantaneously, as Telodendria will finish processing all current
requests before restarting. Also note that this is not a true restart;
the process does not exit and restart, rather, Telodendria simply tears
down all its state and then jumps back to the beginning of its code and
starts over.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The restart request was successfully sent.|

On success, this endpoint simply returns an empty JSON object.

### **POST** `/_telodendria/admin/shutdown`

Shut down the Telodendria process cleanly. This endpoint will respond
immediately after signalling to the daemon that it should be shut
down as soon as possible. Note that the shutdown may not happen
instantaneously, as Telodendria will finish processing all current
requests before shutting down. Also note that once shut down, Telodendria
may be automatically restarted by the system's service manager.
Otherwise, it will have to be manually restarted. This is a true
shutdown; the Telodendria process exits as soon as possible.

| Requires Token | Rate Limited |
|----------------|--------------|
| Yes            | Yes          |

| Response Code | Description |
|---------------|-------------|
| 200           | The shutdown request was successfully sent.|

On success, this endpoint simply returns an empty JSON object.

