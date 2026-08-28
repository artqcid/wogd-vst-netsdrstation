<template>
  <div class="kiwi-root">
    <!-- A. TOP HEADER BAR (~55px, #EAEAEA) -->
    <header class="kiwi-header">
      <div class="kiwi-header__left">
        <!-- Kiwi bird logo (inline SVG, green circle) -->
        <svg class="kiwi-header__logo" viewBox="0 0 40 40" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
          <circle cx="20" cy="20" r="19" fill="#4CAF50" stroke="#2e7d32" stroke-width="1.5"/>
          <ellipse cx="21" cy="23" rx="11" ry="8" fill="#2e7d32"/>
          <circle cx="29" cy="17" r="5" fill="#2e7d32"/>
          <line x1="33" y1="15" x2="40" y2="14" stroke="#2e7d32" stroke-width="2.5" stroke-linecap="round"/>
          <circle cx="31" cy="16" r="1.5" fill="white"/>
        </svg>
        <div class="kiwi-header__title-stack">
          <div class="kiwi-header__title">NetSDRStation</div>
          <div class="kiwi-header__sub">{{ store.station || 'no station' }}</div>
          <div class="kiwi-header__sub">Antenna: KiwiSDR broadband</div>
        </div>
      </div>

      <div class="kiwi-header__center">
        <div class="kiwi-header__center-name">NetSDRStation — KiwiSDR Receiver</div>
        <div class="kiwi-header__center-status">{{ statusText }}</div>
        <StationInput
          :station="store.station"
          :status="store.status"
          @connect="onStation"
          @disconnect="store.disconnect()"
        />
      </div>

      <div class="kiwi-header__right">
        <div class="kiwi-header__callsign-row">
          <span class="kiwi-header__label">Your name or callsign:</span>
          <input type="text" class="kiwi-header__callsign-input" placeholder="callsign" />
        </div>
        <div class="kiwi-header__time-stack">
          <span class="kiwi-header__time-utc">{{ utcTime }}</span>
          <span class="kiwi-header__time-local">{{ localTime }}</span>
          <span class="kiwi-header__timezone">{{ tzName }}</span>
        </div>
      </div>
    </header>

    <!-- B. BAND SCALE STRIP (~20px) -->
    <BandScaleBar
      :view-low-mhz="loMhz"
      :view-high-mhz="hiMhz"
      @tune="onBandTune"
      @pan="onPan"
    />

    <!-- C. TAG / DX AREA -->
    <TagArea
      :view-low-khz="loKhz"
      :view-high-khz="hiKhz"
      @tune="onTagTune"
    />

    <!-- MAIN WORKSPACE: frequency ruler + waterfall + floating panel -->
    <main class="kiwi-main">
      <!-- Frequency ruler at top edge of canvas area -->
      <FrequencyRuler
        :view-low-khz="loKhz"
        :view-high-khz="hiKhz"
        :cursor-khz="store.freqKhz"
        :low-cut-hz="store.lowCut"
        :high-cut-hz="store.highCut"
        :zoom-level="store.wfZoom"
        @tune="onFreqRulerTune"
        @low-cut="onFreqRulerLowCut"
        @high-cut="onFreqRulerHighCut"
        @zoom="onWfZoom"
      />

      <!-- Waterfall canvas (fills remaining space) -->
      <div class="kiwi-canvas-area">
        <!-- Floating Play button left edge -->
        <button class="kiwi-play-btn" aria-label="Start audio" @click="onToggleAudio()">▶</button>

        <Waterfall
          :bins="store.waterfallBins"
          :color-map="store.colorMap"
          :cursor-khz="store.freqKhz"
          :centre-khz="store.freqKhz"
          :span-khz="spanKhz"
          :low-cut-hz="store.lowCut"
          :high-cut-hz="store.highCut"
          @zoom="onWfZoom"
        />

        <!-- Floating control panel — position absolute, bottom-right -->
        <aside class="kiwi-cpanel" aria-label="Control panel">
          <!-- Panel toggle arrow — top-right corner (matching original KiwiSDR) -->
          <button class="kiwi-cpanel__toggle" @click="isPanelOpen = !isPanelOpen" aria-label="Toggle panel"
            :title="isPanelOpen ? 'Hide panel' : 'Show panel'">
            {{ isPanelOpen ? '▼' : '▲' }}
          </button>

          <div class="kiwi-cpanel__body" :class="{ 'kiwi-cpanel__body--closed': !isPanelOpen }">
            <!-- Row 1: Freq input + dropdowns -->
            <div class="kiwi-cpanel__row kiwi-cpanel__row--freq">
              <input
                type="text"
                class="kiwi-cpanel__freq-input"
                :value="store.freqKhz.toFixed(2)"
                @change="onFreqInput"
                aria-label="Frequency kHz"
              />
              <select class="kiwi-cpanel__select" aria-label="Band">
                <option>select band ∨</option>
              </select>
              <select class="kiwi-cpanel__select" aria-label="Extension">
                <option>extension ∨</option>
              </select>
              <button class="kiwi-cpanel__play-btn" aria-label="Play">▶</button>
            </div>

            <!-- Row 2: Mini icons -->
            <div class="kiwi-cpanel__row kiwi-cpanel__row--icons">
              <span class="kiwi-cpanel__icon" title="Menu">☰</span>
              <span class="kiwi-cpanel__icon kiwi-cpanel__icon--cyan" title="Users">A</span>
              <span class="kiwi-cpanel__icon kiwi-cpanel__icon--green" title="Status">✓</span>
              <span class="kiwi-cpanel__icon kiwi-cpanel__icon--green" title="Active receivers">9</span>
              <span class="kiwi-cpanel__icon-sep"></span>
              <button class="kiwi-cpanel__icon-btn" @click="onZoom(1)" title="Zoom in">🔍+</button>
              <button class="kiwi-cpanel__icon-btn" @click="onZoom(-1)" title="Zoom out">🔍−</button>
              <button class="kiwi-cpanel__icon-btn" @click="onZoomTo(0)" title="Max zoom out">↖↙</button>
              <button class="kiwi-cpanel__icon-btn" @click="onZoomTo(14)" title="Max zoom in">↗↘</button>
              <button class="kiwi-cpanel__icon-btn" @click="onZoomToBand()" title="Zoom to band">↔</button>
              <button class="kiwi-cpanel__icon-btn" @click="onPan(-1)" title="Pan left">◀</button>
              <button class="kiwi-cpanel__icon-btn" @click="onPan(1)" title="Pan right">▶</button>
              <span class="kiwi-cpanel__icon-sep"></span>
              <button class="kiwi-cpanel__icon-btn" @click="onToggleCic()" title="CIC compensation">↺</button>
              <span class="kiwi-cpanel__text-label">Spectrum</span>
              <button class="kiwi-cpanel__icon-btn kiwi-cpanel__icon-btn--red" @click="onResetWf()" title="Reset">↻</button>
              <button class="kiwi-cpanel__icon-btn kiwi-cpanel__icon-btn--green" @click="onToggleAudio()" title="Audio">♪</button>
            </div>

            <!-- Row 3: Mode buttons -->
            <div class="kiwi-cpanel__row kiwi-cpanel__row--modes">
              <button
                v-for="m in panelModes"
                :key="m.idx"
                class="kiwi-cpanel__mode-btn"
                :class="{ 'kiwi-cpanel__mode-btn--active': store.mode === m.idx }"
                @click="onMode(m.idx)"
              >{{ m.label }}</button>
            </div>

            <!-- Row 4: Navigation buttons -->
            <div class="kiwi-cpanel__row kiwi-cpanel__row--nav">
              <button class="kiwi-cpanel__nav-btn" @click="stepFreq(-1, 10)" title="−10 kHz">−10</button>
              <button class="kiwi-cpanel__nav-btn" @click="stepFreq(-1, 1)" title="−1 kHz">−1</button>
              <button class="kiwi-cpanel__nav-btn" @click="stepFreq(-1, 0.1)" title="−0.1 kHz">−0.1</button>
              <button class="kiwi-cpanel__nav-btn" @click="stepFreq(1, 0.1)" title="+0.1 kHz">+0.1</button>
              <button class="kiwi-cpanel__nav-btn" @click="stepFreq(1, 1)" title="+1 kHz">+1</button>
              <button class="kiwi-cpanel__nav-btn" @click="stepFreq(1, 10)" title="+10 kHz">+10</button>
            </div>

            <!-- Row 5: Sub-tabs -->
            <div class="kiwi-cpanel__row kiwi-cpanel__row--tabs">
              <button
                v-for="tab in subTabs"
                :key="tab.id"
                class="kiwi-cpanel__tab-btn"
                :class="{ 'kiwi-cpanel__tab-btn--active': activeTab === tab.id }"
                :style="{ background: tab.bg, color: tab.fg }"
                @click="activeTab = tab.id"
              >{{ tab.label }}</button>
            </div>

            <!-- Row 6: Colormap bar -->
            <div class="kiwi-cpanel__colormap" aria-label="Colormap"
              :style="{ background: store.colorMap === 'default'
                ? 'linear-gradient(to right, #000, #f00, #ff0, #fff)'
                : store.colorMap === 'rain'
                ? 'linear-gradient(to right, #001, #00f, #0ff, #fff)'
                : 'linear-gradient(to right, #000, #222, #888, #ddd, #fff)'
              }"></div>

            <!-- Rows 7-10: WF0 tab content -->
            <template v-if="activeTab === 'WF0'">
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">WF ceil</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="-10" :max="0" :step="1"
                  :value="store.wfMaxDb" @input="onSlider('wfMaxDb', $event)" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.wfMaxDb > 0 ? '+' : '' }}{{ store.wfMaxDb }} dB</span>
                <button class="kiwi-cpanel__btn kiwi-cpanel__btn--green">Auto Scale</button>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">WF floor</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="-160" :max="-60" :step="1"
                  :value="store.wfMinDb" @input="onSlider('wfMinDb', $event)" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.wfMinDb }} dB</span>
                <button class="kiwi-cpanel__btn kiwi-cpanel__btn--gray">Spec Color</button>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">WF rate</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="0" :max="4" :step="1"
                  :value="store.wfSpeed" @input="onSlider('wfSpeed', $event)" />
                <span class="kiwi-cpanel__ctrl-val">{{ ['pause','slow','med','fast','max'][store.wfSpeed] }}</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Spec Δ</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="0" :max="2" :step="0.1"
                  :value="0.2" readonly />
                <span class="kiwi-cpanel__ctrl-val">0.2 gain</span>
                <button class="kiwi-cpanel__btn kiwi-cpanel__btn--violet">P1</button>
              </div>
            </template>

            <template v-if="activeTab === 'Audio'">
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Volume</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="0" :max="100" :step="1"
                  :value="store.volume" @input="onSlider('volume', $event)" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.volume }}%</span>
                <button class="kiwi-cpanel__btn kiwi-cpanel__btn--red" @click="store.setParam('mute', store.mute ? 0 : 1)">{{ store.mute ? 'Unmute' : 'Mute' }}</button>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">NR</span>
                <button class="kiwi-cpanel__btn" :class="store.nrOn ? 'kiwi-cpanel__btn--green' : 'kiwi-cpanel__btn--gray'"
                  @click="store.setParam('nrOn', store.nrOn ? 0 : 1)">{{ store.nrOn ? 'ON' : 'OFF' }}</button>
                <span class="kiwi-cpanel__ctrl-label" style="margin-left:16px">Compression</span>
                <button class="kiwi-cpanel__btn kiwi-cpanel__btn--gray" title="Not yet implemented">OFF</button>
                <span class="kiwi-cpanel__ctrl-label" style="margin-left:16px">De‑emphasis</span>
                <button class="kiwi-cpanel__btn kiwi-cpanel__btn--gray" title="Not yet implemented">OFF</button>
              </div>
            </template>

            <template v-if="activeTab === 'AGC'">
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">AGC</span>
                <button class="kiwi-cpanel__btn" :class="store.agcOn ? 'kiwi-cpanel__btn--green' : 'kiwi-cpanel__btn--gray'"
                  @click="store.setParam('agcOn', store.agcOn ? 0 : 1)">{{ store.agcOn ? 'ON' : 'OFF' }}</button>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Threshold</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="-140" :max="0" :step="1"
                  :value="store.agcThresh" @input="onSlider('agcThresh', $event)" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.agcThresh }} dB</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Decay</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="20" :max="5000" :step="10"
                  :value="store.agcDecay" @input="onSlider('agcDecay', $event)" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.agcDecay }} ms</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Hang</span>
                <button class="kiwi-cpanel__btn" :class="store.agcHang ? 'kiwi-cpanel__btn--green' : 'kiwi-cpanel__btn--gray'"
                  @click="store.setParam('agcHang', store.agcHang ? 0 : 1)">{{ store.agcHang ? 'ON' : 'OFF' }}</button>
                <span class="kiwi-cpanel__ctrl-label" style="margin-left:16px">Slope</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider kiwi-cpanel__slider--sm" :min="0" :max="100" :step="1"
                  :value="store.agcSlope" @input="onSlider('agcSlope', $event)" style="width:60px" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.agcSlope }}</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Man Gain</span>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider" :min="0" :max="100" :step="1"
                  :value="store.agcManGain" @input="onSlider('agcManGain', $event)" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.agcManGain }}%</span>
              </div>
            </template>

            <template v-if="activeTab === 'User'">
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Squelch</span>
                <button class="kiwi-cpanel__btn" :class="store.squelchOn ? 'kiwi-cpanel__btn--green' : 'kiwi-cpanel__btn--gray'"
                  @click="store.setParam('squelchOn', store.squelchOn ? 0 : 1)">{{ store.squelchOn ? 'ON' : 'OFF' }}</button>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider kiwi-cpanel__slider--sm" :min="0" :max="1" :step="0.01"
                  :value="store.squelchThr" @input="onSlider('squelchThr', $event)" style="width:60px" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.squelchThr.toFixed(2) }}</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">NB</span>
                <button class="kiwi-cpanel__btn" :class="store.nbOn ? 'kiwi-cpanel__btn--green' : 'kiwi-cpanel__btn--gray'"
                  @click="store.setParam('nbOn', store.nbOn ? 0 : 1)">{{ store.nbOn ? 'ON' : 'OFF' }}</button>
                <input type="range" class="kiwi-slider kiwi-cpanel__slider kiwi-cpanel__slider--sm" :min="0" :max="100" :step="1"
                  :value="store.nbThresh" @input="onSlider('nbThresh', $event)" style="width:60px" />
                <span class="kiwi-cpanel__ctrl-val">{{ store.nbThresh }}%</span>
              </div>
            </template>

            <template v-if="activeTab === 'Stat'">
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">GPS</span>
                <span class="kiwi-cpanel__ctrl-val">locked (8 sats)</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Users</span>
                <span class="kiwi-cpanel__ctrl-val">0/4</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Buffer</span>
                <span class="kiwi-cpanel__ctrl-val kiwi-cpanel__ctrl-val--green">OK</span>
              </div>
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">SNR</span>
                <span class="kiwi-cpanel__ctrl-val">{{ store.station ? '32 dB' : '—' }}</span>
              </div>
            </template>

            <template v-if="activeTab === 'Off'">
              <div class="kiwi-cpanel__ctrl-row">
                <span class="kiwi-cpanel__ctrl-label">Audio output</span>
                <button class="kiwi-cpanel__btn kiwi-cpanel__btn--red" @click="store.setParam('mute', 1)">MUTE</button>
                <span style="color:#888;font-size:9px;margin-left:8px">Audio output disabled</span>
              </div>
            </template>

            <!-- Row 11: Dropdowns + P2 -->
            <div class="kiwi-cpanel__row kiwi-cpanel__row--dropdowns">
              <select class="kiwi-cpanel__select kiwi-cpanel__select--sm" :value="store.colorMap"
                @change="(store as any).colorMap = ($event.target as HTMLSelectElement).value">
                <option value="default">Kiwi</option>
                <option value="rain">Rain</option>
                <option value="grayscale">Grey</option>
              </select>
              <select class="kiwi-cpanel__select kiwi-cpanel__select--sm" :value="store.wfComp ? 1 : 0"
                @change="store.setParam('wfComp', parseInt(($event.target as HTMLSelectElement).value))">
                <option :value="0">auto</option>
                <option :value="2">IIR</option>
                <option :value="1">MMA</option>
                <option :value="-1">off</option>
              </select>
              <select class="kiwi-cpanel__select kiwi-cpanel__select--sm">
                <option>off</option><option>2s</option><option>5s</option>
              </select>
              <select class="kiwi-cpanel__select kiwi-cpanel__select--sm">
                <option>IIR</option><option>MMA</option><option>EMA</option>
              </select>
              <span class="kiwi-cpanel__arrow">▼</span>
              <button class="kiwi-cpanel__btn kiwi-cpanel__btn--violet">P2</button>
            </div>

            <!-- Footer: S-Meter -->
            <div class="kiwi-cpanel__smeter">
              <div class="kiwi-cpanel__smeter-labels">
                <span>S1</span><span>S3</span><span>S5</span><span>S7</span><span>S9</span>
                <span>+10</span><span>+20</span><span>+40</span><span>+60</span>
                <span class="kiwi-cpanel__smeter-dbm">{{ store.signalLevel.toFixed(1) }} dBm</span>
              </div>
              <div class="kiwi-cpanel__smeter-bar">
                <div class="kiwi-cpanel__smeter-fill"
                  :style="{ width: smeterWidth + '%' }"></div>
              </div>
            </div>
          </div>
        </aside>
      </div>
    </main>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import { storeToRefs } from 'pinia'
