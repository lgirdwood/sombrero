# Sombrero

[![CI](https://github.com/lgirdwood/sombrero/actions/workflows/ci.yml/badge.svg)](https://github.com/lgirdwood/sombrero/actions/workflows/ci.yml)

Sombrero is a fast wavelet data processing and object detection C library for 1D and 2D data. 

Sombrero is named after the "Mexican Hat" shape of the wavelet masks used in data convolution. It is released under the GNU LGPL library. Initially developed for astronomical work, it can also be used with any other data that can benefit from wavelet processing. Some of the algorithms in this library are derived from "Astronomical Image and Data Analysis" by Jean-Luc Starck and Fionn Murtagh.

## Features

- Support for both 1D and 2D data element types.
- Significant element detection with k-sigma clipping to reduce background noise, resulting in better structure and object detection. Various K-sigma clipping levels are supported, including custom user-defined clipping coefficients.
- A'trous Wavelet convolution and deconvolution of data elements. The A'trous "with holes" data convolution is supported with either a linear or bicubic mask.
- Data transformations (e.g., add, subtract, multiply). General operators are supported and can be used for stacking, flats, dark frames, etc.
- Detection of structures and objects within data elements, alongside properties like max element values, brightness, average element values, position, and size. Simple object de-blending is also performed.
- SIMD optimizations for element and wavelet operations using SSE, AVX, AVX2, AVX-512, and FMA on x86 CPUs.
- OpenMP support for parallelism within wavelet operations.

## Building from Source

Sombrero uses CMake as its build system. To build the library and example applications, ensure you have CMake installed and a C compiler supporting C99. OpenMP and SIMD support (SSE4.2, AVX, AVX2, FMA) will be automatically detected and enabled if supported by your system and compiler.

```bash
# Clone the repository
git clone https://github.com/lgirdwood/sombrero.git
cd sombrero

# Create a build directory and configure with CMake
mkdir build && cd build
cmake ..

# Build using make
make -j$(nproc)
```

### Build Options

By default, CMake will try to enable SIMD optimizations and OpenMP if they are detected. You can explicitly turn them off during CMake configuration:

```bash
cmake .. -DENABLE_SSE42=OFF -DENABLE_AVX=OFF -DENABLE_AVX2=OFF -DENABLE_AVX512=OFF -DENABLE_FMA=OFF -DENABLE_OPENMP=OFF
```

## Running the Examples

A few example programs are provided in the `build/examples` directory to demonstrate data processing and object detection algorithms:

- `smbrr-atrous`
- `smbrr-structures`
- `smbrr-objects`
- `smbrr-reconstruct`
- `smbrr-reconstruct-1d`

The examples include simple Bitmap (.bmp) data support for working with image data.

Example usage for `smbrr-atrous`:
```bash
./examples/smbrr-atrous -i input.bmp -o output.bmp
```

You can view available options for each command using the tool directly:
```bash
./examples/smbrr-atrous -h
```

## Generating Documentation

If Doxygen is installed, you can generate the library API documentation:

```bash
cd build
make doc
```

The output documentation will be available in HTML format within the `build/html` directory.

## License

This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

Copyright (C) 2012, 2013 Liam Girdwood
