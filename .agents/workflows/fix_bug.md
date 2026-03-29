---
description: How to fix a bug
---

# Fixing a Bug

When fixing a bug in the project, follow this systematic workflow to ensure the root cause is resolved and regression is prevented:

1. **Reproduce the Bug**: Write a reproduction script, command, or test that reliably triggers the bug. Confirm the failing behavior.
2. **Identify Root Cause**: Analyze the system, trace the execution, and diagnose the underlying cause of the bug. Include a brief summary in your commit message.
3. **Implement the Fix**: Modify the underlying C, Python, or configuration files to address the root cause.
4. **Mandatory Regression Testing**: Verify the bug is fixed using the reproduction from step 1, AND add a permanent test case for it (either a C unit test via `ctest` or a Python test in `test_wrapper.py`).
5. **Run All Tests**: Ensure 100% of the repository's tests are passing. Do not finish the task or consider it accepted if there are any failing tests.
6. **Clean Workspace**: Remove any temporary test scripts, `__pycache__`, or intermediate build artifacts from the workspace.
7. **Commit Changes**: Once all tests pass, explicitly ask the user for permission before executing any `git commit` commands.
