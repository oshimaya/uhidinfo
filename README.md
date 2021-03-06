# uhidinfo - Get uhid device information for NetBSD
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
 056A  0302    0          2     tablets/CTH480        # WACOM Intuos CTH480
 056A  0303    0          2     tablets/CTH680        # WACOM Intuos CTH680
 056A  0374    0         16     tablets/CTL4100       # WACOM Intuos CTL4100
 056A  0378    0         16     tablets/CTL6100       # WACOM Intuos BT CTL6100WL
 056A  50A2    1          6     tablets/dynabook      # dynabook V internal
 28BD  092B    1          7     tablets/XPPENPro13    # XP-PEN Pro 13.3
 28BD  0932    1          7     tablets/XPPENDecoFunL # XP-PEN DECO FunL
```

 - VID: Vendor ID. HexDecimal.
 - PID; Product ID. HexDecimal.
 - INTERFACE: Interface Number. Decimal.
 - REPORTID: Report ID. Decimal.
 - ALTNAME: Alternative device name for device alias. Strings.

### run

List:
```
 $ uhidinfo /dev/uhid*
 /dev/uhid0: Vendor=056A Product=50A2 Interface=0 Report=2
 /dev/uhid1: Vendor=056A Product=50A2 Interface=0 Report=3
 /dev/uhid10: Vendor=056A Product=50A2 Interface=1 Report=7
 /dev/uhid11: Vendor=056A Product=50A2 Interface=1 Report=11
 /dev/uhid12: Vendor=056A Product=50A2 Interface=1 Report=15
 /dev/uhid13: Vendor=056A Product=50A2 Interface=1 Report=16
 /dev/uhid14: Vendor=056A Product=50A2 Interface=1 Report=17
 /dev/uhid15: Vendor=056A Product=50A2 Interface=1 Report=18
 /dev/uhid16: Vendor=056A Product=50A2 Interface=1 Report=19
 /dev/uhid17: Vendor=17EF Product=60EE Interface=1 Report=5
 /dev/uhid18: Vendor=17EF Product=60EE Interface=1 Report=16
 /dev/uhid19: Vendor=17EF Product=60EE Interface=1 Report=18
 /dev/uhid2: Vendor=056A Product=50A2 Interface=0 Report=4
 /dev/uhid20: Vendor=17EF Product=60EE Interface=1 Report=19
 /dev/uhid21: Vendor=17EF Product=60EE Interface=1 Report=21
 /dev/uhid22: Vendor=17EF Product=60EE Interface=1 Report=22
 /dev/uhid23: Vendor=17EF Product=60EE Interface=2 Report=4
 /dev/uhid24: Vendor=056A Product=0303 Interface=0 Report=2
 /dev/uhid25: Vendor=056A Product=0303 Interface=0 Report=3
  (snip)
```

Search:
```
 $ uhidinfo -s /dev/uhid24
 /dev/uhid24: tablets/CTH680
```

## quiet mode

When run with -q flag, reports minimum output (for devpubd(8) ).

```
 $ uhidinfo -s -q /dev/uhid24
 tablets/CTH680
```

## kernel patch

Add ioctl(USB_GET_INTERFACE_DESC) to uhid(4).

for netbsd-9:
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

for netbsd-current (after 2022-03-28)

```
--- a/sys/dev/usb/uhid.c
+++ b/sys/dev/usb/uhid.c
@@ -89,6 +89,7 @@ struct uhid_softc {
        struct uhidev *sc_hdev;
        struct usbd_device *sc_udev;
        uint8_t sc_report_id;
+       struct usbd_interface *sc_iface;

        kmutex_t sc_lock;
        kcondvar_t sc_cv;
@@ -182,6 +183,7 @@ uhid_attach(device_t parent, device_t self, void *aux)
        sc->sc_hdev = uha->parent;
        sc->sc_udev = uha->uiaa->uiaa_device;
        sc->sc_report_id = uha->reportid;
+       sc->sc_iface = uha->uiaa->uiaa_iface;

        selinit(&sc->sc_rsel);

@@ -589,6 +591,10 @@ uhidioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
                memcpy(rd->ucrd_data, desc, size);
                break;

+       case USB_GET_INTERFACE_DESC:
+               memcpy((usb_interface_descriptor_t *)addr,
+                       usbd_get_interface_descriptor(sc->sc_iface), sizeof(usb_interface_descriptor_t));
+               break;
        case USB_SET_IMMED:
                if (*(int *)addr) {
                        extra = sc->sc_report_id != 0;

```

## devpubd hook script

for example:

```
#!/bin/sh
#
# check hid device and create alternative symlink
#

event="$1"
shift
devices=$@

for device in $devices
do
        case $device in
        uhid[0-9]*)
                case $event in
                device-attach)
                        altname=$(/usr/local/bin/uhidinfo -s -q /dev/$device)
                        if [ $? -eq 0 ]; then
                                ln -sf /dev/$device /dev/${altname}
                        fi
                        ;;
                device-detach)
                        ;;
                esac
                ;;
        esac
done

```

## Bugs

Many.

 - Multiple USB Configuration
 - Multiple the same devices at the same time (Use serial number? UUID?)
