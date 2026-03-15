# Libsmbrr Python Examples

This directory contains example Python applications that utilize the `sombrero` `ctypes` wrapper to interface with the `libsmbrr` C library.

## Examples

### 1. `object_viewer.py`

A fully interactive GTK4 (+ Cairo) graphical application for inspecting and detecting structures and objects within FITS (`.fit`, `.fits`) and BMP image files.

**Features:**
- Loads and visualizes image files (FITS require the `astropy` Python package).
- Provides an interactive parameter dialog to configure the `libsmbrr` wavelet pipeline (scales, K-Sigma clipping threshold, Anscombe transform, etc.).
- Performs A'trous wavelet decomposition and object detection.
- Displays a hierarchical tree view (using modern GTK4 `ColumnView` and `TreeListModel`) grouping detected structures by their corresponding spatial scale, as well as the final connected physical objects.
- Renders the image natively using Cairo, overlaying visual markers for the detected objects and structures.
- Supports interactive clicking: clicking on the canvas highlights the nearest detected object and automatically selects it within the hierarchy tree.

**Usage:**
```bash
./object_viewer.py
```
*Note: Requires `PyGObject` (GTK4 bindings) and `pycairo`.*

### 2. `test_scroll.py`

A small, isolated GTK4 test script. This script tests the compatibility of the GTK4 `ColumnView.scroll_to` method across different versions of the GTK4 library (e.g., handling the signature change introduced in GTK 4.12 vs earlier GTK 4.0 bindings). It is primarily used as a development utility to ensure UI scrolling behavior works reliably across environments.

**Usage:**
```bash
python3 test_scroll.py
```
