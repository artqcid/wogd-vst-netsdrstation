---
name: nbml-list
description: List all Google NotebookLM notebooks that are reachable via the notebooklm_devblogs MCP server in the current workspace. Use this skill when the user asks for available notebooks, project notebooks, or an overview of the notebooks.
---

# NotebookLM: List Notebooks

Call the tool `notebook_list` (or the equivalent, server-prefixed tool of the
`notebooklm_devblogs` server) to list all notebooks of the account
`markus.wagner.devblogs@gmail.com`.

## Steps

1. Call `notebook_list(max_results=100)`.
2. Show the found notebooks with title and ID.
3. If the call fails with an auth error: run the skill `nbml-reauth`
   and call `refresh_auth`, then retry `notebook_list`.

## Notes

- The notebook IDs are needed for `notebook_query`, `notebook_add_url`,
  `notebook_add_text`, etc.
- Only the `notebooklm_devblogs` server is used.