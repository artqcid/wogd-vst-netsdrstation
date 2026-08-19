---
name: nbml-patch-auth
description: Check and ensure that the multi-profile patch (NOTEBOOKLM_MCP_DATA_DIR) is applied in auth.py of the notebooklm-mcp-server package. Use this skill when NotebookLM MCP servers stop working after a uv tool upgrade or when NOTEBOOKLM_MCP_DATA_DIR is ignored.
---

# Patch NotebookLM Auth for Multi-Profile Support

This skill checks/ensures that the `NOTEBOOKLM_MCP_DATA_DIR`
environment-variable patch is applied to `auth.py`. Required after an update
of the `notebooklm-mcp-server` package (e.g. via `uv tool upgrade`).

## When to Run

- After `uv tool upgrade notebooklm-mcp-server`
- When the `notebooklm_devblogs` MCP stops working because
  `NOTEBOOKLM_MCP_DATA_DIR` is no longer respected
- On auth errors even though the reauth (skill `nbml-reauth`) was successful

## Steps

1. Check whether the patch is already applied:
```powershell
Select-String -Path "C:\Users\marku\AppData\Roaming\uv\tools\notebooklm-mcp-server\Lib\site-packages\notebooklm_mcp\auth.py" -Pattern "NOTEBOOKLM_MCP_DATA_DIR"
```
   If found → the patch is active, stop here.

2. Apply the patch – replace `get_cache_path` in `auth.py`.

**File:** `C:\Users\marku\AppData\Roaming\uv\tools\notebooklm-mcp-server\Lib\site-packages\notebooklm_mcp\auth.py`

**Before:**
```python
def get_cache_path() -> Path:
    """Get the path to the auth cache file."""
    cache_dir = Path.home() / ".notebooklm-mcp"
    cache_dir.mkdir(exist_ok=True)
    return cache_dir / "auth.json"
```

**After:**
```python
def get_cache_path() -> Path:
    """Get the path to the auth cache file."""
    data_dir = os.environ.get("NOTEBOOKLM_MCP_DATA_DIR")
    if data_dir:
        cache_dir = Path(data_dir)
    else:
        cache_dir = Path.home() / ".notebooklm-mcp"
    cache_dir.mkdir(parents=True, exist_ok=True)
    return cache_dir / "auth.json"
```

3. Verify:
```powershell
Select-String -Path "C:\Users\marku\AppData\Roaming\uv\tools\notebooklm-mcp-server\Lib\site-packages\notebooklm_mcp\auth.py" -Pattern "NOTEBOOKLM_MCP_DATA_DIR"
```
   Must return a hit.

4. Afterwards restart OpenCode so the MCP server is restarted with the patch.

## Profile Mapping (Reference)

| MCP Name | Google Account | Auth Dir |
|----------|----------------|----------|
| `notebooklm_devblogs` | markus.wagner.devblogs@gmail.com | `C:\Users\marku\.notebooklm-mcp\profiles\devblogs` |
| `notebooklm_privat` | markus.wagner.privat@gmail.com | `C:\Users\marku\.notebooklm-mcp\profiles\privat` |
| `notebooklm_kiff` | markus.wagner@kiff.ch | `C:\Users\marku\.notebooklm-mcp\profiles\kiff` |
| `notebooklm_aaura` | aaurahcpunkt@gmail.com | `C:\Users\marku\.notebooklm-mcp\profiles\aaura` |