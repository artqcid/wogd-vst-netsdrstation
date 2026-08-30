# KiwiSDR Bug 9 Research Report: BandScaleBar + TagArea

**Research Date:** 2026-08-29  
**Source:** KiwiSDR open-source code (https://github.com/jks-prv/KiwiSDR)  
**Files Analyzed:**
- `web/openwebrx/openwebrx.js` (13,856 lines)
- `web/kiwi/kiwi.js` (3,804 lines)  
- `web/kiwi/kiwi.css`
- `web/kiwi/w3_ext.css`

---

## 1. Source Files and Line Numbers

### openwebrx.js
| Function/Feature | Line Range | Description |
|-----------------|------------|-------------|
| `mk_bands_scale()` | 7885-7965 | Band rendering on canvas |
| `dx_label_render_cb()` | 8722-9155 | DX tag rendering and collision detection |
| `dx_label_step()` | 9256-9325 | Step through labels |
| `bands_init()` | 7492-7619+ | Initialize bands from config |
| `kiwi_bands_db()` | 7708-7717 | Band database accessor |
| `bands_addl_info()` | 7720-7777 | Augment band info |
| `band_svc_lookup()` | 7474-7488 | Look up band service by key |
| Constants (band_canvas_h, etc.) | 7863-7877 | Layout constants |
| `dx_color_init()` | 8584-8618 | Color initialization |

### kiwi.js
| Feature | Line | Description |
|---------|------|-------------|
| `kiwi.bands` | 175 | Band array (null initially) |
| `kiwi.bands_community` | 176 | Community band array |
| `kiwi.ITU_s` | 124-131 | ITU region strings |
| `kiwi.BAND_SCALE_ONLY` | 134 | Constant = 4 |
| `kiwi.BAND_MENU_ONLY` | 135 | Constant = 5 |

### kiwi.css / w3_ext.css
| CSS Class | File | Description |
|-----------|------|-------------|
| `.cl-dx-label` | kiwi.css:109-120 | DX tag label styling |
| `.cl-dx-line` | kiwi.css:122-126 | Connection line styling |
| `.cl-dx-sig` | kiwi.css:128-131 | Signature container |

---

## 2. Band Rendering (BandScaleBar)

### Method
**Canvas-based rendering** using `band_ctx` (a 2D canvas context).

The `mk_bands_scale()` function (line 7885) renders:
1. Clears canvas with white background (line 7898-7899)
2. Iterates through configured bands from `_dxcfg.bands` (line 7906)
3. For each band:
   - Calculates pixel positions from frequency range (line 7930)
   - Fills a rectangle with band color at 20% opacity (line 7937-7940)
   - Draws band name text centered in the bar (line 7946-7963)

### Band Colors
Colors come from the `band_svc` configuration via `band_svc_lookup()`:
- Each band has a `svc` key (e.g., 'A', 'B', 'C', etc.)
- `band_svc_lookup(svc_key)` returns the service object with `color` property
- Color is applied as `band_ctx.fillStyle = svc.o.color` (line 7937)
- Rendered with `globalAlpha = 0.2` (line 7938) for translucent overlay

**Default service colors** (from config.js / dx_config.json):
The actual color values are defined in the KiwiSDR admin configuration (`dx_config.json`), not hardcoded in the JS. The `band_svc` array contains objects like:
```javascript
{ key: 'A', name: 'AM', color: '#...' }
```

### Band Label Centering
Text is centered using:
```javascript
var tx = x + w/2;  // center x position
var txt = b2.longName;  // try long name first
var mt = band_ctx.measureText(txt);
if (w >= mt.width+4) {
    // long name fits
} else {
    txt = b1.name;  // fallback to short name
    mt = band_ctx.measureText(txt);
    if (w >= mt.width+4) {
        // short name fits
    } else {
        txt = null;  // too narrow, no text
    }
}
if (txt) band_ctx.fillText(txt, tx - mt.width/2, ty);
```
(lines 7946-7963)

### Layer
Band blocks are drawn **on the scale canvas** (`band_ctx`), not as separate DOM elements. The canvas is cleared and redrawn on each update.

---

## 3. DX Tags (TagArea)

### Tag Data Structure
Tags are received from the server as JSON arrays. Each tag entry contains:
```javascript
dx.list[gid] = {
    gid: gid,           // unique identifier
    carrier: carrier,   // carrier frequency
    lo: obj.lo,         // low passband cut
    hi: obj.hi,         // high passband cut
    freq: freq,         // frequency in Hz
    f_label: f_base_label_Hz,  // label position frequency
    mkr_off: mkr_off,   // marker offset
    sig_bw: obj.s,      // signal bandwidth
    flags: flags,       // DX flags (type, filtered, etc.)
    begin: obj.b,       // begin time
    end: obj.e,         // end time
    ident: ident,       // station identifier
    notes: notes,       // notes text
    params: params,     // extension parameters
    color: color,       // display color
    center: center      // is at center frequency?
};
```
(lines 8956-8957)

### Tag Rendering

**Shape:** Rectangular div elements with CSS class `cl-dx-label`

**DOM Structure:**
```javascript
var _class = w3_sb('w3-custom-events w3-hold cl-dx-label', 
    has_ext? 'dx-has-ext':'', filtered? 'cl-dx-label-filtered':'', 
    (has_ext && !filtered)? 'cl-dx-label-ext':'');

var _style_attr = sprintf('|left:%s; z-index:%d; background:%s|id="id-dx-label_%s"',
    px(x-10), dx_z, color, dx_idx);

s_a[dx_idx] = w3_button_path(_class + _style_attr, 'dx-'+ gid, '', 'dx_evt', 
    w3_sbc(',', gid, cmkr_x)) + 
    w3_div(sprintf('cl-dx-line|left:%s; z-index:110|id="id-dx-line_%s"', px(x), dx_idx));
```
(lines 8963-8969)

Each tag creates:
1. A button/div with class `cl-dx-label` (the tag itself)
2. A div with class `cl-dx-line` (the connection line)

### Connection Lines

**Implementation:** CSS-styled divs with class `cl-dx-line`

**CSS (kiwi.css lines 122-126):**
```css
.cl-dx-line {
    width: 1px;
    position: absolute;
    background-color: black;
    z-index: 110;
}
```

**Properties:**
- Width: 1px (solid line)
- Color: black
- Position: absolute, positioned at the tag's x coordinate
- Height: Set dynamically in JS (line 9112): `el_line.style.height = px(dx_container_h - top);`
- The line extends from the tag down to the frequency axis

### Two-Row Layout with Collision Detection

**Layout Algorithm:**

The two-row layout is implemented using alternating vertical positions:
```javascript
var gap = eibi? 40 : 35;  // line 8735

// For non-EiBi mode:
top = dx_label_top + (gap * (dx_idx & 1));  // line 8930
```

This creates alternating positions:
- Even indices (dx_idx & 1 == 0): `top = dx_label_top + 0`
- Odd indices (dx_idx & 1 == 1): `top = dx_label_top + gap`

**Constants:**
- `dx_label_top = 5` (line 7871)
- `gap = 35` (non-EiBi) or `40` (EiBi) (line 8735)
- `dx_container_h = 80` (line 7869)
- `dx_line_h = dx_container_h - dx_label_top = 75` (line 7872)

**Collision/Overlap Handling:**

The KiwiSDR code uses a **simple alternating row approach** rather than sophisticated collision detection:

1. **Basic alternating:** Tags are placed in two rows based on index parity
2. **Same-frequency optimization (EiBi only):** For EiBi database mode, when multiple tags share the same frequency base, they are optimized into a horizontal layout:
   ```javascript
   // line 8811-8858: optimize_eibi_label_layout()
   var spacing = 6;
   var n = Math.ceil(gap / spacing);
   // Tags at same frequency are spread horizontally with 6px spacing
   ```
3. **No explicit collision detection:** The code does NOT detect overlapping tags and adjust positions. It relies on the alternating row pattern.

**Minimum gap between overlapping tags:** 35px (non-EiBi) or 40px (EiBi), controlled by the `gap` variable.

**Tags on same frequency:** In EiBi mode, tags at the same `f_base_label_Hz` are grouped via `dx.f_same` array and laid out horizontally using `optimize_eibi_label_layout()`. In non-EiBi mode, no special merging occurs.

---

## 4. Raw Code Excerpts Verbatim

### mk_bands_scale() Function (openwebrx.js lines 7885-7965)

```javascript
function mk_bands_scale()
{
   //console.log('mk_bands_scale');
   var r = g_range;
   
   // band bars & station labels
   var tw = band_ctx.canvas.width;
   var i, x, y = band_scale_top, w, h = band_scale_h, ty = y + band_scale_text_top;
   var start = r.start;
   var end = r.end;
   //console.log('BB fftw='+ wf_fft_size +' tw='+ tw +' start='+ start +' end='+ end +' bw='+ (end - start));
   //console.log('BB pixS='+ scale_px_from_freq(r.start, g_range) +' pixE='+ scale_px_from_freq(r.end, g_range));
   band_ctx.globalAlpha = 1;
   band_ctx.fillStyle = "White";
   band_ctx.fillRect(0,band_canvas_top, tw,band_canvas_h);
   var ITU_region = cfg.init.ITU_region + 1;    // cfg.init.ITU_region = 0:R1, 1:R2, 2:R3

   var _dxcfg = dx_cfg_db();
   if (!_dxcfg) return;
   var _kiwi_bands = kiwi_bands_db(dx.INIT_BANDS_NO);

   for (i = 0; i < _dxcfg.bands.length; i++) {
      var b1 = _dxcfg.bands[i];
      var b2 = _kiwi_bands[i];

      // filter bands based on offset mode
      if ((kiwi.isOffset && !b2.isOffset) || (!kiwi.isOffset && b2.isOffset)) continue;
      if (!(b1.itu == kiwi.BAND_SCALE_ONLY || b1.itu == kiwi.ITU_ANY || b1.itu == ITU_region)) continue;
      //console.log('mk_bands_scale CONSIDER '+ b1.name +' R'+ b1.itu +' min='+ b1.min);
      
      var x1 = -1, x2;
      var bmin = b2.minHz, bmax = b2.maxHz;
      var min_inside = (bmin >= start && bmin <= end)? 1:0;
      var max_inside = (bmax >= start && bmax <= end)? 1:0;
      if (min_inside && max_inside) { x1 = bmin; x2 = bmax; } else
      if (!min_inside && max_inside) { x1 = start; x2 = bmax; } else
      if (min_inside && !max_inside) { x1 = bmin; x2 = end; } else
      if (bmin < start && bmax > end) { x1 = start; x2 = end; }
      //console.log('start='+ start +' end='+ end +' bmin='+ bmin +' bmax='+ bmax +' min_inside='+ min_inside +' max_inside='+ max_inside);

      if (x1 == -1) {
         //console.log('BANDS x1 == -1 SKIP');
         continue;
      }
   
      x = scale_px_from_freq(x1, g_range); var xx = scale_px_from_freq(x2, g_range);
      w = xx - x;
      //console.log("BANDS x="+ x1 +'/'+ x +" y="+ x2 +'/'+ xx +" w="+ w +' '+ ((w < 3)? 'TOO NARROW - SKIP':''));
      if (w < 3) continue;
      //console.log('BANDS SHOW');

      var svc = band_svc_lookup(b1.svc);
      band_ctx.fillStyle = svc? svc.o.color : 'grey';
      band_ctx.globalAlpha = 0.2;
      //console.log("BB x="+x+" y="+y+" w="+w+" h="+h);
      band_ctx.fillRect(x,y,w,h);
      band_ctx.globalAlpha = 1;

      //band_ctx.fillStyle = "Black";
      band_ctx.font = "bold 12px sans-serif";
      band_ctx.textBaseline = "top";
      var tx = x + w/2;
      var txt = b2.longName;
      var mt = band_ctx.measureText(txt);
      //console.log("BB mt="+mt.width+" txt="+txt);
      if (w >= mt.width+4) {
         // long name fits in bar
      } else {
         txt = b1.name;
         mt = band_ctx.measureText(txt);
         //console.log("BB mt="+mt.width+" txt="+txt);
         if (w >= mt.width+4) {
            // short name fits in bar
         } else {
            txt = null;
         }
      }
      //if (txt) console.log("BANDS tx="+(tx - mt.width/2)+" ty="+ty+" mt="+mt.width+" txt="+txt);
      if (txt) band_ctx.fillText(txt, tx - mt.width/2, ty);
   }
}
```

### Tag Rendering in dx_label_render_cb() (openwebrx.js lines 8926-8971)

```javascript
   // stagger the labels vertically
   if (!eibi && cfg.dx_three_high) {
      top = 26 * (dx_idx % 3);
   } else {
      top = dx_label_top + (gap * (dx_idx & 1));
   }
   dx.post_render[dx_idx] = { top: top, ltop: top, x: x /* , f: f_base_label_Hz/1e3, ident: ident */ };
   dx.last_f_base = f_base_label_Hz;

   var color;
   if (eibi) {
      color = filtered? dx.eibi_colors_light[color_idx] : dx.eibi_colors[color_idx];
   } else {
      var c = _dxcfg.dx_type[color_idx].color;
      if (c == '') c = 'white';
      color = filtered? dx.stored_colors_light[color_idx] : c;
   }

   // for backward compatibility, IBP color is fixed to aquamarine
   if (!eibi && color_idx == 0 && (ident == 'IBP' || ident == 'IARU%2fNCDXF')) {
      color = 'aquamarine';
   }
   console_log_lbl('DX '+ dx_idx +' f='+ freq +' k='+ mkr_off +' FL='+ flags.toHex(8) +
      ' m='+ kiwi.modes_uc[dx_decode_mode(flags)] +' <'+ ident +'> <'+ notes +'>');
   
   carrier = freq * 1000 - mkr_off;
   carrier /= 1000;
   var center = (x >= center_x1 && x <= center_x2)? 1:0;
   dx.list[gid] = { gid:gid, carrier:carrier, lo:obj.lo, hi:obj.hi, freq:freq, f_label:f_base_label_Hz, mkr_off:mkr_off, sig_bw:obj.s,
      flags:flags, begin:obj.b, end:obj.e, ident:ident, notes:notes, params:params, color:color, center:center };
   
   dx.displayed[dx_idx] = dx.list[gid];
   
   var has_ext = (params != '');
   
   var _class = w3_sb('w3-custom-events w3-hold cl-dx-label', has_ext? 'dx-has-ext':'',
      filtered? 'cl-dx-label-filtered':'', (has_ext && !filtered)? 'cl-dx-label-ext':'');
   var _style_attr = sprintf('|left:%s; z-index:%d; background:%s|id="id-dx-label_%s"',
      px(x-10), dx_z, color, dx_idx);
   s_a[dx_idx] =
      w3_button_path(_class + _style_attr, 'dx-'+ gid, '', 'dx_evt', w3_sbc(',', gid, cmkr_x)) +
      w3_div(sprintf('cl-dx-line|left:%s; z-index:110|id="id-dx-line_%s"', px(x), dx_idx));
   
   dx_z++;
```

### Tag Data Structure (openwebrx.js lines 8956-8957)

```javascript
dx.list[gid] = { gid:gid, carrier:carrier, lo:obj.lo, hi:obj.hi, freq:freq, f_label:f_base_label_Hz, mkr_off:mkr_off, sig_bw:obj.s,
   flags:flags, begin:obj.b, end:obj.e, ident:ident, notes:notes, params:params, color:color, center:center };
```

### EiBi Same-Frequency Optimization (openwebrx.js lines 8811-8858)

```javascript
var optimize_eibi_label_layout = function(x, z, f) {
   var i, j, k;
   var spacing = 6;
   var n = Math.ceil(gap / spacing);

   // postpone text width sorting until first label render establishes dx.font info for use by getTextWidth()
   if (dx.font) {
      dx.f_same.forEach(function(idx,i) {
         idx++;
         var o = arr[idx];
         o.di = kiwi_decodeURIComponent('', o.i);
         o.tw = getTextWidth(o.di, dx.font);
      });

      dx.f_same.sort(
         function(i1, i2) {
            var o1 = arr[i1+1];
            var o2 = arr[i2+1];
            return o2.tw - o1.tw;
         }
      );
   }
   
   for (i = 0; i <= dx.f_same.length; i++) {
      var idx = dx.f_same[i];
      j = i % (n+1);
      dx.post_render[idx] = {
         top: dx_label_top + (j * spacing),
         ltop: dx_label_top,
         x: x + j * (spacing - 1),
         z: z + i
      };
      if (j == n) {
         x += gap;
      }
   }
};
```

### Connection Line Height Setting (openwebrx.js lines 9109-9112)

```javascript
var el_line = w3_el('id-dx-line_'+ dx_idx);
top = sparse.ltop;
el_line.style.top = px(top);
el_line.style.height = px(dx_container_h - top);
```

### Layout Constants (openwebrx.js lines 7863-7877)

```javascript
band_canvas_h = 30;
band_canvas_top = 0;
   band_scale_h = 20;
   band_scale_top = 5;
      band_scale_text_top = 5;

dx_container_h = 80;
dx_container_top = band_canvas_h;
   dx_label_top = 5;
   dx_line_h = dx_container_h - dx_label_top;

scale_canvas_h = 47;
scale_canvas_top = band_canvas_h + dx_container_h;

scale_container_h = band_canvas_h + dx_container_h + scale_canvas_h;
```

### Gap Variable (openwebrx.js line 8735)

```javascript
var gap = eibi? 40 : 35;
```

### CSS Styles (kiwi.css lines 109-126)

```css
.cl-dx-label {
   position: absolute;
   /*font-family: Courier;*/
   font-size: 11px;
   /*font-weight: bold;*/
   padding: 3px;
   border: 1px solid black;
   border-radius: 3px !important;
   cursor: pointer;
   -moz-border-radius: 3px;
   /*background-color: cyan;*/
   z-index: 120;
}

.cl-dx-label.cl-dx-label-filtered {
   border: 1px dashed black;
}

.cl-dx-label.cl-dx-label-ext {
   /* border: 3px double black; */
   border-width: 1px 4px 1px 1px;
   border-color: black black black black;
}

.cl-dx-line {
   width: 1px;
   position: absolute;
   background-color: black;
   z-index: 110;
}
```

---

## 5. Summary of Key Constants

| Constant | Value | Location | Description |
|----------|-------|----------|-------------|
| `band_canvas_h` | 30 | line 7863 | Band canvas total height |
| `band_scale_h` | 20 | line 7865 | Band scale bar height |
| `band_scale_top` | 5 | line 7866 | Band scale top offset |
| `band_scale_text_top` | 5 | line 7867 | Band text top offset |
| `dx_container_h` | 80 | line 7869 | DX container total height |
| `dx_label_top` | 5 | line 7871 | DX label top offset |
| `dx_line_h` | 75 | line 7872 | DX line height (container - label_top) |
| `scale_canvas_h` | 47 | line 7874 | Scale canvas height |
| `gap` (non-EiBi) | 35 | line 8735 | Vertical gap between tag rows |
| `gap` (EiBi) | 40 | line 8735 | Vertical gap for EiBi mode |
| `spacing` (EiBi optimize) | 6 | line 8813 | Horizontal spacing for same-freq tags |

---

## 6. Key Findings for Bug 9 Implementation

1. **Band rendering** uses Canvas 2D API with `fillRect()` at 20% opacity
2. **Band colors** come from `dx_config.json` → `band_svc[].color`, not hardcoded
3. **Band labels** are centered text on canvas, with fallback from long name to short name to none
4. **DX tags** are absolutely-positioned div elements with CSS class `cl-dx-label`
5. **Connection lines** are 1px wide black divs with class `cl-dx-line`
6. **Two-row layout** uses alternating positions based on index parity (`dx_idx & 1`)
7. **No sophisticated collision detection** - relies on alternating row pattern
8. **Same-frequency tags** only get special handling in EiBi mode via `optimize_eibi_label_layout()`
9. **Tag height** is determined by CSS (font-size 11px + padding 3px + border 1px ≈ 19px)
10. **Vertical spacing** between rows is 35px (or 40px for EiBi)
