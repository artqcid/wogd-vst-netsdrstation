# M5 Implementation Plan — Station Selection Tab

_Cross-reference: `doc/checklist.md` M5 · `doc/ui-architecture.md` §6 ·
`doc/architecture.md` §5 · `doc/M4-implementation-plan.md`_

## Overview

M5 adds a tab structure to the plugin UI:

- **Tab 1 "SDR Stations"** — scrollable station directory; click a station to
  connect.
- **Tab 2 "KIWI UI"** — the M4 KiwiSDR receiver interface.

Default state: **no station loaded**. Tab 2 shows only the message
"please select station first" and no receiver controls. After the user
selects a station, Tab 2 shows the full M4 interface.

**Prerequisite:** M4 must be complete before M5 work begins, since M5 wraps
the M4 UI inside Tab 2.

---

## Chronological implementation order

### Step 1 — M5.2: Tab layout + routing (implement FIRST)

The tab bar and routing are the structural foundation. All other M5 steps plug
into it.

#### 1a. Tab bar component

`KTabBar.vue`:

```html
<template>
  <div class="kiwi-tab-bar">
    <button
      v-for="tab in tabs"
      :key="tab.id"
      :class="['kiwi-tab', { active: currentTab === tab.id }]"
      @click="currentTab = tab.id"
    >{{ tab.label }}</button>
  </div>
  <div class="kiwi-tab-content">
    <slot :name="currentTab" />
  </div>
</template>
```

Styling: active tab = green bottom border (`border-bottom: 2px solid #4CAF50`),
inactive = grey. Same visual language as KiwiSDR's navbar (`w3-navbar a`
pattern).

#### 1b. App.vue tab structure

```html
<KTabBar :tabs="[{id:'stations', label:'SDR Stations'},
                  {id:'kiwi',     label:'KIWI UI'}]">
  <template #stations>
    <StationsView />
  </template>
  <template #kiwi>
    <KiwiView />   <!-- the M4 PluginView, renamed -->
  </template>
</KTabBar>
```

#### 1c. State: `connectedStation`

Extend the Pinia store (`kiwiStore.ts`):

```ts
connectedStation: null as StationInfo | null,
```

`KiwiView` (Tab 2) renders the empty state when `store.connectedStation === null`,
and the full M4 UI once it is set.

#### 1d. Resize with tabs

The `kiwi-layout` grid (M4.1) gets an extra top row for the tab bar:

```css
.kiwi-layout {
  grid-template-rows: auto auto 1fr auto;  /* tab-bar / header / main / status */
}
```

The tab bar is always visible at the top; the content area below it is the
full-width/height panel.

**Files:**
- `ui/src/components/KTabBar.vue` (new)
- `ui/src/App.vue` (add tab structure)
- `ui/src/views/StationsView.vue` (new, empty for now)
- `ui/src/views/KiwiView.vue` (rename from `PluginView.vue` or create as wrapper)
- `ui/src/store/kiwiStore.ts` (add `connectedStation`)

**Tests:**
- Vitest: clicking Tab 1 renders `StationsView`; clicking Tab 2 renders `KiwiView`.
- Vitest: Tab 2 shows the empty state when `connectedStation === null`.
- Vitest: Tab 2 shows the full UI when `connectedStation !== null`.

---

### Step 2 — M5.5: Empty state on Tab 2

Before any station is fetched or selected, Tab 2 must show a clear message.

#### Implementation

In `KiwiView.vue`:

```html
<template>
  <div v-if="!store.connectedStation" class="kiwi-empty-state">
    <div class="kiwi-empty-icon">📡</div>
    <p>Please select a station first.</p>
    <button class="kiwi-btn-primary" @click="emit('go-to-stations')">
      Open station list
    </button>
  </div>
  <div v-else>
    <!-- Full M4 receiver UI -->
    ...
  </div>
</template>
```

The "Open station list" button switches to Tab 1 via the tab store or a parent
event. The empty-state design matches the KiwiSDR dark theme.

> **Note:** do not render the frequency input, mode selector, waterfall etc.
> until `connectedStation` is set. This prevents the UI from sending
> `SET auth` or `SET mod/freq` commands to a null connection.

**Files:**
- `ui/src/views/KiwiView.vue`

**Tests:**
- Vitest: renders empty state when `connectedStation === null`.
- Vitest: "Open station list" emits the correct navigation event.
- Vitest: renders the full UI (mock M4 components) when `connectedStation` is set.

---

### Step 3 — M5.1: Station directory fetch

The public KiwiSDR station directory is available at:

```
https://www.rx-888.com/api/rx/list   (JSON, maintained by Rx-888 community)
https://kiwisdr.com/public/           (HTML, older; less suitable for parsing)
```

