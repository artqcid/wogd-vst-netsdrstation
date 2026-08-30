import { describe, it, expect } from 'vitest';
import {
  viewCenterKhz,
  cursorDrag,
  panWindow,
  clampFreq,
} from '@/components/cursorPanLogic';

describe('cursorPanLogic', () => {
  describe('viewCenterKhz', () => {
    it('returns freqKhz + panOffsetKhz', () => {
      expect(viewCenterKhz(7000, 0)).toBe(7000);
      expect(viewCenterKhz(7000, 500)).toBe(7500);
      expect(viewCenterKhz(7000, -300)).toBe(6700);
    });
  });

  describe('clampFreq', () => {
    it('clamps low value to 0.001', () => {
      expect(clampFreq(0)).toBe(0.001);
    });

    it('clamps high value to 30000', () => {
      expect(clampFreq(40000)).toBe(30000);
    });

    it('keeps value within range unchanged', () => {
      expect(clampFreq(7000)).toBe(7000);
    });
  });

  describe('cursorDrag', () => {
    it('moves cursor by dFreq but keeps window centre fixed', () => {
      const result = cursorDrag(7000, 0, 100);
      expect(result).toEqual({ freqKhz: 7100, panOffsetKhz: -100 });
      // Window centre must stay unchanged
      expect(viewCenterKhz(7100, -100)).toBe(7000);
    });

    it('handles negative dFreq direction', () => {
      const result = cursorDrag(7000, 0, -100);
      expect(result).toEqual({ freqKhz: 6900, panOffsetKhz: 100 });
    });

    it('respects clampFreq on cursor drag', () => {
      const result = cursorDrag(29950, 0, 200);
      expect(result.freqKhz).toBe(30000);
      expect(result.panOffsetKhz).toBe(-200);
      // Window centre with clamped freq
      expect(viewCenterKhz(30000, -200)).toBe(29800);
    });
  });

  describe('panWindow', () => {
    it('moves the window but keeps the cursor at its absolute frequency', () => {
      const result = panWindow(7000, 0, 100);
      expect(result).toEqual({ freqKhz: 7000, panOffsetKhz: 100 });
      // Window centre moves by dFreq
      expect(viewCenterKhz(7000, 100)).toBe(7100);
    });

    it('handles non-zero panOffset', () => {
      const result = panWindow(7000, 500, -50);
      expect(result).toEqual({ freqKhz: 7000, panOffsetKhz: 450 });
    });
  });
});