import StationInput from '@/components/StationInput.vue'
import BandScaleBar from '@/components/BandScaleBar.vue'
import TagArea from '@/components/TagArea.vue'
import FrequencyRuler from '@/components/FrequencyRuler.vue'
import Waterfall from '@/components/Waterfall.vue'
import { useKiwiStore } from '@/store/kiwiStore'
import { pluginService } from '@/services/pluginService'
import type { ParamId } from '@/generated/bridge-validators'

const store = useKiwiStore()
const { statusText, statusState } = storeToRefs(store)

// Zoom-Architektur: sichtbarer Frequenzbereich aus wfZoom + freqKhz
const spanKhz = computed(() => {
  const maxSpan = 30000  // volle Bandbreite 0-30 MHz
  const z = typeof store.wfZoom === 'number' ? store.wfZoom : 0
  return Math.max(0.01, maxSpan / Math.pow(2, z))
})
const loKhz = computed(() => store.freqKhz - spanKhz.value / 2)
const hiKhz = computed(() => store.freqKhz + spanKhz.value / 2)
const loMhz = computed(() => loKhz.value / 1000)
const hiMhz = computed(() => hiKhz.value / 1000)

// Panel state
const isPanelOpen = ref(true)
const activeTab = ref<string>('WF0')

// Time display
const utcTime = ref('')
const localTime = ref('')
const tzName = ref('')
let timeTimer: ReturnType<typeof setInterval>

