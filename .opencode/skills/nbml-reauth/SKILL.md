---
name: nbml-reauth
description: Perform the re-authentication of the NotebookLM devblogs profile when the Google session has expired and the MCP server reports an auth error (needs_auth). Use this skill exclusively when an auth error occurs.
---

# NotebookLM Re-Auth (devblogs)

This skill is used when the profile `devblogs` reports the status `needs_auth`
(e.g. auth errors on `notebook_list` or `notebook_query`).

> [!CAUTION]
> Always specify the profile `devblogs`. Do not run it without a profile argument –
> this would unnecessarily open multiple Chrome instances.
> Do NOT stop the running MCP server.

## Steps

### 1. Run the Reauth Script

```powershell
$uvPython = "C:\Users\marku\AppData\Roaming\uv\tools\notebooklm-mcp-server\Scripts\python.exe"
$reauthScript = "C:\Users\marku\.gemini\antigravity\scripts\reauth_nblm.py"
& $uvPython $reauthScript devblogs
```

### 2. Perform the Login

1. A **visible** Chrome window opens.
2. The script automatically navigates to the Google login page and pre-fills
   `markus.wagner.devblogs@gmail.com`.
3. If necessary: perform the login with a password.
4. Wait until NotebookLM is fully loaded.
5. The script detects that automatically, extracts the tokens, saves them
   and closes Chrome by itself.

### 3. Reload the Tokens in the MCP Server

After a successful reauth, the new tokens must be loaded in the **running**
MCP server. Call `refresh_auth` on the `notebooklm_devblogs` server
(e.g. `mcp__notebooklm_devblogs__refresh_auth`).

After that the session is active immediately – no restart needed.

## Troubleshooting

- If the reauth script fails: check the `reauth_nblm.py` path (the file is at
  `C:\Users\marku\.gemini\antigravity\scripts\`).
- If `refresh_auth` still fails: check whether the
  multi-profile patch is present in `auth.py` (skill `nbml-patch-auth`).