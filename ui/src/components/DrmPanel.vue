<template>
  <div class="drm-panel" data-testid="drm-panel">
    <!-- Top: Schedule & Services overlay -->
    <div class="drm-schedule" data-testid="drm-schedule">
      <div class="drm-schedule__layout">
        <!-- Left: Status checkboxes -->
        <div class="drm-schedule__left">
          <div class="drm-schedule__checks">
            <label class="drm-schedule__check"><input type="checkbox" checked /> IO</label>
            <label class="drm-schedule__check"><input type="checkbox" checked /> Time</label>
            <label class="drm-schedule__check"><input type="checkbox" checked /> Frame</label>
            <label class="drm-schedule__check"><input type="checkbox" checked /> FAC</label>
            <label class="drm-schedule__check"><input type="checkbox" checked /> SDC</label>
            <label class="drm-schedule__check"><input type="checkbox" checked /> MSC</label>
          </div>
          <div class="drm-schedule__services">
            <span class="drm-schedule__services-title">Services:</span>
            <ul class="drm-schedule__services-list">
              <li>Service 1</li>
              <li>Service 2</li>
            </ul>
          </div>
        </div>

        <!-- Center: Schedule station list -->
        <div class="drm-schedule__center">
          <div class="drm-schedule__station-list">
            <div class="drm-schedule__station" v-for="n in 4" :key="n">
              <span class="drm-schedule__station-info">ℹ</span>
              <span class="drm-schedule__station-name">Station {{ n }}</span>
              <span class="drm-schedule__station-time">
                <span class="drm-schedule__bar drm-schedule__bar--green" style="width:40%"></span>
                <span class="drm-schedule__bar drm-schedule__bar--pink" style="width:30%"></span>
              </span>
            </div>
          </div>
          <div class="drm-schedule__now-line"></div>
        </div>

        <!-- Right: Time + legend -->
        <div class="drm-schedule__right">
          <div class="drm-schedule__time">{{ utcStr }}</div>
          <div class="drm-schedule__time">{{ localStr }}</div>
          <select class="drm-schedule__select">
            <option>by service</option>
          </select>
          <div class="drm-schedule__legend">
            <span class="drm-schedule__legend-item drm-schedule__legend-item--verified">verified</span>
            <span class="drm-schedule__legend-item drm-schedule__legend-item--not-verified">not verified</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Bottom: Decoder control panel -->
    <div class="drm-decoder" data-testid="drm-decoder">
      <div class="drm-decoder__header">
        <span class="drm-decoder__title">Digital Radio Mondiale decoder</span>
        <span class="drm-decoder__help">help</span>
        <span class="drm-decoder__close">✕</span>
      </div>
      <div class="drm-decoder__content">
        <p>DRM decoder is based on <a href="#">Dream 2.2.1</a></p>
        <p>Schedule information courtesy of <a href="#">drmrx.org</a></p>
      </div>
      <div class="drm-decoder__footer">
        <button class="drm-decoder__btn drm-decoder__btn--stop">Stop</button>
        <button class="drm-decoder__btn drm-decoder__btn--iq">Monitor IQ</button>
        <button class="drm-decoder__btn drm-decoder__btn--test">Test 1</button>
        <button class="drm-decoder__btn drm-decoder__btn--test">Test 2</button>
        <label class="drm-decoder__lpf">
          <input type="checkbox" /> LPF
        </label>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'

const utcStr = ref('')
const localStr = ref('')
let timeInterval: ReturnType<typeof setInterval> | null = null

function pad(n: number): string {
  return String(n).padStart(2, '0')
}

function updateTime() {
  const now = new Date()
  utcStr.value = `${pad(now.getUTCHours())}:${pad(now.getUTCMinutes())}:${pad(now.getUTCSeconds())} UTC`
  localStr.value = now.toLocaleTimeString()
}

onMounted(() => {
  updateTime()
  timeInterval = setInterval(updateTime, 1000)
})

onBeforeUnmount(() => {
  if (timeInterval) clearInterval(timeInterval)
})
</script>

<style scoped>
.drm-panel {
  position: relative;
  width: 100%;
  z-index: 20;
}

