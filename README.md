# CTS Strasbourg Ticket Reader / Decoder

A lightweight C utility to read, dump, and decode the raw memory of Strasbourg CTS (Compagnie des Transports Strasbourgeois) disposable paper tickets.

These tickets use the **ST Microelectronics SRx (ISO 14443-B)** chip protocol, which is notoriously difficult to read with standard NFC tools due to specific modulation requirements (`ISO14443B2SR`) and weak antenna coupling.

## 🚀 Features
* **Raw Memory Dump:** Bypasses standard drivers to read the full 512-bit (64-byte) memory of the SRI512 chip.
* **Automatic Decoding:** Parses hex data to reveal:
    * **Ticket Type** (e.g., 24 Hour Pass).
    * **Validity Counter** (Remaining rides/validations).
    * **Timestamp** (Decodes the exact Date and Time of the last validation).
    * **UID** (Hardware Serial Number).
* **Libnfc Implementation:** Uses direct hardware addressing to fix "coupling" issues common with USB readers.

## 🛠 Hardware Requirements
* **NFC Reader:** ACS ACR122U (or any `libnfc` compatible reader).
* **Ticket:** Strasbourg CTS "Billet Sans Contact" (Paper ticket with square antenna).
* **OS:** Linux (Ubuntu/Debian/Kali).

## 📦 Prerequisites
You need `libnfc` and `gcc` installed on your system.

```bash
sudo apt update
sudo apt install libnfc-bin libnfc-dev gcc
````

## ⚙️ Compilation

Clone this repo and compile the `dump_ticket.c` file:

```bash
gcc -o dump_ticket dump_ticket.c -lnfc
```

## 📖 Usage

### 1\. Kill Conflicting Drivers

The default Linux smart card drivers (`pcscd` and `pn533`) often conflict with the raw access needed for these chips. **You must stop them before running the tool.**

```bash
sudo systemctl stop pcscd
sudo systemctl stop pcscd.socket
sudo modprobe -r pn533_usb pn533 nfc
```

### 2\. Run the Ripper

Run the executable with root privileges (required for hardware access):

```bash
sudo ./dump_ticket
```

### 3\. The "Rim Hover" Technique

These paper tickets have very small antennas. **Do not place the ticket in the center of the reader.**

1.  Place the ticket on the **outer plastic rim (edge)** of the reader.
2.  Move it slowly until the text appears.
3.  The tool scans continuously for the specific `ISO14443B2SR` protocol.

## 🖥️ Sample Output

When a ticket is successfully read, the tool outputs the raw hex table followed by a decoded report:

```text
--- STRASBOURG TICKET RIPPER ---
[*] Reader opened: ACS / ACR122U PICC Interface
[*] Place ticket on the RIM/EDGE now...

[+] TICKET DETECTED! (UID: 80 XX XX XX XX XX XX XX)

Reading memory...
Blk 00 | 59 00 00 00   Y...
Blk 01 | 40 91 00 25   @..%
Blk 02 | AA BB CC DD   ....  <-- Ticket ID (Anonymized)
Blk 03 | 11 22 33 44   ."3D
Blk 04 | 00 00 00 40   ...@
Blk 05 | 38 0a 00 fe   8...
Blk 06 | fd ff ff 00   ....
...
Blk 10 | fd 24 00 00   .$..
Blk 11 | 09 a8 19 fb   ....
Blk 12 | 00 00 b6 02   ....
Blk 13 | 6a 16 02 00   j...  <-- Validated on Day 22 @ 12:50
Blk 14 | DE AD BE EF   ....
Blk 15 | CA FE BA BE   ....

========================================
       TICKET FORENSICS REPORT          
========================================
[*] TICKET TYPE:    24 HOUR PASS (Detected)
[*] COUNTER VALUE:  10 (Raw Hex: 0A)
[*] LAST RIDE:      Day 22 of the month
[*] TIME STAMP:     12:50 (Calculated)
========================================
```

## 🧠 Memory Map (Forensics)

Based on reverse engineering, here is the structure of the CTS Ticket (ST SRI512 Chip):

| Block | Raw Hex Example | Description |
| :--- | :--- | :--- |
| **UID** | `80 XX ...` | **Serial Number** (Printed on back). Read-only Hardware ID. |
| **00-01** | `59 00 ...` | **System/Lock Bits** (Chip configuration). |
| **02-03** | `AA BB ...` | **Purchase ID**. Static transaction number (e.g., Ticket \#68,062,930). |
| **05** | `38 0A ...` | **Counter**. `0A` = 10. Decrements or locks upon use. |
| **10** | `fd 24 ...` | **Product ID**. `24` = 24 Hour Pass. |
| **11-12** | `09 a8 ...` | **Validator ID**. Serial number of the bus/tram machine used. |
| **13** | `6a 16 ...` | **Timestamp**. Byte 0 = Time (5-min units from 4am), Byte 1 = Day of Month. |
| **14-15** | `dd ca ...` | **Signature**. HMAC/Checksum of UID + Timestamp to prevent tampering. |

## ⚠️ Disclaimer

This tool is for **educational and research purposes only**. It allows you to read your own data to understand how the system works. It does not allow cloning or fare evasion (the Anti-Replay signatures and OTP counters prevent this).
**The author is not responsible for any misuse of this software.**
