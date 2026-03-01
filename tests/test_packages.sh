#!/bin/bash

# test_packages.sh
# Tests package generation and performs local installation checks.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
BUILD_ROOT="@CMAKE_BINARY_DIR@"
SRC_ROOT="@CMAKE_SOURCE_DIR@"

# Local test directories
TEST_DIR="$SCRIPT_DIR/pkg_test"
DEB_DIR="$TEST_DIR/deb"
RPM_DIR="$TEST_DIR/rpm"
VENV_DIR="$TEST_DIR/venv"

echo "============================================================"
echo " Starting Package Build and Installation Tests"
echo "============================================================"

# Clean up previous runs
rm -rf "$TEST_DIR"
mkdir -p "$DEB_DIR" "$RPM_DIR" "$VENV_DIR"

echo "[1/4] Building packages via CMake..."
cd "$BUILD_ROOT"
cmake --build . --target package > "$TEST_DIR/pkg_build.log" 2>&1 || { echo "Failed to build DEB/RPM packages"; cat "$TEST_DIR/pkg_build.log"; exit 1; }
cmake --build . --target python-package > "$TEST_DIR/py_build.log" 2>&1 || { echo "Failed to build Python wheels"; cat "$TEST_DIR/py_build.log"; exit 1; }

echo "[2/4] Testing DEB Package..."
DEB_FILE=$(ls "$BUILD_ROOT"/*.deb 2>/dev/null | head -n 1)
if [ -z "$DEB_FILE" ]; then
    echo "Error: No .deb file found in $BUILD_ROOT!"
    exit 1
fi

echo "  Extracting $DEB_FILE"
dpkg-deb -x "$DEB_FILE" "$DEB_DIR"
if [ ! -f "$DEB_DIR/usr/lib/libsombrero.so" ] || [ ! -f "$DEB_DIR/usr/include/sombrero/sombrero.h" ]; then
    echo "Error: Extracted DEB missing required library files."
    ls -lR "$DEB_DIR"
    exit 1
fi

echo "  Testing compilation against DEB extracted files..."
cd "$DEB_DIR"
cp "$SRC_ROOT/examples/atrous.c" .
cp "$SRC_ROOT/examples/bmp.c" .
cp "$SRC_ROOT/examples/fits.c" .
cp "$SRC_ROOT/examples/bmp.h" .
cp "$SRC_ROOT/examples/fits.h" .
cp "$SRC_ROOT/include/local.h" .

gcc -I./usr/include/sombrero -I"$BUILD_ROOT" -L./usr/lib atrous.c bmp.c fits.c -o test_app_deb -lsombrero -lm -lcfitsio
LD_LIBRARY_PATH=./usr/lib ./test_app_deb -h > /dev/null
echo "  DEB compilation and execution OK."

echo "[3/4] Testing RPM Package..."
if ! command -v rpm2cpio &> /dev/null; then
    echo "  Skipping RPM test: rpm2cpio not found."
else
    RPM_FILE=$(ls "$BUILD_ROOT"/*.rpm 2>/dev/null | head -n 1)
    if [ -z "$RPM_FILE" ]; then
        echo "  Skipping RPM test: No .rpm file found in $BUILD_ROOT!"
    else
        echo "  Extracting $RPM_FILE"
        cd "$RPM_DIR"
        rpm2cpio "$RPM_FILE" | cpio -idmv > /dev/null 2>&1
        if [ ! -f "$RPM_DIR/usr/lib/libsombrero.so" ] || [ ! -f "$RPM_DIR/usr/include/sombrero/sombrero.h" ]; then
            echo "Error: Extracted RPM missing required library files."
            ls -lR "$RPM_DIR"
            exit 1
        fi

        echo "  Testing compilation against RPM extracted files..."
        cp "$SRC_ROOT/examples/atrous.c" .
        cp "$SRC_ROOT/examples/bmp.c" .
        cp "$SRC_ROOT/examples/fits.c" .
        cp "$SRC_ROOT/examples/bmp.h" .
        cp "$SRC_ROOT/examples/fits.h" .
        cp "$SRC_ROOT/include/local.h" .

        gcc -I./usr/include/sombrero -I"$BUILD_ROOT" -L./usr/lib atrous.c bmp.c fits.c -o test_app_rpm -lsombrero -lm -lcfitsio
        LD_LIBRARY_PATH=./usr/lib ./test_app_rpm -h > /dev/null
        echo "  RPM compilation and execution OK."
    fi
fi

echo "[4/4] Testing Python Package..."
WHL_FILE=$(ls "$BUILD_ROOT/python_dist"/*.whl 2>/dev/null | head -n 1)
if [ -z "$WHL_FILE" ]; then
    echo "Error: No .whl file found in $BUILD_ROOT/python_dist!"
    exit 1
fi

echo "  Creating venv and installing $WHL_FILE"
python3 -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"
pip install "$WHL_FILE" > "$TEST_DIR/pip_install.log" 2>&1 || { echo "Pip install failed"; cat "$TEST_DIR/pip_install.log"; exit 1; }

echo "  Testing python import..."
LD_LIBRARY_PATH="$DEB_DIR/usr/lib" python3 -c "import sombrero" || { echo "Failed to import sombrero in python"; exit 1; }
deactivate
echo "  Python wheel installation and execution OK."

echo "============================================================"
echo " All package tests PASSED successfully!"
echo "============================================================"

# Cleanup
rm -rf "$TEST_DIR"
exit 0