function updateTime() {
  const now = new Date()
  const pad = (n: number) => String(n).padStart(2, '0')
  utcTime.value = `${pad(now.getUTCHours())}:${pad(now.getUTCMinutes())}:${pad(now.getUTCSeconds())} UTC`
  localTime.value = now.toLocaleTimeString()
  tzName.value = Intl.DateTimeFormat().resolvedOptions().timeZone
}

// Sub-tabs definition
const subTabs = [
  { id: 'RF',    label: 'RF',    bg: '#4CAF50', fg: 'black' },
  { id: 'WF0',   label: 'WF0',   bg: '#e53935', fg: 'white' },
  { id: 'Audio', label: 'Audio', bg: '#1565c0', fg: 'white' },
  { id: 'AGC',   label: 'AGC',   bg: '#6a1b9a', fg: 'white' },
  { id: 'User',  label: 'User',  bg: '#00838f', fg: 'white' },
  { id: 'Stat',  label: 'Stat',  bg: '#e65100', fg: 'white' },
  { id: 'Off',   label: 'Off',   bg: '#111',    fg: '#666' },
] as const

// Mode buttons (8 main modes shown in panel)
const panelModes = [
  { idx: 0,  label: 'AM' },
  { idx: 13, label: 'SAM' },
  { idx: 12, label: 'DRM' },
  { idx: 5,  label: 'LSB' },
  { idx: 3,  label: 'USB' },
  { idx: 7,  label: 'CW' },
  { idx: 9,  label: 'NBFM' },
  { idx: 11, label: 'IQ' },
]