> **Confirmed endpoint (use this):**
> `GET https://www.rx-888.com/api/rx/list` returns a JSON array of station
> objects. Each station has (at minimum): `name`, `url` (host:port), `sdrHU`
> (location), `users`, `usersMax`, `snr`, `minFreq`, `maxFreq`, `online`.
>
> **Alternative endpoints:**
> - `https://sdr.hu/api.php?do=getSdrList` (older, same community data)
> - Fetch directly from `kiwisdr.com/public/` and parse the HTML (fragile).
> - KiwiSDR server-side API: each station serves `/status` and `/users` JSON
>   endpoints — can be queried per-station for live data.
>
> Recommendation: use `rx-888.com/api/rx/list` as primary, with a fallback
> to `sdr.hu`.

#### 3a. Where to fetch — UI vs. C++

| Option | Pros | Cons |
|--------|------|------|
| **Fetch in Vue (recommended)** | Simple `fetch()` in the browser; no C++ change; cross-origin handled by WebView2 CORS policy. | Requires internet access from the WebView2 context. |
| Fetch in C++ (network thread) | No CORS concerns; can cache aggressively. | More complex; must push JSON to UI via bridge. |

**Recommended:** fetch in Vue using the browser `fetch()` API. WebView2
allows outbound HTTP by default (no CORS restriction for `fetch()` from
`file://` context in WebView2 — unlike Chrome's strict CORS for `file://`
origins). Verify in practice; if CORS blocks the request, proxy via C++.

#### 3b. Station model

```ts
// ui/src/models/station.ts
export interface StationInfo {
  name: string;
  url: string;       // host:port, e.g. "g8ure.ddns.net:8078"
  location: string;
  users: number;
  usersMax: number;
  snr: number;       // dB
  minFreqKhz: number;
  maxFreqKhz: number;
  online: boolean;
}
```

#### 3c. Station service

```ts
// ui/src/services/stationService.ts
const API_URL = 'https://www.rx-888.com/api/rx/list';
const FALLBACK = 'https://sdr.hu/api.php?do=getSdrList';

export async function fetchStations(): Promise<StationInfo[]> {
  const resp = await fetch(API_URL);
  if (!resp.ok) throw new Error(`Station list fetch failed: ${resp.status}`);
  const data = await resp.json();
  return data.map(mapRx888ToStation);
}
```

Normalize the raw API response to `StationInfo` in `mapRx888ToStation`.

#### 3d. Store integration

In `kiwiStore.ts`:

```ts
stations: [] as StationInfo[],
stationsFetched: false,
stationsLoading: false,
stationsError: '',
```

Action `fetchStations()` calls `stationService.fetchStations()`, updates the
store.

**Files:**
- `ui/src/models/station.ts` (new)
- `ui/src/services/stationService.ts` (new)
- `ui/src/store/kiwiStore.ts` (add station list state + action)

**Tests:**
- Integration: mock `fetch` returns a sample API response; `fetchStations()`
  returns normalized `StationInfo[]`.
- Integration: malformed/empty response → error stored in `stationsError`.
- Unit: `mapRx888ToStation` correctly maps every field.

---

### Step 4 — M5.3: Station list (scrollable)

#### Layout

```
┌── SDR Stations ─────────────────────────────────────┐
│ [Search: ________________]  [Refresh]  [Sort: SNR ▾] │
├─────────────────────────────────────────────────────┤
│ ● G8URE (UK)   14–30 MHz   SNR 35 dB   0/4 users    │ ← online
│ ● KPH (USA)    0–30 MHz    SNR 28 dB   2/8 users    │
│ ○ ZL1-SDR      0–30 MHz    —           offline       │ ← offline
│ ...                                                  │
│ (scroll)                                             │
└─────────────────────────────────────────────────────┘
```

#### Implementation

`StationsView.vue`:

- On mount: call `store.fetchStations()` (only once; cached in store).
- Show `<div class="kiwi-station-list">` with a `v-for` over
  `store.stations` (filtered + sorted).
- Each row: `StationRow.vue` showing name, location, SNR badge, user count,
  online indicator (green dot vs. grey dot), frequency range.
- Search input: filters `store.stations` by name or location (client-side,
  no extra API call).
- Sort control: sort by SNR (default), users, name.
- Refresh button: re-fetches the station list.

#### Virtualization

If the station list exceeds ~200 entries, a plain `v-for` creates performance
issues. Use a simple virtual scroll:

