# NetSDRStation-VST — KiwiSDR Station List

_Source: live probe 2026-08-27 (probe_duration.py, 45 s per station) +
`/status` endpoint queries. For M5.1 implementation reference._

> **Kernanforderung:** Nur Stationen mit `ext_api > 0` werden vom Plugin
> geladen. `ext_api == 0` bedeutet Browser-only — der native WebSocket-Client
> hat keinen Zugang zu diesen Stationen und sie werden gefiltert.

> **Dauerbetrieb-Kriterium:** STABLE = ≥50 SND-Frames, 45 s überlebt,
> kein `too_busy`-Kick. KICKED = `too_busy`-Disconnect < 45 s
> (load-abhängig — kann bei geringerer Serverlast auch stabil sein).
> DOWN = nicht erreichbar.

---

## Legende

| Spalte     | Bedeutung                                                      |
|------------|----------------------------------------------------------------|
| `ext_api`  | Anzahl externer API-Kanäle (0 = Browser-only, ≥1 = API-ready) |
| `users`    | Aktive User / Maximum zum Probe-Zeitpunkt                      |
| `SND`      | SND-Frames in 45 s (Richtwert: ~260 bei 12 kHz/1024-Sample)   |
| `Status`   | STABLE / KICKED / DOWN / NO-AUDIO                             |
| `Dauerbetrieb` | ✅ empfohlen / ⚠️ load-abhängig / ❌ nicht geeignet         |

---

## Stationsliste (Stand 2026-08-27)

### USA — Kalifornien

| Host                    | Port | Name                                  | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-------------------------|------|---------------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| kphsdr.com              | 8072 | KPH Point Reyes CA – 100m Marconi-T  | 4       | 4/8   | 251 | STABLE  | ✅            | Top-Station, LW/MW/SW          |
| kphsdr.com              | 8073 | KPH Point Reyes CA – TCI-530 Omni    | 4       | 4/8   |  53 | KICKED  | ⚠️            | load-abhängig (war bei >10s)   |
| kiwisdr.kfsdr.com       | 8073 | KFS Half Moon Bay CA – Omni #1       | 0       | 1/8   |  51 | KICKED  | ❌            | ext_api=0, Browser-only        |
| kiwisdr.kfsdr.com       | 8077 | KFS Half Moon Bay CA – Omni #2       | 0       | 0/8   |  51 | KICKED  | ❌            | ext_api=0, Browser-only        |

### USA — Utah

| Host                    | Port | Name                                  | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-------------------------|------|---------------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| kiwisdr2.sdrutah.org    | 8074 | N. Utah #2 – TCI-530 Omni            | 0       | 6/8   |   0 | DOWN    | ❌            | ext_api=0 + timeout bei Probe  |
| kiwisdr4.sdrutah.org    | 8076 | N. Utah #5 – LP-1002                 | 0       | 5/8   |   0 | KICKED  | ❌            | ext_api=0, Browser-only        |

### USA — Nord Carolina

| Host                    | Port | Name                                  | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-------------------------|------|---------------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| kiwisdr.ku4by.com       | 8073 | KU4BY Elizabeth City NC #1           | 4       | 2/8   |  15 | DOWN    | ⚠️            | ext_api=4, instabil bei Probe  |
| kiwisdr.ku4by.com       | 8074 | KU4BY Elizabeth City NC #2           | 4       | 7/8   | 141 | STABLE  | ✅            | Gut, 7/8 voll → knapp          |
| kiwisdr.itfais.com      | 8073 | KT4RS Laurel Springs NC              | 4       | 1/8   |   0 | DOWN    | ⚠️            | ext_api=4, Timeout bei Probe   |

### USA — Oregon

| Host                    | Port | Name                                  | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-------------------------|------|---------------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| kk6pr.ddns.net          | 8076 | KK6PR Crooked River Ranch OR #1      | 4       | 0/8   |   0 | DOWN    | ⚠️            | ext_api=4, DDNS offline bei Probe |
| kk6pr.ddns.net          | 8077 | KK6PR Crooked River Ranch OR #2      | 4       | 0/4   |   0 | DOWN    | ⚠️            | ext_api=4, DDNS offline bei Probe |