// S-meter width (0-100%)
const smeterWidth = computed(() => {
  const min = -127; const max = 20
  return Math.max(0, Math.min(100, ((store.signalLevel - min) / (max - min)) * 100))
})

function onStation(hostPort: string) {
  store.setStation(hostPort)
  if (!pluginService.isInNative() && !/error/i.test(store.status)) {
    store.setStatus('Connected (dev)')
  }
}

function onFreqInput(e: Event) {
  const val = parseFloat((e.target as HTMLInputElement).value)
  if (!isNaN(val)) store.setParam('freqKhz', Math.max(0.001, Math.min(30000, val)))
}

function onZoom(delta: number) {
  const z = typeof store.wfZoom === 'number' ? store.wfZoom : 0
  store.setParam('wfZoom', Math.max(0, Math.min(14, z + delta)))
}

function onZoomTo(level: number) {
  store.setParam('wfZoom', Math.max(0, Math.min(14, level)))
}

function stepFreq(dir: number, step = 1) {
  store.setParam('freqKhz', Math.max(0.001, Math.min(30000, store.freqKhz + dir * step)))
}

function onToggleCic() {
  store.setParam('wfComp', store.wfComp ? 0 : 1)
}

function onResetWf() {
  store.setParam('wfZoom', 0)
  store.setParam('wfMaxDb', -30)
  store.setParam('wfMinDb', -130)
  store.setParam('wfSpeed', 2)
}

