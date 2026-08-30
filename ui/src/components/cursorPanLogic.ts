/**
 * Pan/Cursor separation logic (KiwiSDR-konform).
 *
 * In KiwiSDR's frequency display, two independent concepts coexist:
 *
 *   - The *cursor* (a movable marker whose position is given in kHz).
 *   - The *pan offset*, which shifts the visible window independently of the cursor.
 *
 * The **window centre** is the reference point of the visible spectrum and is
 * computed as:
 *
 *     viewCenterKhz = freqKhz + panOffsetKhz
 *
 * This means the window centre is *decoupled* from the cursor position: the
 * cursor sits at `freqKhz` kHz on the scale, while the visible window is centred
 * at `viewCenterKhz`.  Dragging the cursor moves `freqKhz` and automatically
 * compensates `panOffsetKhz` so that the window centre remains unchanged.  Panning
 * the window moves `freqKhz` only, so both window centre and cursor travel together.
 *
 * The two exported operations implement exactly this separation:
 *
 *   - `cursorDrag`: keep the window centre fixed, move the cursor (freqKhz) and
 *     compensate panOffsetKhz accordingly.
 *   - `panWindow`: move the window only — `freqKhz` (the cursor position) stays at
 *     its absolute frequency, `panOffsetKhz` changes so the visible window moves
 *     underneath the cursor (KiwiSDR behaviour).
 *
 * Both operations clamp the resulting freqKhz into the valid KiwiSDR range
 * `[0.001, 30000]` kHz.
 */

/** View window centre = freqKhz + panOffsetKhz. */
export function viewCenterKhz(freqKhz: number, panOffsetKhz: number): number {
  return freqKhz + panOffsetKhz
}

/**
 * Cursor drag keeps the WINDOW CENTRE fixed:
 *   freqKhz'      = clampFreq(freqKhz + dFreq)
 *   panOffsetKhz' = panOffsetKhz - dFreq
 */
export function cursorDrag(
  freqKhz: number,
  panOffsetKhz: number,
  dFreq: number,
): { freqKhz: number; panOffsetKhz: number } {
  return {
    freqKhz: clampFreq(freqKhz + dFreq),
    panOffsetKhz: panOffsetKhz - dFreq,
  }
}

/**
 * Pan moves the WINDOW only (KiwiSDR): the cursor stays at its absolute
 * frequency, the window centre moves by dFreq:
 *   freqKhz'      = clampFreq(freqKhz)          (cursor unchanged)
 *   panOffsetKhz' = panOffsetKhz + dFreq        (window moves)
 */
export function panWindow(
  freqKhz: number,
  panOffsetKhz: number,
  dFreq: number,
): { freqKhz: number; panOffsetKhz: number } {
  return {
    freqKhz: clampFreq(freqKhz),
    panOffsetKhz: panOffsetKhz + dFreq,
  }
}

/** Clamp to [0.001, 30000] kHz. */
export function clampFreq(f: number): number {
  return Math.max(0.001, Math.min(30000, f))
}
