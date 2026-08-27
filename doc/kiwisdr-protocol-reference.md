# KiwiSDR Protocol Reference (aus Referenz-Clients abgeleitet)

_Stand 2026-08-27. Erstellt aus der Analyse von:_
- `jks-prv/KiwiSDR` (Server-Quellcode, master) — `rx/rx_sound_cmd.h/.cpp`,
  `rx/rx_server.cpp`, `rx/rx_cmd.cpp`, `rx/rx_util.cpp`
- `hcab14/gr-kiwisdr` (GNU-Radio-C++-Client, funktionierend)
- `mcogoni/supersdr` (Python, Fork von kiwiclient)
- `jks-prv/kiwiclient` (offizielle Python-Referenz, `kiwi/client.py`)
- `thepacket/radiom` (Web-App, MIT) — `src/kiwi/client.ts` (bot-detector-Analyse, PCAP)
- `Vort/KiwiAngel` (SDRangel-Plugin, GPLv3) — `kiwisdrworker.cpp` (URL-Pfad, IQ-Modus)

_Erweitert 2026-08-27: radiom- und KiwiAngel-Analyse, Dauerbetrieb-Stationsprobe._

_Zweck: kanonisches Protokollwissen für `source/network/kiwi_client.cpp`.
Gekoppelt an `doc/architecture.md` §6 (Protocol / Handshake)._

---

## 1. Verbindungsaufbau (WebSocket-Pfad)

- Host:Port, Pfad `/<ts>/SND` (ältere Klienten) oder `/ws/kiwi/<ts>/SND`
  (Kiwi-UI). **Beides handshakt 101; der UI-Pfad ist der kanonische.** `ts` ist
  ein (annähernd) eindeutiger Wert (Sekunden oder Zufall).
- WebSocket-Standard-Handshake. **Kein** `Origin`-Header nötig, **kein**
  `SET options=1` nötig (nur bei `--nolocal` für lokale Verbindungen).

## 2. Handshake-Sequenz (kanonisch, alle Referenzen)

**Phase 1 (nach OPEN):**
```
SET auth t=kiwi p=
SET ident_user=<name>        # optional, Referenzen setzen meist einen Namen
```

**Phase 2 (getrennt nach zwei Server-Nachrichten):**

Auf `audio_rate=<rate>`:
```
SET AR OK in=<rate> out=44100     # out=44100 = Server resampled
```

Auf `sample_rate=<rate>`:
```
SET squelch=0 max=0
SET genattn=0
SET gen=0 mix=0                  # (kiwiclient: mix=-1)
SET mod=<mode> low_cut=<lc> high_cut=<hc> freq=<freq.kHz>
SET agc=1 hang=0 thresh=-100 slope=6 decay=1000 manGain=50
SET keepalive
```

> **Wichtig:** Der Server sendet `sample_rate=` **vor** `audio_rate=`. Phase 2
> darf deshalb NICHT auf den ersten Trigger gelatcht werden, sonst wird
> `SET AR OK` nie gesendet (das war der Root-Cause von FIX-41).

> **Eigene Abweichung (bewusst):** unser Client sendet `SET AR OK in=<rate>
> out=<rate>` (kein Server-Resampling) und alles gebündelt auf `audio_rate=`.
> Funktioniert, da der Server nur `arate_out != 0` prüft (siehe §3).

## 3. Server-seitiger Audio-Gate: `CMD_SND_ALL`

`rx/rx_sound_cmd.h`:

```c
#define CMD_FREQ       0x01
#define CMD_MODE       0x02
#define CMD_PASSBAND   0x04
#define CMD_AGC        0x08
#define CMD_AR_OK      0x10
#define CMD_SND_ALL    (CMD_FREQ | CMD_MODE | CMD_PASSBAND | CMD_AGC | CMD_AR_OK)
```

- Der Server verarbeitet **kein** Audio, bis `s->cmd_recv == CMD_SND_ALL`
  (`rx/rx_sound.cpp`, `s->cmd_recv != CMD_SND_ALL` → `TaskSleepMsec(100)`).
- Bits werden gesetzt in `rx_sound_cmd.cpp`:
  - `CMD_FREQ` — `SET mod=... freq=` (nur wenn sich `freq` ändert).
  - `CMD_MODE` — `SET mod=<mode> ...` (bei `mode != "x"`).
  - `CMD_PASSBAND` — `SET mod=... low_cut=<≠0> high_cut=<≠0> ...` (nur wenn
    `low_cut`/`high_cut` ≠ 0 und sich geändert haben).
  - `CMD_AGC` — `SET agc=...` (sscanf mit 6 Feldern).
  - `CMD_AR_OK` — `SET AR OK in=<n> out=<≠0>` (nur wenn `arate_out != 0`).

