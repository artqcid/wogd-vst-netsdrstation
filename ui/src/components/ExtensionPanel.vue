<template>
  <KPanel title="Extensions" class="kiwi-panel" data-testid="extension-panel">
    <KSelect
      :model-value="selected"
      :options="extensionOptions"
      label="Extension"
      @update:model-value="onSelect"
    />
    <div class="extension-panel__content">
      <CwPanel v-if="selected === 'cw'" />
      <WfaxPanel v-else-if="selected === 'wfax'" />
      <RttyPanel v-else-if="selected === 'rtty'" />
      <SstvPanel v-else-if="selected === 'sstv'" />
      <TdoaPanel v-else-if="selected === 'tdoa'" />
      <IqPanel v-else-if="selected === 'iq'" />
      <AntennaPanel v-else-if="selected === 'antenna'" />
      <p v-else class="extension-panel__none">No extension selected.</p>
    </div>
  </KPanel>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import KPanel from '@/components/KPanel.vue'
import KSelect from '@/components/KSelect.vue'
import CwPanel from '@/components/extensions/CwPanel.vue'
import WfaxPanel from '@/components/extensions/WfaxPanel.vue'
import RttyPanel from '@/components/extensions/RttyPanel.vue'
import SstvPanel from '@/components/extensions/SstvPanel.vue'
import TdoaPanel from '@/components/extensions/TdoaPanel.vue'
import IqPanel from '@/components/extensions/IqPanel.vue'
import AntennaPanel from '@/components/extensions/AntennaPanel.vue'

const extensionOptions = [
  { value: 'cw', label: 'CW Decoder' },
  { value: 'wfax', label: 'WFAX' },
  { value: 'rtty', label: 'RTTY/FSK' },
  { value: 'sstv', label: 'SSTV' },
  { value: 'tdoa', label: 'tDoA' },
  { value: 'iq', label: 'IQ Display' },
  { value: 'antenna', label: 'Antenna Switch' },
]

const selected = ref('cw')

function onSelect(value: string | number) {
  selected.value = String(value)
}
</script>

<style scoped>
.extension-panel__content {
  margin-top: 8px;
}

.extension-panel__none {
  color: #888;
  font-size: var(--kiwi-font-sm, 11px);
  margin: 0;
}
</style>