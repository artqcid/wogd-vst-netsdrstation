# Plan History — NetSDRStation-VST

_Historical status blocks extracted from plan.md during LLM-Wiki refactoring
(2026-08-29). These entries are preserved for reference but no longer duplicated
in plan.md, which now describes only the current plan state._

---

## M3 Status History (2026-08-22 through 2026-08-27)

> **Status 2026-08-22:** implemented. M3.1–M3.4, M3.6, M3.7 done (details in
> `doc/checklist.md` M3). The RT-safety defect that made the pipeline test flaky
> is fixed (allocation-free ring-buffer jitter buffer + bounded resampler +
> prefill start latch). Automated verification green: 86 C++ test cases
> (676k assertions), Vitest 28/28, Playwright smoke, Debug+Release builds.
> M3.5 (manual acceptance against the real KiwiSDR at `g8ure.ddns.net:8078`)
> is the only open item — it requires a DAW/host and network access.
>
> **Status 2026-08-26 (post-M3 defects resolved):**
> - BUG-03 (Connect button): fix implemented (ix::initNetSystem + status feedback channel).
> - BUG-04 (Winsock include-order): fix implemented (WIN32_LEAN_AND_MEAN in diag.h).
> - BUG-05 (missing dependsOn): fix implemented (dependsOn + dependsOrder in tasks.json).
> - **F2 (KiwiSDR-Verbindung):** fix implemented (2026-08-26) — Auth-first handshake on SND stream, Phase 2 command sequence, 20-byte SND header stripping. Verified via Python test + SDRAngel reference.
> - **New M3 defects (in investigation):**
>   - F3: Connection closes after ~5 s (code=1005) — keepalive timing issue suspected.
>   - F4: Audio choppy with volume fluctuations — multiple concatenated SND frames per binary message not handled; ADPCM decoder state continuity issue.
> - All documented in `doc/checklist.md`.
>
> **Status 2026-08-27 (FIX-40 root cause + implementation):**
> - F3/FIX-38/FIX-39 (Keepalive-Throttling, Clock-Drift-Richtung, inactivity_timeout-
>   Befehl) sind behoben und getestet (82/82 Debug+Release). Der Nutzer testet immer
>   Debug, nie Release.
> - Korrigierte Root-Cause (2026-08-27, nach Rechnung): ~5,5 SND-Frames/s ist die
>   KORREKTE Frame-Rate für 12 kHz / 1034-Byte-Frames. Keepalive-Timer als eigener
>   1-Hz-Thread, unabhängig von Audio-Frames.
>
> **Status 2026-08-27 (M3 Blocker-Analyse):**
> - FIX-41 (n_snd=0) identifiziert als kritischer M3-Blocker.
> - Root Cause: Fehlender/falscher Befehl in der Client-Init-Sequenz.
>
> **Status 2026-08-27 (M3 abgeschlossen — FIX-41 behoben):**
> - FIX-41 Root-Cause gefunden & behoben: n_snd=0 war ein Bug im Python-Probe.
> - Live-Verifikation: korrigierte Probe gegen kphsdr.com:8073 → SND-Frames fließen.
> - M3-Status: M3.1–M3.7 abgeschlossen. Test-Suite 92/92 grün (Debug + Release),
>   Validator 47/47. M3.5: Manual Acceptance bestanden.

## M2 Status History

> M2 delivered the KiwiSDR building blocks as unit/integration-tested
> libraries, but did not wire them into the processor. The actual receiver
> pipeline, the full UI and the station selection follow in M3–M5 (see
> `doc/checklist.md` and `doc/ui-architecture.md`).
