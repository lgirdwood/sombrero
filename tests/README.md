# Libsmbrr Unit Tests

This directory contains the unit testing framework and test suite for the `libsmbrr` (Sombrero) library. The suite is designed to prevent regressions and validate the integrity of image processing algorithms, performance levels, and package distributions.

## Running Tests

All unit tests are automatically compiled alongside the main library when configured via CMake.

### CMake (CTest)
You can run the full test suite using CMake's standard test driver, `ctest`. This is the recommended way to invoke the testing framework.

Run `ctest` from your `build/` directory:
```bash
# Run all tests 
ctest

# Run all tests verbosely
ctest -V
```

### Direct Bash Execution
For a more detailed output specific to `libsmbrr`, you can execute the runner script directly. This outputs colorful, detailed logs.

From the `build/tests/` directory:
```bash
./run_tests.sh
```

To test against FITS images instead of the default BMP images:
```bash
./run_tests.sh --fits
```

## Test Structure

### Mathematical & Algorithmic Tests (`test_*.c`)
These C binaries validate the core functionality of the library. They load a reference image (`wiz-ha-x.bmp` or `ngc7380.fit`), perform mathematical operations (convolution, clipping, structure finding), and verify the output metrics exactly match hardcoded mathematically correct assertions.

- **`test_atrous.c`**: Validates multi-scale A'trous wavelet convolution logic and K-Sigma statistics.
- **`test_structures.c`**: Validates contiguous pixel structure detection at specific scales.
- **`test_objects.c`**: Validates vertical connection of structures into overarching physical bounding objects.
- **`test_reconstruct.c`**: Validates the end-to-end reconstruction process and output image generation.

### Performance Benchmark (`test_performance.c`)
Measures the execution time of multi-threaded and SIMD-accelerated paths. Ensures that standard processing throughput does not regress over time.

### Package Validation (`test_packages.sh`)
This script simulates an end-user installation experience using the freshly built `.deb`, `.rpm`, and Python `.whl` files.

1. **Linux Packages (*.deb, *.rpm)**: Extracts the compiled artifacts, establishes local header configurations, points `LD_LIBRARY_PATH` to the extracted paths, and independently dynamically links the demo C examples using `gcc` to verify no symbols are missing.
2. **Python Weights (*.whl)**: Synthesizes a fresh Python virtual environment (`venv`), strictly installs via pip directly from the local Python wheel, and validates ctypes bindings by asserting `import sombrero` works correctly.

## Adding Tests

To add a new mathematical test:
1. Create a `test_[module].c` file mirroring existing binaries.
2. Register the executable in `tests/CMakeLists.txt`.
3. Add it to the test runner via the `add_test` loop within CMake and visually in `run_tests.sh`.
4. Ensure the output strictly conforms to the expected API assertions.
