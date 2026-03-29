---
description: How to modify Python code files
---

# Modifying Python Code

When making changes to Python source (`.py`) files in the repository (such as the ctypes wrapper or tests), you must follow this workflow to ensure code quality and consistent formatting:

1. **Make Changes**: Edit the Python code files as required to fulfill the request.
2. **Auto-Format**: Automatically format the modified Python files to maintain style consistency.
   // turbo

   ```bash
   black .
   ```

3. **Build the Python Package**: If you modified the ctypes wrapper, rebuild the python package.
   // turbo

   ```bash
   cmake --build ../build --target python-package
   ```

4. **Mandatory Testing**: After modifying or formatting Python code, you MUST run the Python tests to verify the repository remains in a healthy state.
   // turbo

   ```bash
   python3 test_wrapper.py
   ```

5. **Clean Workspace**: Check `git status` and rigorously delete any temporary or generated files such as `egg-info` or `__pycache__` before committing your final changes.
6. **Commit Changes**: You must ALWAYS ask the user for permission before running any `git commit` commands.
