---
name: nbml-devblogs
description: Use NotebookLM with the devblogs Google account (markus.wagner.devblogs@gmail.com) via the NotebookLM MCP. Use this skill when you need to read notebooks, add sources, run chats, or create artifacts in Google NotebookLM.
---

# NotebookLM: devblogs (Workspace Integration)

In this workspace, the `notebooklm_devblogs` MCP server is automatically
registered and active via `.opencode/skills/../..` / `opencode.json`.

- **Account:** markus.wagner.devblogs@gmail.com
- **MCP Server Name:** `notebooklm_devblogs`
- **Profile data:** `C:\Users\marku\.notebooklm-mcp\profiles\devblogs`
- **Configuration source:** `opencode.json` in the workspace root (this project only)

## Usage

Use exclusively the tools of the `notebooklm_devblogs` MCP server.
The available tools are (called depending on the environment with a server
prefix, e.g. `mcp__notebooklm_devblogs__*` or `notebooklm_devblogs_*`):

- `notebook_list` – list all notebooks of the account
- `notebook_get` / `notebook_describe` – retrieve notebook details
- `notebook_query` – ask a question to a notebook (chat)
- `notebook_create` / `notebook_rename` / `notebook_delete` – manage notebooks
- `notebook_add_url` / `notebook_add_text` / `notebook_add_drive` – add sources
- `source_describe` / `source_get_content` / `source_list_drive` / `source_delete` – manage sources
- `source_sync_drive` – sync a Google Drive source
- `research_start` / `research_status` / `research_import` – run research
- `audio_overview_create` / `video_overview_create` / `studio_status` / `studio_delete` – studio overviews
- `infographic_create` / `slide_deck_create` / `report_create` / `flashcards_create` / `quiz_create` / `data_table_create` / `mind_map_create` – create artifacts
- `chat_configure` – configure chat behavior
- `refresh_auth` – reload auth tokens (after reauth)

## Auth Errors

If a tool call fails with an auth error (status `needs_auth`):

1. Start the skill `nbml-reauth` (re-authentication for profile `devblogs`).
2. Then call `refresh_auth` on the `notebooklm_devblogs` server.

## Notes

- Do not use other NotebookLM profiles (privat/kiff/aaura) in this workspace.
- The global Antigravity/MCP configs (`~\.gemini\...\mcp_config.json`)
  must NOT be modified by this workflow – the server runs isolated via the
  workspace `opencode.json`.
- If problems occur after a `uv tool upgrade notebooklm-mcp-server`:
  run the skill `nbml-patch-auth`.