# Agent Instructions for libsombrero

## Building and Testing

### C Code

libsombrero uses CMake for building the C library.

**Building:**

```bash
# The build directory must always be outside the source directory
cmake -S . -B ../build -DCMAKE_BUILD_TYPE=Release
cmake --build ../build -j$(nproc)
```

**Testing:**
Unit tests are executed using CTest or the provided test script.

Using CTest:
```bash
cmake --build ../build --target test
```

Using the test script (for image output validation):
```bash
cd ../build/tests
./run_tests.sh
```

### Python Code

Sombrero includes a ctypes Python wrapper for the C library.

**Building the Python Package:**
To build the Python package (wheel/sdist):
```bash
cmake --build ../build --target python-package
```

**Testing:**
To test the Python bindings, run the test script:
```bash
python3 test_wrapper.py
```

## Commit Message Guidelines

When making commits, ensure you strictly follow these rules:

- **Line Length**: The overall commit message must be at most 80 characters per line.
- **Subject**: The subject line must follow the format `feature: description`.
- **Body**: The commit message body must properly describe the change and why it was made.
- **Sign-off**: Every commit must include a sign-off line using the developer's name and email configured in git (e.g., retrieve via `git config user.name` and `git config user.email`).

### Example Commit Message

```text
core: optimize wavelet transform performance

This change implements AVX2 intrinsics for the 2D wavelet transform convolution
step, improving overall processing speed by 15%.

Signed-off-by: Developer Name <developer@example.com>
```
