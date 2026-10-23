# Task Completion Validation

Run the following mandatory validation gates before declaring any task complete:

1. **Host-Native Unit Tests**:
   ```bash
   pio test -e native
   ```
   Must pass with 0 failures.

2. **Firmware Build**:
   ```bash
   pio run -e cyd_28r
   ```
   Must compile and link without errors or unresolved symbols. If touching capacitive configurations, verify `pio run -e cyd_28c` as well.

3. **Design System Linting** (if UI or color tokens modified):
   ```bash
   task lint:design
   ```

4. **Compilation Database Refresh** (if files or includes changed):
   ```bash
   pio run -t compiledb
   ```

5. **Git Review**:
   ```bash
   rtk git status
   rtk git diff
   ```
   Ensure no untracked temp files or accidental edits remain.