# NetSDRStation-VST — Knowledge Index

_Karpathy-style LLM-Wiki catalog. Each entry: link + 1-line summary + category.
This is the deterministic entry point for LLM agents and humans navigating the
codebase knowledge. Update this index whenever a new concept file is added,
removed or renamed._

_See `log.md` for the chronological append-only changelog._

---

## Architecture & Design
- [`architecture.md`](./architecture.md) — Complete system architecture: problem statement, tech stack, threading model, protocol handshake, audio pipeline, conventions (384 lines)
- [`ui-architecture.md`](./ui-architecture.md) — UI architecture: KiwiSDR mirror inventory, component tree, parameter mapping, resizable layout (412 lines)
- [`coding-standards.md`](./coding-standards.md) — Clean Code Developer (CCD) coding rules and conventions
- [`framework-licensing.md`](./framework-licensing.md) — License analysis: all libraries must be permissive (MIT/BSD); JUCE/KFR/HISE excluded

## Plans & Roadmap
- [`plan.md`](./plan.md) — Draft plan: milestones M1–M5, architecture goals, open questions/risks
- [`checklist.md`](./checklist.md) — Open tasks only (short descriptions); the single source of truth for "what's next"

## Implementation Plans (per milestone)
- [`M3-implementation-plan.md`](./M3-implementation-plan.md) — M3 Integration & Ship: pipeline wiring, parameter set, RT safety (status: done)
- [`M4-implementation-plan.md`](./M4-implementation-plan.md) — M4 KiwiSDR UI parity: Vue 1:1 replica of the browser interface (status: in-progress)
- [`M5-implementation-plan.md`](./M5-implementation-plan.md) — M5 Station selection tab: SDR Stations / KIWI UI tabs (status: in-progress)
- [`M6-implementation-plan.md`](./M6-implementation-plan.md) — M6 Multi-Provider: OpenWebRX, SpyServer, Web-888 support beyond KiwiSDR (status: draft)

## UI / Frontend
- [`M4-ui-replication-analysis.md`](./M4-ui-replication-analysis.md) — Detailed analysis of KiwiSDR browser UI for 1:1 Vue replication
- [`reference-matrix.md`](./reference-matrix.md) — Reference matrix: live KiwiSDR elements ↔ PluginView 1:1 mapping
- [`M4b-bugs.md`](./archive/M4b-bugs.md) — M4 bug list & analysis (ARCHIV, Bugs in M4c gefixt)
- [`M4c.7-bugs.md`](./M4c.7-bugs.md) — M4c.7 Bug-Manifest: 15 Bugs + E2E-Lückenanalyse + Extensions-Planung (stand 2026-08-29; Bugs 1–6 gefixt, Bug 7–15 offen)
- [`plan-history.md`](./archive/plan-history.md) — Archived historical status blocks from plan.md (M2/M3 era)

## Protocol & Reference
- [`kiwisdr-protocol-reference.md`](./kiwisdr-protocol-reference.md) — Full KiwiSDR protocol: handshake, SND frames, keepalive, command reference

## Operations & Quality
- [`test-strategy.md`](./test-strategy.md) — Test strategy: automated first, CCD yellow/green, coverage targets
- [`logging-strategy.md`](./logging-strategy.md) — Two-level file logger strategy (INFO/DEBUG) with real-time safety
- [`workspace-workflow.md`](./workspace-workflow.md) — Build/debug/hot-reload workflow for all platforms

## Infrastructure
- [`station-list.md`](./station-list.md) — List of API-ready KiwiSDR stations (ext_api > 0)
- [`webview2-bundle-audit.md`](./webview2-bundle-audit.md) — WebView2 Fixed Version Runtime + bundle deployment audit

## Wiki Meta
- [`LLM-WIKI-Refactoring.md`](./archive/LLM-WIKI-Refactoring.md) — This wiki structure's own design document (Karpathy LLM-Wiki + Google OKF v0.2; all phases complete, archived)
- [`code_wiki.md`](./code_wiki.md) — Auto-generated symbol index (MCP-only, never read directly)
- [`log.md`](./log.md) — Chronological append-only changelog (newest first)