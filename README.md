# AIDL GNSS HAL for Raspberry Pi 5

Working GNSS HAL for Android 14+ (AIDL). Parses NMEA from UART GPS modules.

**Why this exists:** Most GNSS HALs are HIDL (Android 13 and older) or mock implementations. This is a real, production-tested AIDL HAL.

## Versions

| Folder | Output | Use case |
|--------|--------|----------|
| `basic/` | 1Hz (native GPS rate) | Simple, debugging |
| `interpolation/` | 10Hz (smooth) | **Navigation apps** |

## Hardware

**Tested modules:**
- Quectel LC29H

**Wiring:**
```
GPS TX  →  RPi5 GPIO15 (RXD)
GPS GND →  RPi5 GND
GPS VCC →  RPi5 3.3V
```

## Installation

1. Copy files to `hardware/interfaces/gnss/aidl/rpi5/`

2. Choose version:
```bash
cp basic/NmeaReader.* .        # OR
cp interpolation/NmeaReader.* .
```

3. Add to device makefile:
```makefile
PRODUCT_PACKAGES += android.hardware.gnss-service.rpi5
```

4. Build:
```bash
source build/envsetup.sh
lunch aosp_rpi5-userdebug
mmm hardware/interfaces/gnss/aidl/rpi5
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No GPS data | Check wiring: GPS TX → Pi RX (cross!) |
| 188+ satellites | Use `interpolation/` version (has timeout cleanup) |
| Jerky navigation | Use `interpolation/` version |
| Permission denied | `chmod 666 /dev/ttyAMA0` or check SELinux |

## Debug

```bash
adb logcat -s GnssHal:V GnssNmeaReader:V
adb shell su -c "cat /dev/ttyAMA0"
```

## License

Apache 2.0