**Konsequenz für uns:** `SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000`
+ `SET agc=...` + `SET AR OK in=... out=<≠0>` setzt alle 5 Bits. Wird eines
ausgelassen (z. B. `low_cut=0 high_cut=0` → kein `CMD_PASSBAND`), bleibt das
Gate zu und der Server kickt (siehe §4).

## 4. Kick-Mechanismen (Server)

`rx/rx_sound.cpp` (Sound-Task-Loop):

```c
bool keepalive_expired = (conn->keep_alive > (conn->auth? KEEPALIVE_SEC : KEEPALIVE_SEC_NO_AUTH));
bool connection_hang = (conn->keepalive_count > 4 && s->cmd_recv != CMD_SND_ALL);
if (keepalive_expired || connection_hang || conn->inactivity_timeout || conn->kick) {
    // remove connection
}
```

- `KEEPALIVE_SEC = 60`, `KEEPALIVE_SEC_NO_AUTH = 20`.
- `connection_hang`: nach **5 Keepalives** ohne vollständige SND-Kommandos.
- `conn->kick`: gesetzt durch `OPTION_DENY_APP_FINGERPRINT_CONN`
  (`rx_util.cpp` LOG_ARRIVED `ident_user=="kiwi_nc.py"` in HFDL-Bändern;
  `rx_cmd.cpp` unbekannte `SET MARKER`-Variante).

**`MSG too_busy=<n>`** wird gesendet bei:
- `rx_server.cpp`: „zu viele Verbindungen/Kanäle" (`n = rx_chans`).
- `rx_cmd.cpp` (Fingerprint-Deny): `n = ext_api_nchans`.

## 5. ~11-s-Kick auf kphsdr.com:8073 (`too_busy=4`) — Server-Policy

**Empirisch verifiziert (2026-08-27):** kphsdr.com:8073 kickt **jeden**
External-API-Client nach ~11 s mit `MSG too_busy=4` + CLOSE 1005. Getestet mit:
unserem C++-Client, einer kiwiclient-nachgebauten Python-Probe **und** der
exakten gr-kiwisdr-Handshake-Variante (`ident_user`+`geo`+`compression=0`) —
alle werden nach ~11 s gekickt.

- `4 = ext_api_nchans` (Anzahl External-API-Kanäle dieser Station).
- **Das ist eine Server-seitige Zeit-/Kapazitäts-Policy, KEIN Client-Bug.** Der
  Client kann sie nicht verhindern. Die offizielle Referenz wirft bei `too_busy`
  eine Exception (gibt auf); gr-kiwisdr rekombiniert nicht explizit.
- **Unsere Antwort:** automatischer Reconnect (bereits implementiert). Das führt
  zu einem ~1 s-Audio-Gap alle ~11 s auf dieser Station. Für einen stabilen
  Dauerbetrieb eine Station mit längerem Limit wählen (z. B. g8ure.ddns.net:8078,
  die diese Policy nicht hat).

## 6. SND-Frame-Format (Audio)

`kiwi/client.py` `_process_aud`:

```
bytes[0]      flags  (0x02 ADC_OVFL, 0x08 STEREO/IQ, 0x10 COMPRESSED, 0x80 LITTLE_ENDIAN)
bytes[1..4]   seq    (u32 LE)
bytes[5..6]   smeter (u16 BE)
bytes[7..]    ADPCM (compressed) oder int16 PCM (uncompressed)
```

Gesamt-Header = 3 (`"SND"`) + 1 (flags) + 4 (seq) + 2 (smeter) = **10 Bytes**
(vom WebSocket-Binary-Frame mit `"SND"`-Präfix aus gezählt). Deckt sich mit
`kSndHeaderSize = 10` in `plugin_processor.cpp`.

## 7. Keepalive-Regeln

- Referenzen senden `SET keepalive` **nach** `sample_rate`/`audio_rate` (Phase 2)
  und dann im SND-Empfangs-Callback (1 Hz, throttled) bzw. „alle 10 Frames"
  (gr-kiwisdr).
- **NIE vor Abschluss des Handshakes** — frühes Keepalive versetzt die
  Verbindung bei Firmware v1.900 in den „monitor"-Modus (Root-Cause FIX-42).
- Server zählt Keepalives (`keepalive_count`); `connection_hang` feuert nach 5.

## 8. Warum die Mock-Server-Tests die Realität nicht abbilden

Der Test-Mock (`tests/network/kiwi_client_tests.cpp` etc.) sendet nur
`MSG audio_rate=12000` auf `SET auth` und prüft die Frame-Reihenfolge. Er
modelliert **nicht**:

1. `sample_rate=` vor `audio_rate=` (Reihenfolge) → FIX-41 nicht abgedeckt.
2. `CMD_SND_ALL`-Gate / `connection_hang`-Kick.
3. `too_busy`-/Zeitlimit-Kick (server-seitige Policy).
4. „monitor"-Modus bei frühem Keepalive.
5. Echtes Timing (Keepalive vor Handshake).

