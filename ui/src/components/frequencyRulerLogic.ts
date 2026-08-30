/**
 * Pixel-geometry helpers and constants for the frequency-ruler cursor.
 *
 * Replaces the old frequency-based logic (`passbandVisible`, `clampLowCut`,
 * `clampHighCut`, `cursorHitZone`) with the KiwiSDR/OpenWebRX pixel-level
 * cursor shape from `openwebrx.js`.  FrequencyRuler.vue now owns the
 * frequency-range clamping inline.
 *
 * Reference constants (KiwiSDR `openwebrx.js`):
 *   ENV_BL = 5, ENV_ATT = 5, ENV_H1 = 17, ENV_H2 = 5,
 *   ENV_SLOPE = ENV_BL + ENV_ATT = 10,
 *   ENV_LINE_CLICK = 8, ENV_ADJ = 20.
 */

/** Bounding-line width (left/right foot of the trapezoid). */
export const ENV_BL = 5
/** Attenuation slope width. */
export const ENV_ATT = 5
/** y at the base / bottom of the trapezoid. */
export const ENV_H1 = 17
/** y at the top / roof of the trapezoid. */
export const ENV_H2 = 5
/** ENV_BL + ENV_ATT → 10. */
export const ENV_SLOPE = ENV_BL + ENV_ATT
/** Carrier-line click width in pixels. */
export const ENV_LINE_CLICK = 8
/** Hit-zone adjustment (padding around the trapezoid). */
export const ENV_ADJ = 20

/**
 * Map a frequency (kHz) to an x-pixel inside the ruler SVG.
 *
 * @param freqKhz    frequency in kHz
 * @param viewLowKhz low edge of visible window (kHz)
 * @param viewHighKhz high edge of visible window (kHz)
 * @param widthPx    ruler width in pixels
 */
export function freqToPx(
  freqKhz: number,
  viewLowKhz: number,
  viewHighKhz: number,
  widthPx: number,
): number {
  const span = viewHighKhz - viewLowKhz
  if (span <= 0 || widthPx <= 0) return 0
  return ((freqKhz - viewLowKhz) / span) * widthPx
}
