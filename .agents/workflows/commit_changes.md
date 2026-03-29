---
description: How to commit changes
---

# Committing Changes

When making commits to the repository, you must strictly follow these rules to maintain git history quality:

1. **Get Permission**: You must ALWAYS ask for the user's permission before executing any `git commit` commands. Do not commit autonomously.
2. **Verify Tests**: Ensure that 100% of the relevant unit tests (both C and Python) compile and pass successfully.
3. **Clean Workspace**: Check `git status` and rigorously delete any temporary, build, or generated files (`egg-info`, `__pycache__`, `build/`) before committing.
4. **Author/Sign-off Verification**: Retrieve the user's git configuration name and email:
   // turbo
   ```bash
   git config user.name && git config user.email
   ```
5. **Format Commit Message**:
   - **Line Length**: The overall commit message must be at most 80 characters per line.
   - **Subject**: The subject line must follow the format `feature: description`.
   - **Body**: The commit message body must properly describe the change and why it was made.
   - **Sign-off**: Every commit must include a sign-off line using the developer's name and email exactly as configured in git.
   
   Example format:
   ```text
   core: optimize wavelet transform performance

   This change implements AVX2 intrinsics for the 2D wavelet transform convolution
   step, improving overall processing speed by 15%.

   Signed-off-by: Developer Name <developer@example.com>
   ```

6. **Execute Commit**: Stage the relevant changes via `git add` and create the commit adhering accurately to the required guidelines.