function onToggleAudio() {
  store.setParam('mute', store.mute ? 0 : 1)
}

function onZoomToBand() {
  const z = typeof store.wfZoom === 'number' ? store.wfZoom : 0
  if (z > 10) store.setParam('wfZoom', 0)
  else if (z < 4) store.setParam('wfZoom', 14)
  else store.setParam('wfZoom', 7)
}

function onMode(idx: number) {
  store.setParam('mode', idx)
}

function onSlider(id: ParamId, e: Event) {
  const val = parseFloat((e.target as HTMLInputElement).value)
  if (!isNaN(val)) store.setParam(id, val)
}

// BandScaleBar: Klick auf Band → Frequenz auf Band-Mitte setzen
function onBandTune(freqKhz: number) {
  store.setParam('freqKhz', Math.max(0.001, Math.min(30000, freqKhz)))
}

// TagArea: Klick auf DX-Tag → Frequenz auf Tag-Frequenz
function onTagTune(freqKhz: number) {
  store.setParam('freqKhz', Math.max(0.001, Math.min(30000, freqKhz)))
}

// FrequencyRuler: Draggen des Cursors
function onFreqRulerTune(freqKhz: number) {
  store.setParam('freqKhz', Math.max(0.001, Math.min(30000, freqKhz)))
}

function onFreqRulerLowCut(hz: number) {
  store.setParam('lowCut', hz)
}

function onFreqRulerHighCut(hz: number) {
  store.setParam('highCut', hz)
}

// Zoom-Event von Waterfall/FrequencyRuler: Ctrl+Wheel
function onWfZoom(delta: number, anchorFrac: number) {
  const oldFreq = store.freqKhz
  const oldLo = loKhz.value
  const oldHi = hiKhz.value
  const anchorKhz = oldLo + (oldHi - oldLo) * anchorFrac

  let newZoom = (typeof store.wfZoom === 'number' ? store.wfZoom : 0) + delta
  newZoom = Math.max(0, Math.min(14, newZoom))
  store.setParam('wfZoom', newZoom)

  // Anchor-Frequenz beibehalten: freqKhz neu berechnen
  const newSpan = Math.max(0.01, 30000 / Math.pow(2, newZoom))
  const newLo = anchorKhz - newSpan * anchorFrac
  store.setParam('freqKhz', Math.max(0.001, Math.min(30000, newLo + newSpan / 2)))
}

