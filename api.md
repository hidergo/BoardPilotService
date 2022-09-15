# API definition

## Server

* Raw TCP server
* Running on port 24429

## Authentication

To prevent other software to accidentally connecting to this service, a client must authenticate.

Authentication should happen after establishing connection by sending the following message:

```
{
    cmd: 0x01,
    key: "p*kG462jhJBY166EZLKxf9Du",
    reqid: number
}
```
Server returns:
```
{
    cmd: 0x01,
    status: boolean,
    reqid: number
}
```
If the client does not authenticate correctly or at all, server will drop the client.

## Messages

All messages should contain "cmd" field for command and "reqid", a running/unique number for the request.

List of messages which client can request from the API:

**APICMD_DEVICES**

Request connected devices:
```
{
    cmd: 0x10,
    reqid: number
}
```
Server response:
```
{
    cmd: 0x10,
    devices: [
        {
            product: {
                vid: number,
                pid: number,
                manufacturer: string,
                product: string,
                rev: number
            },
            device: {
                serial: string
            }
        },
        ...
    ]
    reqid: number
}
```

---

List of messages which API may send to client (not requested):

**APICMD_EVENT**

An event, for example device connecting or disconnecting. 

Server message:
```
{
    cmd: 0x10,
    type: number,
    device: {
        product: {
            vid: number,
            pid: number,
            manufacturer: string,
            product: string,
            rev: number
        },
        device: {
            serial: string
        }
    },
    reqid: number
}
```
