/**
 * Waterfall colour maps (M4.7): map a dBFS bin value (-160..0) to an RGB
 * colour. Simple per-map lookup tables per the M4 plan (Default, Rain,
 * Grayscale). Used by Waterfall.vue.
 */

export type ColorMapName = 'default' | 'rain' | 'grayscale'

/** Interpolates between two RGB stops. */
function lerp(a: number, b: number, t: number): number {
  return Math.round(a + (b - a) * t)
}

interface Rgb {
  r: number
  g: number
  b: number
}

const STOPS: Record<ColorMapName, { at: number; rgb: Rgb }[]> = {
  // KiwiSDR "Default": dark blue noise floor -> blue -> cyan -> green ->
  // yellow -> red for strong signals (standard SDR colourmap).
  default: [
    { at: 0.0, rgb: { r: 0, g: 0, b: 0 } },
    { at: 0.15, rgb: { r: 0, g: 0, b: 128 } },
    { at: 0.35, rgb: { r: 0, g: 0, b: 255 } },
    { at: 0.5, rgb: { r: 0, g: 255, b: 255 } },
    { at: 0.65, rgb: { r: 0, g: 255, b: 0 } },
    { at: 0.8, rgb: { r: 255, g: 255, b: 0 } },
    { at: 1.0, rgb: { r: 255, g: 0, b: 0 } },
  ],
  // Rain: violet -> blue -> green -> red
  rain: [
    { at: 0.0, rgb: { r: 120, g: 0, b: 180 } },
    { at: 0.33, rgb: { r: 0, g: 60, b: 220 } },
    { at: 0.66, rgb: { r: 0, g: 200, b: 80 } },
    { at: 1.0, rgb: { r: 240, g: 20, b: 0 } },
  ],
  grayscale: [
    { at: 0.0, rgb: { r: 0, g: 0, b: 0 } },
    { at: 1.0, rgb: { r: 255, g: 255, b: 255 } },
  ],
}

/** Maps a dBFS value (-160..0) to an RGB colour using the given map. */
export function colorFor(dBFS: number, map: ColorMapName): [number, number, number] {
  const t = Math.min(1, Math.max(0, (dBFS + 160) / 160))
  const stops = STOPS[map]
  const first = stops[0]
  if (!first) return [0, 0, 0]
  for (let i = 1; i < stops.length; ++i) {
    const a = stops[i - 1]
    const b = stops[i]
    if (!a || !b) continue
    if (t <= b.at) {
      const span = b.at - a.at
      const local = span > 0 ? (t - a.at) / span : 0
      return [lerp(a.rgb.r, b.rgb.r, local), lerp(a.rgb.g, b.rgb.g, local), lerp(a.rgb.b, b.rgb.b, local)]
    }
  }
  const last = stops[stops.length - 1]
  if (!last) return [0, 0, 0]
  return [last.rgb.r, last.rgb.g, last.rgb.b]
}