> **Option A (recommended for simplicity):** Use
> [`vue-virtual-scroller`](https://github.com/Akryum/vue-virtual-scroller)
> (MIT). Wrap the `v-for` in `<RecycleScroller :items="stations"
> :item-size="52">`. Adds ~20 kB to the bundle.
>
> **Option B:** Implement a simple fixed-height virtual scroll using a computed
> `slicedStations = stations.slice(scrollTop/rowHeight, ...)` and an
> `@scroll` handler on the container. No dependency, ~50 lines of code.

**Files:**
- `ui/src/views/StationsView.vue` (new)
- `ui/src/components/StationRow.vue` (new)
- `ui/package.json` (add `vue-virtual-scroller` if Option A)

**Tests:**
- Vitest: list renders all fetched stations.
- Vitest: search filter hides non-matching stations.
- Vitest: sort by SNR orders stations descending.
- Vitest: offline stations shown but visually distinct.

---

### Step 5 — M5.4: Connect on click

Clicking a station must:
1. Update `store.connectedStation`.
2. Trigger the connection in C++ (`KiwiClient::connect`).
3. Automatically switch to Tab 2 ("KIWI UI").

#### UI side

In `StationRow.vue`:

```ts
async function onConnect(station: StationInfo) {
  store.connectedStation = station;
  pluginService.setStation(station.url);   // bridge message
  tabStore.currentTab = 'kiwi';            // switch tab
}
```

#### C++ side

`plugin_editor.cpp` already handles `setStation` (added in M3.3):

```cpp
case MessageType::setStation:
    processor_->setStation(host, port);
    break;
```

`PluginProcessor::setStation(host, port)` posts a connect task to the worker
thread, which calls `kiwiClient_->disconnect()` (if connected) then
`kiwiClient_->connect(host, port)`.

#### Reconnection / error handling

- If connect fails (timeout, refused), C++ pushes
  `{"type":"status","data":{"connected":false,"error":"Connection refused"}}`.
- UI shows an error badge on the station row and resets `store.connectedStation`
  to `null`.
- Implement exponential backoff in `KiwiClient` (if not already done in M3):
  1st retry after 2 s, 2nd after 4 s, 3rd after 8 s, then give up.

#### Disconnect / switch station

- Clicking a different station while connected: disconnect first, then connect.
- A "Disconnect" button in the status bar (or on the station row of the
  currently connected station).

**Files:**
- `ui/src/components/StationRow.vue`
- `ui/src/views/StationsView.vue`
- `ui/src/services/pluginService.ts` (setStation already present)
- `source/vst/processor/plugin_processor.h/.cpp` (reconnect logic)
- `source/network/kiwi_client.h/.cpp` (exponential backoff)

**Tests:**
- Integration (mock server): station click → `SET auth` WebSocket frame sent to
  the selected host:port.
- Integration: click while connected → disconnect first, then reconnect.
- Integration: connect failure → `store.connectedStation` reset to null.
- Vitest: UI automatically switches to Tab 2 on station click.

---

## State diagram

```
         ┌────────────────────────────────────────┐
         │            No Station                  │
         │  Tab 1: station list (offline/online)  │
         │  Tab 2: "please select station first"  │
         └──────────────┬─────────────────────────┘
                        │ User clicks station
                        ▼
         ┌────────────────────────────────────────┐
         │           Connecting...                │
         │  Tab 2: spinner / "Connecting to X..." │
         └──────────────┬─────────────────────────┘
              ┌─────────┴────────────┐
          success                 failure
              ▼                        ▼
  ┌──────────────────────┐   ┌──────────────────────┐
  │     Connected         │   │      Error            │
  │  Tab 2: full M4 UI    │   │  Tab 1: error badge   │
  │  Tab 1: ● station row │   │  Tab 2: empty state   │
  └──────────────────────┘   └──────────────────────┘
```

---

## Risk register

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| `rx-888.com` API unavailable or format changes | Medium | Cache last-known list in `localStorage`; show stale-data warning; implement fallback endpoint. |
| WebView2 CORS blocks `fetch()` from `file://` to `https://` | Medium | Verify in practice. If blocked: proxy the fetch via C++ network thread, push JSON to UI via bridge. |
| Station list has 1000+ entries → slow render | Low | Implement virtual scroll from the start (Option A or B). |
| User connects to wrong station accidentally | Low | Show the connected station name prominently in the status bar; one-click disconnect. |
| `KiwiClient::connect()` while audio thread is active | Medium | Gate the reconnect behind a state machine in the processor; pause audio output during reconnect. |

---

## Full M5 file list

| File | Status |
|------|--------|
| `ui/src/components/KTabBar.vue` | new |
| `ui/src/views/StationsView.vue` | new |
| `ui/src/views/KiwiView.vue` | new (or rename `PluginView.vue`) |
| `ui/src/components/StationRow.vue` | new |
| `ui/src/models/station.ts` | new |
| `ui/src/services/stationService.ts` | new |
| `ui/src/store/kiwiStore.ts` | extend (tabs + station list) |
| `ui/src/App.vue` | extend (tab structure) |
| `source/network/kiwi_client.h/.cpp` | extend (reconnect, backoff) |
| `source/vst/processor/plugin_processor.h/.cpp` | extend (reconnect state machine) |
