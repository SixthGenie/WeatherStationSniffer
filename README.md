This is an attempt do decode the usb protocol that can be read from a Weather Station from Clas Ohlson.
I plan to hook up an ESP32-S3 to this to send data out over WIFI.

The first version is a Windows console application to dump data read over USB. The values read was compared
by those given by the WeatherHome software supplied with the weather station. This is the format of the USB
packet read:

## USB Data Packet Map (76-Byte Payload)

The FT0203A (Fine Offset / WH1080 clone) transmits weather data over USB in a 76-byte fixed-length packet structure. The following memory map details the specific offsets, data sizes, and bitwise decoding math required to extract real-time sensor metrics.

### Packet Layout Table

| Hex Offset | Data Size | Data Type | Sensor / Parameter | Decoding Logic & Conversion Math |
| :--- | :--- | :--- | :--- | :--- |
| **`0x00`** | 4 Bytes | `uint8_t[4]` | **Packet Header** | Fixed identification signature: `4C 04 01 01` |
| **`0x07`** | 2 Bytes | `uint16_t` | **Indoor Temperature** | `((le16(0x07) & 0x0FFF) - 400) * 0.1` (Fahrenheit scaled $\rightarrow$ Convert to °C) |
| **`0x09`** | 1 Byte  | `uint8_t`  | **Indoor Humidity** | Direct integer value in `%` |
| **`0x0A`** | 2 Bytes | `uint16_t` | **Outdoor Temperature**| `((le16(0x0A) & 0x0FFF) - 400) * 0.1` (Fahrenheit scaled $\rightarrow$ Convert to °C) |
| **`0x16`** | 1 Byte  | `uint8_t`  | **Outdoor Humidity**| Direct integer value in `%` |
| **`0x34`** | 2 Bytes | `uint16_t` | **Absolute Pressure** | `le16(0x34) * 0.1` (Result in hPa / mbar) |
| **`0x36`** | 2 Bytes | `uint16_t` | **Relative Pressure** | `le16(0x36) * 0.1` (Result in hPa / mbar) |
| **`0x3A`** | 1 Byte  | `uint8_t`  | **Wind Speed** | `raw_byte * 0.1` (Result in m/s) |
| **`0x3B`** | 2 Bytes | `uint16_t` | **Wind Gust** | `le16(0x3B) * 0.1` (Result in m/s) |
| **`0x3D`** | 1 Byte  | `uint8_t`  | **Wind Direction** | Raw meteorological degrees ($0^\circ \text{ to } 360^\circ$) |
| **`0x3F`** | 2 Bytes | `uint16_t` | **Rain Last Hour** | Returns `0.0` if `0xA7FA` (Null Flag), else `(le16(0x3F) >> 4) * 0.1` (mm) |
| **`0x41`** | 2 Bytes | `uint16_t` | **Rain Today** | Returns `0.0` if `0x7FA7` (Null Flag), else `(le16(0x41) >> 4) * 0.1` (mm) |
| **`0x43`** | 2 Bytes | `uint16_t` | **Rain This Week** | Returns `0.0` if `0xFAA7` (Null Flag), else `(le16(0x43) >> 4) * 0.1` (mm) |
| **`0x45`** | 2 Bytes | `uint16_t` | **Rain This Month** | `(le16(0x45) >> 4) * 0.1` (mm) |
| **`0x48`** | 2 Bytes | `uint16_t` | **Total Accumulated Rain**| `le16(0x48) * 0.1` (mm) Lifetime counter |
| **`0x4A`** | 2 Bytes | `uint16_t` | **Station Tracker** | Internal EEPROM rolling write-cycle sequence counter |

---

### Critical Implementation Details

#### 1. Temperature Flag Masking (12-bit Extraction)
Both the indoor and outdoor temperature blocks are stored across 2 bytes, but the hardware utilizes the upper 4 bits for transmission metadata and status flags. To get stable data, isolate the lower 12 bits before applying the translation offset:

```cpp
// Isolation mask: 0x0FFF
int16_t raw_temp = le16(packet, 0x0A) & 0x0FFF;
double temp_fahrenheit = (raw_temp - 400) * 0.1;
double temp_celsius = (temp_fahrenheit - 32.0) * (5.0 / 9.0);
```

### Time-Window Rain Shift
The dynamic rain arrays (Hour, Day, Week, Month) use high-nibble bit packing. The raw little-endian reading must be right-shifted by 4 bits (>> 4) before parsing decimals:

```cpp
uint16_t raw_rain = le16(packet, 0x45);
double rain_mm = (raw_rain >> 4) * 0.1;
