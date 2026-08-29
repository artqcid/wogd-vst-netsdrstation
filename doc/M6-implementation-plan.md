---
type: Implementation Plan
title: M6 Implementation Plan — Multi-Provider Support
description: Step-by-step plan for M6: OpenWebRX, SpyServer, Web-888 support beyond KiwiSDR
status: draft
generated:
  by: human:marku
  at: 2026-08-27
verified:
  by: human:marku
  at: 2026-08-29
tags: [m6, multi-provider, openwebrx, spyserver, web-888]
resource: doc/station-list.md doc/architecture.md
---

# M6 Implementation Plan — Multi-Provider Support

_Cross-reference: `doc/checklist.md` · `doc/architecture.md` · `doc/station-list.md` ·
`doc/M5-implementation-plan.md`_

_Reference analysis: radiom (github.com/thepacket/radiom), VibeSDR (github.com/Stuey3D/VibeSDR),
KiwiAngel (github.com/Vort/KiwiAngel) — analysed 2026-08-27._

---

## Motivation

M1–M5 deliver a KiwiSDR-only plugin. M6 extends it to three additional SDR
server types, covering the majority of publicly accessible online receivers:

| Provider     | Protocol          | Typical use                          | Reference client             |
|--------------|-------------------|--------------------------------------|------------------------------|
| **KiwiSDR**  | Custom WS (M1–M5) | HF (0–30 MHz), global network        | kiwiclient, radiom           |
| **OpenWebRX**| WS + JSON         | HF + VHF/UHF, self-hosted            | radiom `openwebrx/client.ts` |
| **SpyServer**| Binary WS (proxy) | HF + VHF/UHF, AirSpy hardware        | radiom `spyserver/client.ts` |
| **RTL-TCP**  | Binary TCP        | VHF/UHF, RTL-SDR hardware            | radiom `rtltcp/client.ts`    |

---

## Architecture Overview

### Provider Abstraction Layer

radiom demonstrates the correct pattern: a single `KiwiHandlers`-shaped
interface (`onAudio`, `onWaterfall`, `onStatus`, `onMessage`, `onError`,
`onClose`) that all four providers implement. The player/shell wiring is
provider-agnostic.

In C++ terms (our plugin):

```
IReceiverClient (abstract)
    ├── KiwiClient          (existing, M1–M5)
    ├── OpenWebRxClient     (M6.1)
    ├── SpyServerClient     (M6.2)
    └── RtlTcpClient        (M6.3)
```

All providers deliver `int16 PCM` audio at a known sample rate into the
existing `AudioSampleQueue → Resampler → JitterBuffer` DSP pipeline.
The pipeline is provider-agnostic; only the network/decode layer changes.

### Provider Detection (for M5.1 station list)

Each station entry carries a `provider` field:

```json
{ "provider": "kiwisdr",   "host": "kphsdr.com",      "port": 8072 }
{ "provider": "openwebrx", "host": "sdr.example.com",  "port": 8073 }
{ "provider": "spyserver", "host": "spy.example.com",  "port": 5555 }
{ "provider": "rtltcp",    "host": "192.168.1.10",     "port": 1234 }
```

For KiwiSDR: detected by `/status` returning `ext_api`, `rx_chans`.
For OpenWebRX: detected by HTTP `/api/features` or WS greeting `{"type":"config"}`.
For SpyServer: native binary protocol; radiom uses a WS proxy bridge.
For RTL-TCP: native TCP binary; direct connection on port 1234.

---

## Step-by-step Implementation

### M6.1 — OpenWebRX Client

**Protocol summary** (from radiom `openwebrx/client.ts`, 896 lines):

- WS URL: `ws://<host>:<port>/ws/`
- Handshake:
  1. Server sends `{"type":"config", ...}` JSON with `profiles` list.
  2. Client sends `{"type":"selectprofile", "value": "<profile_id>"}`.
  3. Server sends `{"type":"dspconfig", "output_rate": N, ...}`.
  4. Audio frames: binary, IMA ADPCM with SYNC markers (4-byte `SYNC`
     re-anchor every N samples — **different** from KiwiSDR ADPCM, no SYNC).
  5. Tuning: `{"type":"setfreq", "params": {"offset": Hz, "base_frequency": Hz}}`.
  6. Mode: `{"type":"setmod", "params": {"mod": "usb", ...}}`.

- **Key difference from KiwiSDR:** OpenWebRX uses a **different ADPCM variant**
  with explicit SYNC markers and `syncCounter` re-synchronisation logic
  (radiom `OwrxImaAdpcmCodec`). Our existing `ImaAdpcmDecoder` must NOT be
  used for OpenWebRX frames without modification.

