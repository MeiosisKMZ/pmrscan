# PMRScan — Flipper Zero Application

## Introduction

**PMRScan** is a Flipper Zero Sub-GHz scanning application designed to cycle through common PMR (Private Mobile Radio) frequencies in the 446 MHz band. It monitors signal strength (RSSI) and provides **audio feedback through the Flipper speaker**, allowing users to hear when a signal is detected.

When a signal above the configured sensitivity threshold is detected, scanning pauses and the signal is routed to the speaker. Press the **OK (center) button** to toggle scanning/lock behavior.

---

## Features

* Scans common PMR 446 MHz channels
* Real-time RSSI signal monitoring
* Audio output via Flipper speaker
* Auto channel scanning (up/down direction control)
* Adjustable sensitivity threshold
* Manual lock/unlock on detected signals
* Visual UI with channel + RSSI display

---

## Supported Frequencies

The app scans standard PMR446 channels:

```
446.00625 MHz
446.01875 MHz
446.03125 MHz
446.04375 MHz
446.05625 MHz
446.06875 MHz
446.08125 MHz
446.09375 MHz
446.10625 MHz
446.11875 MHz
446.13125 MHz
446.14375 MHz
446.15625 MHz
446.16875 MHz
446.18125 MHz
446.19375 MHz
```

---

## Installation

### Requirements

* Flipper Zero
* Custom firmware with Sub-GHz API support
* Build environment (Flipper Zero SDK (Unleashed or OFW))

### Build

```bash
git clone https://github.com/MeiosisKMZ/pmrscan/
cd pmrscan
./fbt fap_pmrscan
```

### Install to device

```bash
./fbt flash_usb
```
Or drop the compiled (Released) `.fap` file for your firmware directly in `ext/apps/Sub-GHz/`

---

## Usage

1. Launch **PMRScan** from Flipper apps menu
2. Device begins scanning PMR channels automatically
3. When signal is detected:

   * Scanning pauses
   * Audio output is enabled
4. Press **OK** to lock/unlock scanning manually

---

## Troubleshooting

I don't have a walkie-talkie for the moment but I'm looking forward on testing my app against one.
If any issues with the app not working, open an issue on Github.

---

## Project Structure

```
pmrscan/
│
├── pmrscan_app.c        # Main logic (scanning, UI, audio)
├── pmrscan_app.h        # App structure and declarations
├── README.md            # Documentation
├── LICENSE              # License
├── application.fam      # App informations
└── icons                
      └── pmrscan.png    # Fap icon
```

---

## License

[LICENSE](LICENSE)

> The original work is from @thecybermind with [FRSScan](https://github.com/thecybermind/frsscan)
---
### Happy Hunting !
