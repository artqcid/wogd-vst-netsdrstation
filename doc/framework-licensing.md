# Framework & DSP Licensing Analysis

_Web research result. Constraint: a VST/DSP framework may be used ONLY if it is
license-free (no fees) AND the plugin can be sold WITHOUT open-sourcing the
source code (closed-source commercial use)._

## Verdict Summary

| Framework | Type | License | Closed-source commercial OK? | Suitable? |
|-----------|------|---------|------------------------------|-----------|
| VST3 SDK (Steinberg) | Plugin format SDK | MIT (2026) | Yes | YES (recommended) |
| CLAP SDK (free-audio) | Plugin format SDK | MIT | Yes | YES |
| DPF (DISTRHO) | Full framework | ISC | Yes | YES (alternative) |
| iPlug2 | Full framework | zlib-like | Yes | YES (alternative) |
| VSTGUI (Steinberg) | Native UI framework | BSD-3-Clause | Yes | Optional |
| WDL (Cockos) | Library | zlib | Yes | Optional |
| JUCE | Full framework | AGPLv3 + commercial | NO (paid) | NO |
| KFR | DSP framework | GPLv2 + commercial | NO (paid) | NO |
| HISE | Sampler framework | GPLv3 + commercial | NO (paid) | NO |

## Details

### Suitable (permissive, closed-source commercial allowed)

- **VST3 SDK (Steinberg)** — LICENSE.txt is **MIT** as of 2026 (Copyright 2026
  Steinberg Media Technologies GmbH). Historically dual GPLv3 + Steinberg
  proprietary; the MIT relicensing removes the open-source obligation. This is
  the primary plugin-format SDK for the project.
- **CLAP SDK (free-audio/clap)** — MIT. The modern open plugin standard
  (Bitwig + u-he). Can be added as an extra format next to VST3/AU.
- **DPF (DISTRHO Plugin Framework)** — ISC (very permissive). Full framework
  (VST2/VST3/CLAP/LV2), no GUI layer imposed; fine for closed-source.
- **iPlug2** — zlib-like ("free to use in closed source projects"). Full
  framework (VST2/VST3/AU/AAX/CLAP). Its IGraphics/NanoVG GUI layer is not
  needed here (we use Vue/WebView).
- **VSTGUI (Steinberg)** — BSD-3-Clause. Native C++ GUI framework; optional if
  a non-WebView fallback UI is ever needed.

### NOT suitable (require a paid license for closed-source)

- **JUCE** — AGPLv3 + commercial JUCE licence. Closed-source requires a paid
  licence (and the project also excludes JUCE by design).
- **KFR** — GPLv2+ + commercial license. Closed-source requires purchase.
- **HISE** — GPLv3 + commercial licence. Closed-source requires purchase.

## Permissive DSP libraries (supplementary, if needed)

| Library | Purpose | License |
|---------|---------|---------|
| libsamplerate | sample-rate conversion | BSD-2-Clause (already in stack) |
| STK (Synthesis Toolkit) | synthesis/effects | permissive (CCRMA) |
| SoLoud | audio engine | zlib/libpng |
| Maximilian | audio/DSP (cross-platform) | MIT |
| PFFFT | FFT | BSD |
| moodycamel ReaderWriterQueue | lock-free SPSC queue | BSD-2-Clause (already in stack) |

## Recommendation

Stay with the current architecture (all permissive, closed-source OK):

> VST3 SDK (MIT) + webview/webview (MIT) + Vue 3/Vite (MIT) + IXWebSocket
> (BSD-3) + moodycamel ReaderWriterQueue (BSD-2) + libsamplerate (BSD-2).

No JUCE, no KFR, no HISE. If a higher-level full framework is ever wanted,
**iPlug2** (zlib) or **DPF** (ISC) are the only candidates — both allow
closed-source commercial use.

## Caveats

- The VST3 SDK MIT relicensing is recent (2026); verify the exact LICENSE.txt
  at adoption time. The "VST" name/logo has separate trademark guidelines that
  do not affect source licensing.
- CLAP is an optional extra target (project goal lists VST3/AU/CLAP).

## Orienting on JUCE (allowed, with limits)

It is allowed and encouraged to **orient** on the solutions and architecture of
https://github.com/juce-framework/JUCE. However, JUCE itself is AGPLv3 +
commercial, so the code must NOT be copied. Guidance:

- **Allowed (ideas are not copyrightable):**
  - Study JUCE's *concepts/architecture*: Processor/Editor split, parameter
    abstraction, message-thread vs. audio-thread separation, lock-free
    real-time constraints, unit-testable DSP core, component orientation.
  - Reimplement these ideas independently (own names, own structure, own
    comments).
  - Document when a solution is "inspired by JUCE's approach to X".
- **Not allowed (copyright / AGPLv3 derivative risk):**
  - Copying JUCE source code (verbatim or near-verbatim) into this repo.
  - Porting/translating JUCE classes (AudioProcessor, AudioParameter*, etc.).
  - Reusing JUCE's expression: comments, identifiers, structure, or code layout.
- **Note:** most of JUCE's plugin architecture is actually dictated by the
  **VST3 SDK** (MIT), not JUCE. Those interfaces (`Steinberg::Vst::*`) can be
  used directly and legally.
- **Rule of thumb:** read JUCE for the "what/why", write our own "how". When in
  doubt, keep the implementation independent and note the inspiration.

_This is a practical/technical assessment, not formal legal advice. For the
specific commercial use, a final check with a lawyer is recommended._
