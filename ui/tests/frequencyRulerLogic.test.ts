import { describe, it, expect } from 'vitest'

import {
  ENV_BL,
  ENV_ATT,
  ENV_H1,
  ENV_H2,
  ENV_SLOPE,
  ENV_LINE_CLICK,
  ENV_ADJ,
  freqToPx,
} from '@/components/frequencyRulerLogic'

describe('frequencyRulerLogic constants (KiwiSDR openwebrx.js)', () => {
  it('ENV_BL is 5 (bounding line width)', () => {
    expect(ENV_BL).toBe(5)
  })

  it('ENV_ATT is 5 (attenuation slope width)', () => {
    expect(ENV_ATT).toBe(5)
  })

  it('ENV_H1 is 17 (trapezoid base y)', () => {
    expect(ENV_H1).toBe(17)
  })

  it('ENV_H2 is 5 (trapezoid roof y)', () => {
    expect(ENV_H2).toBe(5)
  })

  it('ENV_SLOPE is 10 (= ENV_BL + ENV_ATT)', () => {
    expect(ENV_SLOPE).toBe(10)
  })

  it('ENV_LINE_CLICK is 8 (carrier line click width)', () => {
    expect(ENV_LINE_CLICK).toBe(8)
  })

  it('ENV_ADJ is 20 (hit zone adjustment)', () => {
    expect(ENV_ADJ).toBe(20)
  })
})

describe('frequencyRulerLogic.freqToPx', () => {
  it('maps a frequency inside the window to the correct pixel', () => {
    // viewLow=0, viewHigh=30000kHz, width=1000px → 0.03333... kHz/px
    // freq=7000kHz → (7000-0)/30000 * 1000 = 233.33...
    const px = freqToPx(7000, 0, 30000, 1000)
    expect(px).toBeCloseTo(233.33, 0)
  })

  it('maps the low edge of the window to pixel 0', () => {
    expect(freqToPx(0, 0, 30000, 1000)).toBe(0)
  })

  it('maps the high edge of the window to the full width', () => {
    expect(freqToPx(30000, 0, 30000, 1000)).toBe(1000)
  })

  it('returns 0 when span is zero', () => {
    expect(freqToPx(7000, 100, 100, 1000)).toBe(0)
  })

  it('returns 0 when width is zero', () => {
    expect(freqToPx(7000, 0, 30000, 0)).toBe(0)
  })

  it('maps frequencies below the window to negative pixels', () => {
    expect(freqToPx(-100, 0, 30000, 1000)).toBeLessThan(0)
  })

  it('maps frequencies above the window to >width pixels', () => {
    expect(freqToPx(31000, 0, 30000, 1000)).toBeGreaterThan(1000)
  })
})
