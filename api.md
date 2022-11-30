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
                serial: string,
                protocol: 'usb'|'bt'
            }
        },
        ...
    ]
    reqid: number
}
```

**APICMD_SET_IQS_REGS**

Set IQS5xx (trackpad) registers:
```
{
    cmd:    0x40,
    device: string,
    save:   boolean,
    regs: {
        activeRefreshRate?:         number,
        idleRefreshRate?:           number,
        singleFingerGestureMask?:   number,
        multiFingerGestureMask?:    number,
        tapTime?:                   number,
        tapDistance?:               number,
        touchMultiplier?:           number,
        debounce?:                  number,
        i2cTimeout?:                number,
        filterSettings?:            number,
        filterDynBottomBeta?:       number,
        filterDynLowerSpeed?:       number,
        filterDynUpperSpeed?:       number,
        initScrollSpeed?:           number
    },
    reqid: number
}

```
Server response:
```

{
    cmd:    0x40,
    status: boolean,
    reqid:  number
}
```

**APICMD_GET_IQS_REGS**

Get IQS5xx (trackpad) registers:
```
{
    cmd:    0x41,
    device: string,
    reqid: number
}
```
Server response:
```
{
    cmd:    0x41,
    status: boolean,
    regs: {
        activeRefreshRate?:         number,
        idleRefreshRate?:           number,
        singleFingerGestureMask?:   number,
        multiFingerGestureMask?:    number,
        tapTime?:                   number,
        tapDistance?:               number,
        touchMultiplier?:           number,
        debounce?:                  number,
        i2cTimeout?:                number,
        filterSettings?:            number,
        filterDynBottomBeta?:       number,
        filterDynLowerSpeed?:       number,
        filterDynUpperSpeed?:       number,
        initScrollSpeed?:           number
    },
    reqid:  number
}
```

**APICMD_SET_KEYMAP**

Set keymap registers:
```
{
    cmd:    0x20,
    device: string,
    save: boolean,
    keymap: number[][][],
    reqid: number
}
```
`keymap` field is a 3 dimensional array in following format Layers -> \[Keys -> \[Single key -> \[device, param\]\]\].

example: 
```
keymap: [
    // Layer 0
    [
        [6, 0x70005], // would be letter B
        [6, 0x70004], // would be letter A
    ],
    // Layer 1
    [
        [6, 0x70006], // would be letter C
        [6, 0x70005], // would be letter B
    ]
]
```

Server response:
```
{
    cmd:    0x20,
    status: boolean,
    reqid:  number
}
```

**APICMD_GET_KEYMAP**

Get keymap registers:
```
{
    cmd:    0x21,
    device: string,
    reqid: number
}
```

Server response:
```
{
    cmd:    0x21,
    status: boolean,
    keymap: [][][]
    reqid:  number
}
```

See keymap format from *APICMD_SET_KEYMAP*


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
            serial: string,
            protocol: 'usb'|'bt'
        }
    },
    reqid: number
}
```