// Pan-Event von BandScaleBar
function onPan(dir: number) {
  const shift = spanKhz.value * 0.5 * dir
  store.setParam('freqKhz', Math.max(0.001, Math.min(30000, store.freqKhz + shift)))
}

onMounted(() => {
  updateTime()
  timeTimer = setInterval(updateTime, 1000)

  pluginService.onMessage(message => {
    if (message.type === 'param') store.applyParam(message.data.id, message.data.value)
    if (message.type === 'status') store.setStatus(message.data)
  })
  pluginService.onLevel(dbm => store.setSignalLevel(dbm))
  pluginService.onWaterfall(bins => store.setWaterfallBins(bins))
  pluginService.getParameters()
})

onBeforeUnmount(() => {
  clearInterval(timeTimer)
})
</script>

<style scoped>
/* ===== ROOT ===== */
.kiwi-root {
  display: flex;
  flex-direction: column;
  width: 100%;
  height: 100%;
  background: var(--kiwi-bg);
  color: var(--kiwi-text);
  font-size: var(--kiwi-font-md);
  overflow: hidden;
}

/* ===== A. HEADER BAR ===== */
.kiwi-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-shrink: 0;
  min-height: 55px;
  padding: 4px 10px;
  background: var(--kiwi-topbar-bg);
  color: var(--kiwi-topbar-text);
  gap: 10px;
}

.kiwi-header__left {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.kiwi-header__logo {
  width: 40px;
  height: 40px;
  flex-shrink: 0;
}

.kiwi-header__title-stack {
  display: flex;
  flex-direction: column;
  line-height: 1.3;
}

.kiwi-header__title {
  font-size: var(--kiwi-font-xl);
  font-weight: bold;
  color: var(--kiwi-topbar-text);
}

.kiwi-header__sub {
  font-size: var(--kiwi-font-sm);
  color: var(--kiwi-topbar-sub);
}

.kiwi-header__center {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  font-size: var(--kiwi-font-sm);
  color: var(--kiwi-topbar-sub);
  text-align: center;
  min-width: 0;
}

.kiwi-header__center-name {
  font-weight: bold;
  font-size: var(--kiwi-font-md);
  color: var(--kiwi-topbar-text);
}

.kiwi-header__center-status {
  font-size: var(--kiwi-font-sm);
  color: var(--kiwi-topbar-sub);
}

.kiwi-header__right {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-shrink: 0;
}

.kiwi-header__callsign-row {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 2px;
}

.kiwi-header__label {
  font-size: var(--kiwi-font-sm);
  color: var(--kiwi-topbar-sub);
}

.kiwi-header__callsign-input {
  background: white;
  border: 1px solid #ccc;
  padding: 2px 6px;
  font-size: var(--kiwi-font-md);
  width: 130px;
  color: #333;
}

.kiwi-header__time-stack {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
}

.kiwi-header__time-utc {
  font-size: 13px;
  font-weight: bold;
  color: var(--kiwi-text-muted);
}

.kiwi-header__time-local {
  font-size: 10px;
  color: var(--kiwi-text-muted);
}

.kiwi-header__timezone {
  font-size: 8px;
  color: var(--kiwi-text-muted);
}

/* ===== B. BAND SCALE ===== */
.kiwi-bandscale {
  display: flex;
  align-items: center;
  height: 20px;
  background: white;
  flex-shrink: 0;
  border-bottom: 1px solid #ddd;
}

.kiwi-bandscale__arrow {
  font-size: 10px;
  padding: 0 4px;
  color: #555;
  cursor: pointer;
  flex-shrink: 0;
  user-select: none;
}

.kiwi-bandscale__inner {
  flex: 1;
  position: relative;
  height: 100%;
  overflow: hidden;
}

.kiwi-bandscale__block {
  position: absolute;
  height: 16px;
  top: 2px;
  border-radius: 3px;
  font-size: 8px;
  font-weight: bold;
  padding: 1px 3px;
  white-space: nowrap;
  cursor: pointer;
  color: black;
  border: 1px solid rgba(0,0,0,0.2);
  line-height: 14px;
}

/* ===== C. TAG / DX AREA ===== */
.kiwi-tagarea {
  position: relative;
  height: 22px;
  background: #aaaaaa;
  flex-shrink: 0;
  overflow: hidden;
  border-bottom: 1px solid #888;
}

.kiwi-tag {
  position: absolute;
  top: 2px;
  height: 18px;
  font-size: 9px;
  padding: 1px 3px;
  border: 1px solid rgba(0,0,0,0.3);
  border-radius: 2px;
  cursor: pointer;
  white-space: nowrap;
  line-height: 16px;
  color: black;
}

/* ===== MAIN WORKSPACE ===== */
.kiwi-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
  position: relative;
}