**Folge:** „Tests grün" ≠ „läuft gegen echten Server". Für echte
Protokoll-Sicherheit sind Live-Proben gegen reale Kiwis nötig (siehe
`C:\Users\marku\AppData\Local\Temp\opencode\kiwiprobe\`).

---

## 9. v1.817+ Bot-Detector (radiom PCAP-Analyse)

radiom `client.ts` dokumentiert einen Bot-Detector in Firmware v1.817+:

- **TEXT-Frame-Kick:** Klienten, die `SET`-Befehle als WebSocket-TEXT-Frames
  senden, werden nach ~10 s gekickt. Der Kick ist aber **load-abhängig**, nicht
  deterministisch — bei geringer Serverlast können auch TEXT-Frame-Klienten 45 s
  überleben (empirisch belegt: probe_radiom.py, TEXT-Modus, 30.1 s bei niedriger
  Last). BINARY-Frames allein vermeiden den Kick NICHT (BINARY + radiom-Sequenz
  gegen kphsdr.com:8073 → kick bei t=11.2 s).
- **Frühes Keepalive:** `SET keepalive` vor `audio_rate=` versetzt v1.900-Firmware
  in den „monitor"-Modus (Root-Cause FIX-42, bereits behoben).
- **`SET auth` als erste Message:** Alles vor `SET auth` gilt als bad-password-
  Versuch (nach 5 Versuchen: IP-Sperre). Unser `SET options=1` vor `SET auth`
  ist unkritisch, weil `SET options` keinen auth-Versuch zählt — aber die
  radiom-Sequenz schickt options+auth als Paar (options zuerst, wie kiwiclient).
- **Simultaner Doppel-Socket (SND+W/F):** radiom öffnet W/F 150 ms nach SND
  (simultane Öffnung sieht „robotisch" aus für den Bot-Detector). Wir haben
  keinen W/F-Socket → kein Problem.
- **HTTP-Session-Footprint:** v1.817+ prüft, ob vor dem WS-Connect typische
  Browser-Assets abgerufen wurden (`/`, `kiwi_js_load.js`, `config.js`,
  `sprintf.js`). Klienten ohne diesen Footprint bekommen `audio_init=0` für
  immer + 10-s-Kick. **Unser Client** umgeht das durch das korrekte `SET AR OK`
  (nicht `audio_init=0` forever), weil der CMD_SND_ALL-Gate bei uns vollständig
  gesetzt wird.

**Fazit:** Der ~11-s-Kick auf kphsdr.com:8073 ist eine **Server-Policy**
(`ext_api_nchans`-basiertes Kapazitätsmanagement), keine Bot-Detector-Reaktion.
Unser kanonischer Handshake (FIX-41/42) ist korrekt; der einzige Weg zu
stabilem Dauerbetrieb ist eine Station mit `ext_api > 0` und genug freien
Kanälen (`users < users_max`).

---

## 10. `ext_api` — API-Kanal-Kontingent

Jede KiwiSDR-Station veröffentlicht `ext_api` in `/status`:

```
ext_api=4   → 4 Kanäle für externe API-Klienten (unser Plugin)
ext_api=0   → Browser-only, externer API-Zugang gesperrt
```

`too_busy=N` wird gesendet wenn `N = ext_api` und alle API-Kanäle belegt sind.
**Filter-Anforderung (M5.1):** Nur Stationen mit `ext_api > 0` laden.

**Probe-Ergebnis (2026-08-27, 45 s, 19 Stationen):**
- STABLE (45 s, kein kick): 5 Stationen — alle mit `ext_api ≥ 2`, `users ≤ 7/8`.
- KICKED: 6 Stationen — davon 5 mit `ext_api=0` (Browser-only); 1 mit
  `ext_api=4` aber voll (kphsdr:8073, 4/8 User).
- DOWN: 8 Stationen — DDNS-Ausfall, Timeout, HTTP-Redirect.

Details: `doc/station-list.md`.

---

## 11. KiwiAngel: WS-URL-Pfad und IQ-Modus

KiwiAngel (`kiwisdrworker.cpp`) verwendet:
- URL: `ws://<host>:<port>/kiwi/<ms>/SND` (ohne `/ws/`-Präfix).
- Unser Client: `/ws/kiwi/<ms>/SND` — beide Varianten funktionieren.
- IQ-Modus (`SET mod=iq`): SND-Frame-Daten beginnen bei Byte 20 (nicht 10 wie
  bei Mono-ADPCM). IQ = Stereo, kein ADPCM — separate Decodierpipeline nötig.
- `SERVER DE CLIENT <name> SND`: optionale Client-Identifikationsmessage.
  Unser FIX-41 entfernte `SERVER DE CLIENT openwebrx.js SND` korrekt (war
  ein Fake-Fingerprint); ein eigenes `SERVER DE CLIENT NetSDRStation SND`
  könnte wieder eingefügt werden, ist aber nicht zwingend.