/* ===== Schedule Overlay ===== */
.drm-schedule {
  background: #1a1a1a;
  border-bottom: 1px solid #444;
  font-size: 11px;
  color: #ccc;
  padding: 8px;
}

.drm-schedule__layout {
  display: grid;
  grid-template-columns: 180px 1fr 140px;
  gap: 8px;
}

.drm-schedule__left {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.drm-schedule__checks {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}

.drm-schedule__check {
  display: flex;
  align-items: center;
  gap: 3px;
  font-size: 10px;
  cursor: pointer;
}

.drm-schedule__check input {
  accent-color: #4CAF50;
}

.drm-schedule__services-title {
  font-weight: 600;
  color: #aaa;
  font-size: 10px;
}

.drm-schedule__services-list {
  list-style: none;
  padding: 0;
  margin: 4px 0 0;
  font-size: 10px;
  color: #888;
}

.drm-schedule__center {
  position: relative;
  min-height: 80px;
}

.drm-schedule__station {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 2px 0;
  font-size: 10px;
  border-bottom: 1px solid #333;
}

.drm-schedule__station-info {
  color: #42a5f5;
  cursor: pointer;
}

.drm-schedule__station-name {
  flex: 1;
  color: #ddd;
}

.drm-schedule__station-time {
  display: flex;
  gap: 2px;
  width: 80px;
}

.drm-schedule__bar {
  height: 6px;
  border-radius: 2px;
}

.drm-schedule__bar--green { background: #4CAF50; }
.drm-schedule__bar--pink { background: #e91e63; }

.drm-schedule__right {
  display: flex;
  flex-direction: column;
  gap: 4px;
  align-items: flex-end;
}

.drm-schedule__time {
  font-size: 11px;
  font-weight: 600;
  color: #aaa;
}

.drm-schedule__select {
  background: #333;
  border: 1px solid #555;
  color: #ccc;
  font-size: 10px;
  padding: 2px 4px;
}

.drm-schedule__legend {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.drm-schedule__legend-item {
  font-size: 9px;
  padding: 0 4px;
  border-radius: 2px;
}

.drm-schedule__legend-item--verified { color: #4CAF50; }
.drm-schedule__legend-item--not-verified { color: #e91e63; }

/* ===== Decoder Panel ===== */
.drm-decoder {
  position: absolute;
  bottom: 0;
  left: 8px;
  width: 260px;
  background: #222;
  border: 1px solid #444;
  border-radius: 4px;
  font-size: 11px;
  color: #ccc;
  z-index: 25;
  box-shadow: 0 2px 8px rgba(0,0,0,0.5);
}

.drm-decoder__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 8px;
  background: #2a2a2a;
  border-bottom: 1px solid #444;
  border-radius: 4px 4px 0 0;
}

.drm-decoder__title {
  color: #00bcd4;
  font-weight: 600;
  font-size: 11px;
}

.drm-decoder__help {
  color: #4CAF50;
  cursor: pointer;
  font-size: 10px;
}

.drm-decoder__close {
  color: #888;
  cursor: pointer;
  font-size: 12px;
}

.drm-decoder__content {
  padding: 8px;
  max-height: 100px;
  overflow-y: auto;
  font-size: 10px;
  line-height: 1.5;
}

.drm-decoder__content a {
  color: #42a5f5;
  text-decoration: none;
}

.drm-decoder__content a:hover {
  text-decoration: underline;
}

.drm-decoder__footer {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  padding: 6px 8px;
  border-top: 1px solid #444;
  align-items: center;
}

.drm-decoder__btn {
  border: 1px solid #555;
  background: #333;
  color: #ddd;
  font-size: 10px;
  padding: 3px 8px;
  border-radius: 3px;
  cursor: pointer;
}

.drm-decoder__btn--stop {
  background: #c62828;
  border-color: #b71c1c;
  font-weight: 600;
}

.drm-decoder__btn--iq {
  background: #6a1b9a;
  border-color: #4a148c;
}

.drm-decoder__btn--test {
  background: #00838f;
  border-color: #006064;
}

.drm-decoder__lpf {
  display: flex;
  align-items: center;
  gap: 3px;
  font-size: 10px;
  margin-left: auto;
  cursor: pointer;
}

.drm-decoder__lpf input {
  accent-color: #4CAF50;
}
</style>