# Suggested Commands

## Build & Test Workflows
- **Build Firmware**: `pio run -e cyd_28r` (default), `pio run -e cyd_28c`, `pio run -e cyd_35c` (or via Taskfile: `task build:cyd_28r`).
- **Run Host-Native Tests**: `pio test -e native` (or `rtk test pio test -e native`).
- **Compilation Database**: `pio run -t compiledb` (required for Clangd semantic navigation & LSP).
- **Clean Build Artifacts**: `pio run -t clean`.
- **Upload & Monitor**: `task uam:cyd_28r` or `pio run -e cyd_28r -t upload -t monitor`.
- **Design System Linting**: `task lint:design`.
- **Package Binaries for Testers**: `bash .agents/skills/package-binaries/package_binaries.sh cyd_28c`.

## Tool Rules & Command Wrappers
- **RTK Prefix**: Always prefix `git` and `gh` with `rtk` (e.g. `rtk git status`, `rtk git diff`, `rtk gh issue list | cat`).
- **Paging**: Always pipe GitHub CLI output to `cat` (e.g. `rtk gh pr status | cat`).
- **Inspection**: Use `rtk ls`, `rtk tree`, `rtk read`, and `rtk grep` for token-optimized inspection.