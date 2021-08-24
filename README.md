# uhidinfo - Get uhid device information

uhidinfo reports some basic information from /dev/uhid* device.

   - USB Vendor ID
   - USB Product ID
   - USB Interface Number (only custom kernel with original patch)
   - USB HID report ID

Also reports alternative name from device list file.

## Get Information

```
 $ uhidinfo /dev/uhid1
```
 
## Report the alternative name
### device list file 
 - uhidlist

### format

```
 # VID  PID  INTERFACE REPORTID  ALTNAME
 056A  0302    0          2     tablets/CTL480        # Intuos CTH480
 056A  0378    0         16     tablets/CTL6100       # Intuos BT CTL6100WL
 28BD  092B    1          7     tablets/XPPENPro13    # XP-PEN Pro 13.3
 28BD  0932    1          7     tablets/xPPENDecoFunL # XP-PEN DECO FunL
```

 - VID: Vendor ID. HexDecimal.
 - PID; Product ID. HexDecimal.
 - INTERFAC: Interface Number. Decimal.
 - REPORTID: Report ID. Decimal.
 - ALTNAME: Alternative device name for device alias. Strings.

### run

```
 $ uhidinfo -s /dev/uhid0
```

## quiet mode

When run with -q flag, reports minimum output (for devpubd(8) ).

## kernel patch

Add ioctl(USB_GET_INTERFACE_DESC) to uhid(4).

```
--- a/sys/dev/usb/uhid.c
+++ b/sys/dev/usb/uhid.c
@@ -695,6 +695,10 @@ uhid_do_ioctl(struct uhid_softc *sc, u_long cmd, void *addr,
                memcpy(rd->ucrd_data, desc, size);
                break;

+       case USB_GET_INTERFACE_DESC:
+               memcpy((usb_interface_descriptor_t *)addr,
+                       usbd_get_interface_descriptor(sc->sc_hdev.sc_parent->sc_iface), sizeof(usb_interface_descriptor_t));
+               break;
        case USB_SET_IMMED:
                if (*(int *)addr) {
                        extra = sc->sc_hdev.sc_report_id != 0;
```

## devpubd hook script

(T.B.D)

## Bugs

Many.

 - Multiple USB Configuration
 - Multiple the same devices at the same time (Use serial number? UUID?)
