# NdisHook
Hijacking the Windows network stack from the kernel :)

> [!WARNING]
> NdisHook is a simple **proof-of-concept** kernel driver. The techniques used in this project have been publicly disclosed prior to this release. This driver should only be run on systems you are own and/or are authorized to test. This driver is provided "as is" without warranty of any kind. The author(s) assume no liability for damages, system instability, or misuse of this code. By using this driver, you agree to assume all associated risks.

## What is this?
NdisHook is a kernel-mode driver that is designed to intercept TCP packets by "hooking" internal structures within the NDIS driver.

## How does it work?
NdisHook first calls the `NdisRegisterProtocolDriver` function with mostly null pointers. According to Microsoft's documentation, the `NdisProtocolHandle` parameter is simple pointer type called `PNDIS_HANDLE`. In reality, this pointer is of an undocumented (officially, at least) structure called `_NDIS_PROTOCOL_BLOCK`. This structure is quite large, but contains a linked list member called `NextProtocol` which points to another registered protocol (duh!). We can walk this linked list until we find the `TCPIP` device (indicated by the `Name` member). The `OpenQueue` member of `_NDIS_PROTOCOL_BLOCK` for the `TCPIP` device is a pointer to another undocumented structure called `_NDIS_OPEN_BLOCK`. This structure contains tons of pointers to functions that will ultimately handle incoming and outgoing TCP connections (an astute reader might recognize these function pointers because even the "fake protocol" driver needs to register these functions! We kept them null since our driver is not being used. We only call this function to locate the aforementioned pointers for our shenanigans). By swapping the function pointers for `ReceiveNetBufferLists` and `SendNetBufferLists` to functions inside our own driver, we can intercept the `_NET_BUFFER_LIST` structures being passed to them. This nested structure contains the Memory Descriptor Lists (MDLs) for both inbound and outbound raw TCP packet data!

Since we can intercept all TCP packet data (via the `ReceiveNetBufferLists` hook), we can scan each packet for a "magic constant" (`MP5` in this case) and any given command. In this PoC, a `kill` command can be specified followed by a process ID. This way, you could kill any process just by sending a packet through an already open TCP port! The same logic can be applied to the `SendNetBufferLists` hook (not used). Use your imagination. 

## Usage
Enable test signing mode
```
bcdedit /set testsigning on
```

Create and start driver service
```
sc.exe create NdisHook binPath=C:\Users\user\Desktop\NdisHook.sys type=kernel && sc.exe start NdisHook
```

Run the `controller.py` script with the target IP and port (any open TCP port can be used)
```
# python controller.py <ip> <port>

# Example with NetBIOS port
python controller.py 192.168.43.130 139

connected: 192.168.43.130:139
controller > kill 1234
```

## Resources
- [SECURITY.COM - Daxin Backdoor: In-Depth Analysis, Part One](https://www.security.com/threat-intelligence/daxin-malware-espionage-analysis)
- [BlackHat 2006 - Rootkits: Attacking Personal Firewalls](https://blackhat.com/presentations/bh-usa-06/BH-US-06-Tereshkin.pdf)
- [United States Patent: DETECTING DEVIATION FROM A DATA PACKET SEND-PROTOCOL IN A COMPUTER SYSTEM](https://patentimages.storage.googleapis.com/45/8e/31/4c1122aacfcac7/US9654498.pdf)
    - NOTE: No technique described in this patent was used directly.  