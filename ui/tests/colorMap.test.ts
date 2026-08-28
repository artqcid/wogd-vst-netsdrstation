import { describe, it, expect } from 'vitest'

import { colorFor } from '@/components/waterfall/colorMap'

describe('colorMap', () => {
  it('maps silence (-160 dBFS) to the dark end', () => {
    const [r, g, b] = colorFor(-160, 'default')
    expect(r).toBeLessThan(80)
    expect(b).toBeGreaterThan(0)
  })

  it('maps full-scale (0 dBFS) to the hot end (red)', () => {
    const [r] = colorFor(0, 'default')
    expect(r).toBeGreaterThan(200)
  })

  it('grayscale produces equal channels', () => {
    const [r, g, b] = colorFor(-80, 'grayscale')
    expect(r).toBe(g)
    expect(g).toBe(b)
  })

  it('clamps values outside the range', () => {
    const lo = colorFor(-999, 'rain')
    const hi = colorFor(999, 'rain')
    expect(lo).toBeDefined()
    expect(hi).toBeDefined()
  })

  it('rain map differs from default at mid scale', () => {
    const def = colorFor(-80, 'default')
    const rain = colorFor(-80, 'rain')
    expect(def.join(',')).not.toBe(rain.join(','))
  })
})