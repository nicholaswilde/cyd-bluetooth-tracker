# Enforce Context-Mode MCP Tools

## Rule
Always use `context-mode` MCP tools instead of native tools for file inspection, command execution, and search. Invoke via `call_mcp_tool` with `ServerName: "context-mode"`.

## Tool Mappings & Capabilities
- **Command Execution & Diagnostics:**
  - Use `call_mcp_tool(ServerName: "context-mode", ToolName: "ctx_execute", Arguments: { language: "shell", code: "rtk <cmd>" })` instead of native `run_command`.
  - Prefix shell commands with `rtk` (e.g., `rtk git status`, `rtk test pio test -e native`, `rtk gh issue list | cat`) to combine RTK output filtering with context-mode sandboxing.
  - Native `run_command` is reserved only for interactive/background tasks (daemons, serial monitors) or when state mutation requires host process control.
- **File Inspection & Analysis:**
  - Use `call_mcp_tool(ServerName: "context-mode", ToolName: "ctx_execute_file", Arguments: { path: "...", language: "javascript", code: "..." })` instead of native `read_file` to inspect, summarize, filter, or extract information from files without dumping raw bytes into context.
  - Native `read_file` is reserved strictly for targeted exact-byte viewing immediately prior to an `edit_file` call.
- **Batch Exploration & Multi-Command Execution:**
  - Use `call_mcp_tool(ServerName: "context-mode", ToolName: "ctx_batch_execute", Arguments: { commands: [...], queries: [...] })` for broad repository discovery instead of consecutive shell calls.
- **Code & Knowledge Search:**
  - Use `call_mcp_tool(ServerName: "context-mode", ToolName: "ctx_search", Arguments: { queries: [...] })` to search indexed content and session history.
- **External Web Content:**
  - Use `call_mcp_tool(ServerName: "context-mode", ToolName: "ctx_fetch_and_index", Arguments: { url: "..." })` followed by `ctx_search` instead of raw web fetches.