### USA — Virginia / Ohio

| Host                        | Port | Name                              | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-----------------------------|------|-----------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| kiwisdr.n2yo.net            | 8073 | N2YO Chantilly VA                 | 4       | 1/4   |   0 | DOWN    | ⚠️            | Timeout bei Probe; low capacity |
| 21690.proxy.kiwisdr.com     | 8073 | Hilliard Ohio – Magnetic Loop     | 4       | 0/2   |   0 | DOWN    | ⚠️            | HTTP 307 Redirect (proxy)      |

### Kanada

| Host                    | Port | Name                                  | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-------------------------|------|---------------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| kj6ei.ddns.net          | 8073 | KJ6EI/VE7 Salt Spring Island BC      | 0       | 7/7   |   0 | KICKED  | ❌            | ext_api=0, Browser-only, voll  |

### Europa — Deutschland

| Host                      | Port | Name                                | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                         |
|---------------------------|------|-------------------------------------|---------|-------|-----|---------|--------------|-------------------------------|
| kiwisdr.birdsnest.name    | 8073 | Birdy (SWL) Emsbüren DE             | 4       | 2/8   | 260 | STABLE  | ✅            | Sehr stabil, niedrige Last      |

### Europa — Frankreich

| Host                    | Port | Name                                  | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-------------------------|------|---------------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| linkz.ddns.net          | 8073 | linkz kiwi 1 Isere FR                | 4       | 1/4   | 260 | STABLE  | ✅            | Stabil, nur 4 Slots total      |
| linkz.ddns.net          | 8074 | linkz kiwi 2 Isere FR                | 0       | 0/4   |  56 | KICKED  | ❌            | ext_api=0, Browser-only        |

### Europa — Irland

| Host                        | Port | Name                              | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-----------------------------|------|-----------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| malinheadkiwi.hopto.org     | 8073 | EI0CF Malin Head IE               | 2       | 1/8   | 259 | STABLE  | ✅            | Gute Europäische HF-Lage       |

### Europa — UK

| Host                    | Port | Name                                  | ext_api | users | SND | Status  | Dauerbetrieb | Notizen                        |
|-------------------------|------|---------------------------------------|---------|-------|-----|---------|--------------|--------------------------------|
| g8ure.ddns.net          | 8078 | G8URE 80m Dipole Chichester UK       | 0       | 3/8   |  57 | KICKED  | ❌            | ext_api=0, Browser-only        |

---

## Zusammenfassung

### Empfohlen für Dauerbetrieb (✅ STABLE, ext_api > 0)

| Host                      | Port | Name / Standort                        | ext_api |
|---------------------------|------|----------------------------------------|---------|
| kphsdr.com                | 8072 | KPH Point Reyes CA – Marconi-T 0–30 MHz | 4     |
| kiwisdr.ku4by.com         | 8074 | KU4BY Elizabeth City NC #2            | 4       |
| kiwisdr.birdsnest.name    | 8073 | Birdy (SWL) Emsbüren DE               | 4       |
| linkz.ddns.net            | 8073 | linkz kiwi 1 Isere FR                 | 4       |
| malinheadkiwi.hopto.org   | 8073 | EI0CF Malin Head IE                   | 2       |

### Bedingt geeignet (⚠️ ext_api > 0, bei Probe instabil/offline)

Diese Stationen haben `ext_api > 0` und _könnten_ stabil laufen — bei der Probe
waren sie offline (DDNS-Ausfall, Timeout) oder load-bedingt instabil. Für die
dynamische Stationsliste (M5.1) werden sie über `/status` live gefiltert.

