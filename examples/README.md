# Libsmbrr Examples

This directory contains standalone examples that demonstrate how to use `libsmbrr` (Sombrero) for wavelet processing, noise reduction, and object detection. These examples serve both as tutorials and functional command-line tools for processing real data.

The examples natively support reading **BMP** images (greyscale) and **FITS** (Flexible Image Transport System) images.

## C Examples

These examples are built automatically when configuring CMake.

### 1. `smbrr-atrous`
Performs a basic A'trous wavelet decomposition of an image into 9 independent spatial frequency scales and saves each scale as a separate image.

**Usage:**
```bash
./smbrr-atrous [-k clip strength] [-s sigma delta] [-A gain strength] [-S scales] -i infile.bmp -o outfile_prefix
```
* `-k`: K-Sigma clip strength. Default 1. Values 0 .. 5 (gentle -> strong)
* `-A`: Gain strength. Default 0. Values 0 .. 4 (low .. high freq)
* `-S`: Number of scales to process. Default and max 9.

### 2. `smbrr-structures`
Extracts and isolates contiguous significant structures distinct from the background on each wavelet scale.

**Usage:**
```bash
./smbrr-structures [-g gain] [-b bias] [-r readout] [-a] [-k clip strength] [-s sigma delta] [-A gain strength] [-S scales] -i infile.bmp -o outfile_prefix
```
* `-a`: Enable Anscombe transform to stabilize Poisson noise using `-g` (gain), `-b` (bias), and `-r` (readout noise).

### 3. `smbrr-objects`
Performs full multi-scale object detection by connecting 2D structures across vertical wavelet layers. It extracts discrete mathematical metadata about objects (like stars or galaxies) and saves cropped image versions of the highest-magnitude detected objects.

**Usage:**
```bash
./smbrr-objects [-g gain] [-b bias] [-r readout] [-a] [-k clip strength] [-s sigma delta] [-A gain strength] [-S scales] -t -i infile.bmp -o outfile_prefix
```
* `-t`: Print execution timing for each internal mathematical step.

### 4. `smbrr-reconstruct`
Demonstrates full automated image noise reduction. It performs wavelet decomposition, K-Sigma clipping to drop noisy components, and recombines only the pure "significant" bands back into a single clean pristine image.

**Usage:**
```bash
./smbrr-reconstruct -i infile.bmp -o outfile.bmp
```

### 5. `smbrr-reconstruct-1d`
An example of processing 1-dimensional signals (like audio) rather than 2D pixel grids.

**Usage:**
```bash
./smbrr-reconstruct-1d file.wav
```

## Python Examples

Python scripts utilizing the `sombrero` ctypes bindings to demonstrate interactive video processing capabilities.

### `usb_camera_reconstruct.py`
Captures live frames from an external USB camera via OpenCV, processes the frames using the `smbrr_reconstruct` pipeline to remove noise in real-time, and renders a side-by-side comparison window.

**Requirements:** `python3 -m pip install opencv-python numpy sombrero`
**Usage:** `python3 usb_camera_reconstruct.py`

### `usb_camera_sigall.py`
Captures USB camera frames, extracts all structure significance layers in real-time, and visualizes the aggregated components.

### `usb_camera_objects.py`
Live object tracking and detection via the USB camera frame feed.

---

**Note**: For FITS files, the `.fit` or `.fits` extension will be parsed and loaded appropriately. If processing BMPs, ensure they are greyscale images formatted with 8-bit depth.