/* Frequency ruler */
.kiwi-freq-ruler {
  display: flex;
  align-items: stretch;
  height: 26px;
  background: #2a2a2a;
  flex-shrink: 0;
  border-bottom: 1px solid #444;
}

.kiwi-freq-ruler__db {
  font-size: 9px;
  color: var(--kiwi-yellow);
  padding: 2px 6px;
  flex-shrink: 0;
  align-self: center;
}

.kiwi-freq-ruler__scale {
  flex: 1;
  position: relative;
  overflow: hidden;
}

.kiwi-freq-ruler__label {
  position: absolute;
  bottom: 2px;
  font-size: 9px;
  color: white;
  transform: translateX(-50%);
  white-space: nowrap;
}

.kiwi-freq-ruler__tick {
  position: absolute;
  top: 0;
  bottom: 0;
  width: 1px;
  background: rgba(255,255,255,0.4);
}

/* Canvas area (waterfall + floating panel) */
.kiwi-canvas-area {
  flex: 1;
  position: relative;
  background: #000;
  overflow: hidden;
  min-height: 0;
}

/* Floating play button */
.kiwi-play-btn {
  position: absolute;
  left: 0;
  top: 50%;
  transform: translateY(-50%);
  width: 28px;
  height: 44px;
  background: #7c4dff;
  color: white;
  border: none;
  border-radius: 0 6px 6px 0;
  font-size: 16px;
  cursor: pointer;
  z-index: 20;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0;
}