- **Audio rate:** server-driven via `dspconfig.output_rate`; typically 12000 Hz
  or a clean divisor of the AudioContext sample rate.

**Implementation tasks:**

- [ ] **M6.1a** `OpenWebRxClient` class wrapping IXWebSocket, JSON handshake,
  profile selection, ADPCM-with-SYNC decode.
  - Decoder: port `OwrxImaAdpcmCodec` from radiom (MIT licence, can inspire;
    write own implementation).
  - Interface: same `IReceiverClient` / callbacks as `KiwiClient`.
- [ ] **M6.1b** Station directory integration: detect OpenWebRX via HTTP
  `/api/features` endpoint.
- [ ] **M6.1c** UI: provider badge "OpenWebRX" in station list row.
- [ ] **M6.1d** Tests: unit test ADPCM-with-SYNC decoder; integration test
  against a mock OpenWebRX server.

**Licensing note:** radiom is MIT-licensed. We may use it as a reference
and inspiration but must write our own C++ implementation (no copy-paste of
TypeScript → C++). The ADPCM step/index tables are from the public Intel/DVI
ADPCM standard (no copyright concern).

---

### M6.2 — SpyServer Client

**Protocol summary** (from radiom `spyserver/client.ts`, 378 lines):

- radiom uses a **WS proxy bridge** (`/ws/spyserver/<host>:<port>`) that
  translates the native SpyServer binary protocol to JSON + binary WS frames.
  The client sends JSON commands (`{"t":"freq","hz":N}`), the bridge sends
  binary IQ or audio frames tagged with `TAG_IQ=0x00`, `TAG_AUDIO=0x01`,
  `TAG_FFT=0x04`.

- Native SpyServer protocol (without proxy): TCP binary, little-endian.
  Packet header: `[magic:4][cmd:4][body_size:4]`. Well-documented in the
  SDRSharp project.

- **For our VST plugin:** we connect directly to SpyServer via TCP (native
  protocol) without a proxy. This gives lower latency and avoids the proxy
  dependency.

- **Audio output:** raw IQ at the tuned centre frequency. Demodulation would
  need to be added to the DSP pipeline (not required for M6 initial; expose
  raw IQ and use AM/SSB demod from a later DSP milestone).

**Implementation tasks:**

- [ ] **M6.2a** `SpyServerClient` class: TCP connection, native binary
  protocol, IQ frame delivery.
  - Reference: SDRSharp SpyServer protocol spec + radiom proxy bridge.
  - IQ → audio demod: AM/USB/LSB using existing `ImaAdpcmDecoder` pipeline
    (replace ADPCM with direct PCM; demod in DSP layer).
- [ ] **M6.2b** Station entry type: `provider=spyserver`, `host`, `port`
  (default 5555).
- [ ] **M6.2c** Tests: mock TCP SpyServer; verify IQ frame parsing.

**Licensing note:** SpyServer protocol is documented publicly by AirSpy/SDRSharp
(no licence restriction on implementing the protocol).

---

### M6.3 — RTL-TCP Client

**Protocol summary** (from radiom `rtltcp/client.ts`):

- TCP binary, port 1234 (default). Server sends a `dongle_info` magic header
  (12 bytes: `RTL0` magic, tuner type, gain count). Client sends commands as
  5-byte big-endian packets: `[cmd:1][param:4]`.
- Audio: raw 8-bit IQ samples (uint8, centre = 127). Demodulation required.
- Very low latency; hardware is local (not public internet stations).

**Use case:** Local RTL-SDR dongle as audio source in the DAW. Not for the
public station directory (M5) — added as a "local device" option in the UI.

**Implementation tasks:**

- [ ] **M6.3a** `RtlTcpClient` class: TCP, dongle_info parsing, frequency/gain
  commands, uint8 IQ stream delivery.
- [ ] **M6.3b** UI: "Local RTL-TCP" connection type, host + port fields.
- [ ] **M6.3c** Tests: mock TCP RTL-TCP server; verify dongle_info + command.

---

### M6.4 — Provider-Agnostic Station Directory Update

Extends M5.1 to show OpenWebRX and SpyServer stations alongside KiwiSDR:

- [ ] **M6.4a** Station model gains `provider` field (KiwiSDR / OpenWebRX /
  SpyServer / RTL-TCP).
- [ ] **M6.4b** Station list fetcher gains OpenWebRX detection (HTTP
  `/api/features`).
- [ ] **M6.4c** Station row in UI shows provider badge/icon.
- [ ] **M6.4d** `PluginProcessor` routes `connectToStation` to the correct
  `IReceiverClient` based on `provider`.

---

## VibeSDR Analysis Notes