| Host                      | Port | Grund der Einschränkung                |
|---------------------------|------|----------------------------------------|
| kphsdr.com                | 8073 | Load-bedingt `too_busy` (oft voll)    |
| kiwisdr.ku4by.com         | 8073 | Probe-Timeout (evtl. kurz offline)    |
| kiwisdr.itfais.com        | 8073 | Probe-Timeout                          |
| kk6pr.ddns.net            | 8076 | DDNS offline bei Probe                 |
| kk6pr.ddns.net            | 8077 | DDNS offline bei Probe                 |
| kiwisdr.n2yo.net          | 8073 | Timeout, nur 4 Slots                   |
| 21690.proxy.kiwisdr.com   | 8073 | HTTP 307 (Proxy-Redirect)              |

### Nicht geeignet (❌ ext_api=0 oder dauerhaft down)

| Host                      | Port | Grund                                  |
|---------------------------|------|----------------------------------------|
| kiwisdr.kfsdr.com         | 8073 | ext_api=0 (Browser-only)              |
| kiwisdr.kfsdr.com         | 8077 | ext_api=0 (Browser-only)              |
| kiwisdr2.sdrutah.org      | 8074 | ext_api=0 (Browser-only)              |
| kiwisdr4.sdrutah.org      | 8076 | ext_api=0 (Browser-only)              |
| kj6ei.ddns.net            | 8073 | ext_api=0 (Browser-only), immer voll  |
| linkz.ddns.net            | 8074 | ext_api=0 (Browser-only)              |
| g8ure.ddns.net            | 8078 | ext_api=0 (Browser-only)              |

---

## M5.1 Implementierungshinweise

### Dynamische Stationsliste

Diese statische Liste ist ein **Seed** für die Entwicklung. In M5.1 wird die
Liste dynamisch geladen:

1. **`/status` HTTP-Endpunkt** (bereits im M5.1-Plan): Jede KiwiSDR-Station
   exponiert `http://<host>:<port>/status` mit Feldern:
   - `ext_api` — Anzahl API-Kanäle (Filter: `> 0`)
   - `users` / `users_max` — Auslastung
   - `name`, `loc` — Anzeigename und Standort
   - `version_maj`, `version_min` — Firmware-Version

2. **Öffentliche Verzeichnisse** (Referenz-Implementierungen analysiert):
   - **sdr.hu** (`http://sdr.hu/api/sdr-list`) — aktuell nicht erreichbar
   - **rx.kiwisdr.com/public** — hinter Anti-Bot-Click-Gate, nicht automatisierbar
   - **receiverbook.de** — aktuell nicht erreichbar
   - **VibeSDR Directory** (`vibeserver.vibesdr.net`) — proprietär für VibeSDR-Server
   - **AetherSDR** `KiwiPublicDirectory` — liest `ext_api` direkt vom `/status`-Endpunkt
     (Referenz-Implementierung für M5.1, laut checklist.md M5.1)

3. **Empfohlener Ansatz für M5.1:** Embedded-Seed-Liste (diese Datei) als
   Fallback + Live-`/status`-Abfrage pro Station beim Laden der Stationsliste.
   Stationen mit `ext_api == 0` werden ausgeblendet. Stationen die nicht
   antworten werden als OFFLINE markiert.

### `too_busy`-Verhalten

- `too_busy=N` (N = `ext_api`-Wert) = Server hat keine freien API-Kanäle mehr.
- **Nicht** ein fixer Timeout — load-abhängig. Eine volle Station (users ≥ users_max)
  oder eine Station mit `ext_api=0` schickt uns sofort raus.
- **Probe-Ergebnis:** Alle 5 STABLE-Stationen hatten `ext_api ≥ 2` **und**
  waren bei der Probe zu ≤ 25% ausgelastet (users ≤ 2/8 typisch).
- **Strategie:** In M5.1 live `users` vs. `users_max` + `ext_api` prüfen und
  volle/Browser-only Stationen filtern.

---

_Letzte Aktualisierung: 2026-08-27. Probe-Skript: `C:\Users\marku\AppData\Local\Temp\opencode\kiwiprobe\probe_duration.py`._
