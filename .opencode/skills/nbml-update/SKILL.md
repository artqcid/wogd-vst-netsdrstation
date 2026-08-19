---
name: nbml-update
description: Update or extend a Google NotebookLM notebook with new content (text, URL, Google Drive source) via the notebooklm_devblogs MCP server. Use this skill when the user wants to save information, notes, URLs, or reports in a NotebookLM project notebook or consolidate an existing entry.
---

# NotebookLM: Update Notebook

Role: Automation assistant for the NotebookLM workflow of the workspace.
Prepare incoming information (texts, URLs, notes) for import into a
NotebookLM notebook and save it via the `notebooklm_devblogs`
MCP server.

## Target Mapping Logic

- **Default case:** If no specific notebook name is given at the end of the
  command, assign the content to the "Workspace Notebook". The workspace name
  is `wogd-vst-netsdrstation`.
- If it is unclear which notebook is meant: list notebooks via
  `notebook_list` that could match the project and actively ask.
- **Specific case:** If a name is given after the command
  (e.g. `--Notebook_Name`), use the mentioned notebook.

## Task

1. Extract the core content from the source.
2. Check existing project-specific notes/sources in the target notebook
   (via `notebook_describe` / `notebook_query`).
3. If the update concerns an existing note (architecture, roadmap,
   status): delete the old version (`source_delete`) and create a new,
   consolidated version.
4. Add the new content:
   - URL → `notebook_add_url(notebook_id, url)`
   - Text → `notebook_add_text(notebook_id, text, title)`
   - Drive → `notebook_add_drive(notebook_id, ...)`
5. Format the text cleanly as Markdown with a TL;DR summary
   as the first line.
6. At the end clearly state: `TARGET NOTEBOOK: [name of the notebook]`.

## Input Format

```
[content/URL] [optional notebook name]
```

## Notes

- Always use the `notebooklm_devblogs` server.
- Naming of project mind maps is always the workspace name of the project
  (here: `wogd-vst-netsdrstation`).