VibeSDR (React Native app, MIT-licensed) uses its own **VibeServer** backend
(a Raspberry Pi + Cloudflare Tunnel + custom WS server). The directory at
`vibeserver.vibesdr.net` is a Cloudflare D1-backed registry (15-min ping,
expires-at lease, hashed keys, Maidenhead grid locator). **Not compatible**
with KiwiSDR or OpenWebRX protocols — VibeServer speaks a proprietary JSON
protocol. Relevant lessons:

1. **Directory design:** Operator self-registers with a key; directory entries
   expire if the server stops pinging (no probe required). Good model for
   a future community directory (M6+).
2. **Address stability:** Cloudflare Tunnel gives a stable public URL even
   for home operators without a static IP. Our M5 `/status` approach is
   simpler but less resilient.
3. **No ext_api equivalent:** VibeServer does not distinguish browser-only vs.
   API-access — all VibeServers are API-capable by design.

---

## KiwiAngel Analysis Notes (SDRangel plugin, GPLv3)

KiwiAngel is the KiwiSDR input plugin for SDRangel. Key findings:

- **WS URL:** `ws://<host>:<port>/kiwi/<ms>/SND` (path without `/ws/` prefix).
  Our client uses `/ws/kiwi/<ms>/SND` — both work on modern firmware.
- **Handshake:** `SET auth t=kiwi p=#` on connect; then triggered on
  `audio_init=0 audio_rate=12000` → sends `SET AR OK in=12000 out=48000` +
  `SERVER DE CLIENT KiwiAngel SND` (client identification, optional) + AGC + MOD.
- **IQ mode:** uses `SET mod=iq low_cut=-5980 high_cut=5980`. SND frame
  dataOffset=20 for IQ (vs. our 10-byte for mono ADPCM).
- **Keepalive:** 5 s timer, TEXT frames.
- **`SERVER DE CLIENT <name> SND`:** a real optional protocol message for
  client identification. Our FIX-41 correctly removed the bogus
  `SERVER DE CLIENT openwebrx.js SND` line.

---

## radiom Analysis Notes (Web app, MIT-licensed)

radiom is the most complete multi-provider reference. Key findings for M6:

1. **Provider abstraction:** single `KiwiHandlers`-shaped callback interface
   used by all four providers. Our `IReceiverClient` design mirrors this.
2. **KiwiSDR bot-detector (v1.817+):** radiom's `client.ts` documents that
   v1.817+ firmware kicks TEXT-frame clients and requires the exact canonical
   sequence. However, our probe shows the kick is **load-dependent** (not
   purely frame-type dependent): TEXT+canonical survived 45s when the server
   had spare capacity. Binary frames did not avoid the kick either. Our FIX-41
   canonical sequence is the correct fix.
3. **OpenWebRX ADPCM:** Different from KiwiSDR — uses SYNC markers + syncCounter.
   Separate decoder required.
4. **SpyServer proxy bridge:** radiom uses a server-side WS bridge; we bypass
   with a direct TCP connection.
5. **Station list (radiom `stations.ts`):** hand-picked SW broadcast/utility
   stations (WWV, CHU, BBC, etc.) with kHz/mode/name/tol. Useful seed for our
   `doc/station-list.md` (already incorporated).

---

## Checklist Items to Add

These items should be added to `doc/checklist.md` under a new **Milestone M6**
section when M5 is complete:

```
## Milestone M6 - Multi-Provider Support

- [ ] M6.1a  OpenWebRxClient (IXWebSocket, JSON handshake, ADPCM-with-SYNC)
- [ ] M6.1b  OpenWebRX station detection (HTTP /api/features)
- [ ] M6.1c  UI: OpenWebRX provider badge
- [ ] M6.1d  Tests: OpenWebRX ADPCM decoder + mock server integration
- [ ] M6.2a  SpyServerClient (TCP, native binary protocol, IQ frames)
- [ ] M6.2b  Station entry type: provider=spyserver
- [ ] M6.2c  Tests: mock SpyServer
- [ ] M6.3a  RtlTcpClient (TCP, uint8 IQ, dongle_info)
- [ ] M6.3b  UI: Local RTL-TCP connection type
- [ ] M6.3c  Tests: mock RTL-TCP server
- [ ] M6.4a  Station model gains provider field
- [ ] M6.4b  Station list fetcher: OpenWebRX detection
- [ ] M6.4c  Station row: provider badge/icon
- [ ] M6.4d  PluginProcessor: routes to correct IReceiverClient by provider
```

---

_Created: 2026-08-27. References: radiom (MIT), VibeSDR (MIT), KiwiAngel (GPLv3),
SDRangel, SDRSharp SpyServer protocol spec._