.kiwi-play-btn:hover { background: #9c6fff; }

/* ===== CONTROL PANEL ===== */
.kiwi-cpanel {
  position: absolute;
  bottom: 10px;
  right: 0;
  z-index: 100;
  width: var(--kiwi-panel-width, 360px);
}

.kiwi-cpanel__toggle {
  position: absolute;
  top: -22px;
  right: 0;
  width: 24px;
  height: 20px;
  background: #3a3a3a;
  color: #ccc;
  border: 1px solid #555;
  border-bottom: none;
  border-radius: 6px 6px 0 0;
  cursor: pointer;
  font-size: 9px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0;
  z-index: 101;
  line-height: 1;
}

.kiwi-cpanel__toggle:hover {
  background: #555;
  color: #fff;
}

.kiwi-cpanel__body {
  background: var(--kiwi-panel);
  border: 1px solid var(--kiwi-border);
  border-radius: var(--kiwi-panel-radius);
  overflow: hidden;
  transition: transform 0.25s ease, opacity 0.25s ease;
  transform: translateX(0);
  display: flex;
  flex-direction: column;
  gap: 0;
}

.kiwi-cpanel__body--closed {
  transform: translateX(100%);
  opacity: 0;
  pointer-events: none;
}

/* Panel rows */
.kiwi-cpanel__row {
  display: flex;
  align-items: center;
  padding: 3px 6px;
  gap: 3px;
  border-bottom: 1px solid rgba(255,255,255,0.06);
}

/* Row 1: frequency */
.kiwi-cpanel__freq-input {
  background: var(--kiwi-input-bg);
  color: white;
  border: 1px solid var(--kiwi-input-border);
  padding: 2px 5px;
  font-size: 13px;
  font-family: var(--kiwi-font-mono);
  width: 85px;
  flex-shrink: 0;
}

.kiwi-cpanel__select {
  background: var(--kiwi-select-bg);
  color: var(--kiwi-text);
  border: 1px solid var(--kiwi-border);
  font-size: 10px;
  padding: 1px 2px;
  flex: 1;
  min-width: 0;
}

.kiwi-cpanel__select--sm {
  flex: 1;
  min-width: 50px;
  font-size: 9px;
}

.kiwi-cpanel__play-btn {
  background: #555;
  color: white;
  border: 1px solid #777;
  border-radius: 50%;
  width: 22px;
  height: 22px;
  font-size: 10px;
  cursor: pointer;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0;
}

/* Row 2: icons */
.kiwi-cpanel__row--icons { flex-wrap: wrap; gap: 2px; }

.kiwi-cpanel__icon {
  font-size: 13px;
  cursor: pointer;
  padding: 1px 3px;
  color: var(--kiwi-text);
}

.kiwi-cpanel__icon--cyan { color: #00bcd4; }
.kiwi-cpanel__icon--green { color: var(--kiwi-accent); }

.kiwi-cpanel__icon-sep {
  flex: 1;
  max-width: 8px;
}

.kiwi-cpanel__icon-btn {
  background: transparent;
  color: var(--kiwi-text);
  border: 1px solid var(--kiwi-border);
  border-radius: 2px;
  width: 20px;
  height: 18px;
  font-size: 12px;
  cursor: pointer;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.kiwi-cpanel__icon-btn--red { color: #e53935; border-color: #e53935; }
.kiwi-cpanel__icon-btn--green { color: var(--kiwi-accent); border-color: var(--kiwi-accent); }
.kiwi-cpanel__icon-btn:hover { background: rgba(255,255,255,0.1); }

.kiwi-cpanel__text-label {
  font-size: 10px;
  color: var(--kiwi-text);
  padding: 0 2px;
}

/* Row 3: mode buttons */
.kiwi-cpanel__row--modes { flex-wrap: wrap; }

.kiwi-cpanel__mode-btn {
  background: #3a3a3a;
  color: #999;
  border: 1px solid #555;
  padding: 2px 6px;
  font-size: 10px;
  cursor: pointer;
  border-radius: 2px;
  font-weight: bold;
  transition: background 0.1s, color 0.1s;
}

.kiwi-cpanel__mode-btn--active {
  background: #00FF00;
  color: #000;
  border-color: #00dd00;
}

.kiwi-cpanel__mode-btn:hover:not(.kiwi-cpanel__mode-btn--active) {
  background: #4a4a4a;
  color: #ddd;
}

/* Row 4: nav buttons */
.kiwi-cpanel__nav-btn {
  background: #3a3a3a;
  color: #ccc;
  border: 1px solid #555;
  border-radius: 3px;
  width: 30px;
  height: 26px;
  font-size: 11px;
  cursor: pointer;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.kiwi-cpanel__nav-btn:hover { background: #4a4a4a; color: white; }

/* Row 5: sub-tabs */
.kiwi-cpanel__row--tabs { padding: 2px 4px; gap: 2px; }

.kiwi-cpanel__tab-btn {
  flex: 1;
  padding: 2px 4px;
  font-size: 10px;
  font-weight: bold;
  border: 1px solid rgba(255,255,255,0.15);
  cursor: pointer;
  border-radius: 2px;
  opacity: 0.8;
  transition: opacity 0.15s;
}

.kiwi-cpanel__tab-btn--active { opacity: 1; border-color: white; }

/* Row 6: colormap bar */
.kiwi-cpanel__colormap {
  height: 12px;
  margin: 0 6px 2px;
  background: linear-gradient(to right, #000, #00f, #0ff, #0f0, #ff0, #f00, #f0f, #fff);
  border-radius: 2px;
  cursor: crosshair;
  flex-shrink: 0;
}

/* Rows 7-10: slider controls */
.kiwi-cpanel__ctrl-row {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 2px 6px;
  font-size: 10px;
}

.kiwi-cpanel__ctrl-label {
  font-size: 10px;
  color: #ccc;
  min-width: 60px;
  flex-shrink: 0;
}

.kiwi-cpanel__slider {
  flex: 1;
  height: 3px;
  accent-color: var(--kiwi-accent);
}

.kiwi-cpanel__ctrl-val {
  font-size: 10px;
  color: #aaa;
  min-width: 50px;
  text-align: right;
  flex-shrink: 0;
}

.kiwi-cpanel__ctrl-val--green {
  color: #4CAF50;
}

/* Buttons */
.kiwi-cpanel__btn {
  font-size: 9px;
  padding: 1px 5px;
  border-radius: 2px;
  cursor: pointer;
  border: none;
  flex-shrink: 0;
}

.kiwi-cpanel__btn--green { background: var(--kiwi-accent); color: black; }
.kiwi-cpanel__btn--gray  { background: #555; color: #ddd; border: 1px solid #777; }
.kiwi-cpanel__btn--violet { background: #7c4dff; color: white; }

/* Row 11: dropdowns */
.kiwi-cpanel__row--dropdowns { flex-wrap: wrap; gap: 2px; }
.kiwi-cpanel__arrow { font-size: 10px; color: #888; }

/* S-Meter footer */
.kiwi-cpanel__smeter {
  padding: 3px 6px 4px;
  background: var(--kiwi-panel);
  border-top: 1px solid rgba(255,255,255,0.1);
  flex-shrink: 0;
}

.kiwi-cpanel__smeter-labels {
  display: flex;
  justify-content: space-between;
  font-size: 8px;
  color: #999;
  margin-bottom: 2px;
}

.kiwi-cpanel__smeter-dbm {
  color: var(--kiwi-accent);
  font-size: 9px;
  font-family: var(--kiwi-font-mono);
}

.kiwi-cpanel__smeter-bar {
  height: 10px;
  background: linear-gradient(to right, #1a1a1a 0%, #4CAF50 30%, #ffeb3b 70%, #f44336 100%);
  border-radius: 2px;
  position: relative;
  border: 1px solid #333;
}

.kiwi-cpanel__smeter-fill {
  position: absolute;
  top: 0;
  left: 0;
  height: 100%;
  background: #4CAF50;
  border-radius: 2px 0 0 2px;
  max-width: 100%;
  transition: width 0.1s;
}
</style>
