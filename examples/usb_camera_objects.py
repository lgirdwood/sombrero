#!/usr/bin/env python3
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  Copyright (C) 2026 Liam Girdwood

import cv2
import ctypes
import os
import sys

# Define constants
SMBRR_DATA_2D_FLOAT = 3
SMBRR_SOURCE_UINT8 = 0
SMBRR_CONV_ATROUS = 0
SMBRR_WAVELET_MASK_LINEAR = 0
SMBRR_OBJECT_POINT = 0
SMBRR_OBJECT_EXTENDED = 1

class SmbrrCoord(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_uint),
        ("y", ctypes.c_uint)
    ]

class SmbrrObject(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint),
        ("type", ctypes.c_int),
        ("pos", SmbrrCoord),
        ("minXy", SmbrrCoord),
        ("minxY", SmbrrCoord),
        ("maxXy", SmbrrCoord),
        ("maxxY", SmbrrCoord),
        ("pa", ctypes.c_float),
        ("object_adu", ctypes.c_float),
        ("object_radius", ctypes.c_float),
        ("object_area", ctypes.c_uint),
        ("snr", ctypes.c_float),
        ("error", ctypes.c_float),
        ("background_area", ctypes.c_uint),
        ("background_adu", ctypes.c_float),
        ("max_adu", ctypes.c_float),
        ("mean_adu", ctypes.c_float),
        ("sigma_adu", ctypes.c_float),
        ("mag_delta", ctypes.c_float),
        ("scale", ctypes.c_uint)
    ]

# Load libsombrero
lib_path = os.path.join(os.path.dirname(__file__), "..", "build", "src", "libsombrero.so")
if not os.path.exists(lib_path):
    print(f"Error: Could not find libsombrero.so at {lib_path}")
    print("Please build the library first.")
    sys.exit(1)

smbrr = ctypes.CDLL(lib_path)

# Set up ctypes signatures
smbrr.smbrr_new.argtypes = [ctypes.c_int, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_int, ctypes.c_void_p]
smbrr.smbrr_new.restype = ctypes.c_void_p

smbrr.smbrr_free.argtypes = [ctypes.c_void_p]
smbrr.smbrr_free.restype = None

smbrr.smbrr_wavelet_new.argtypes = [ctypes.c_void_p, ctypes.c_uint]
smbrr.smbrr_wavelet_new.restype = ctypes.c_void_p

smbrr.smbrr_wavelet_free.argtypes = [ctypes.c_void_p]
smbrr.smbrr_wavelet_free.restype = None

smbrr.smbrr_wavelet_convolution.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
smbrr.smbrr_wavelet_convolution.restype = ctypes.c_int

smbrr.smbrr_wavelet_ksigma_clip.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_float]
smbrr.smbrr_wavelet_ksigma_clip.restype = ctypes.c_int

smbrr.smbrr_wavelet_structure_find.argtypes = [ctypes.c_void_p, ctypes.c_uint]
smbrr.smbrr_wavelet_structure_find.restype = ctypes.c_int

smbrr.smbrr_wavelet_structure_connect.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint]
smbrr.smbrr_wavelet_structure_connect.restype = ctypes.c_int

smbrr.smbrr_wavelet_object_get.argtypes = [ctypes.c_void_p, ctypes.c_uint]
smbrr.smbrr_wavelet_object_get.restype = ctypes.POINTER(SmbrrObject)

def main():
    # Configuration
    scales = 5
    k_sigma = 1
    sigma_delta = 0.001

    # Open the camera
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open camera.")
        sys.exit(1)

    print("Starting USB camera object detection...")
    print("Press 'q' to quit.")

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Could not read frame.")
            break

        # Convert to grayscale for libsombrero
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        height, width = gray.shape
        stride = gray.strides[0]

        # Get C pointer to image data
        data_ptr = gray.ctypes.data_as(ctypes.c_void_p)

        # Create smbrr image context
        image = smbrr.smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, SMBRR_SOURCE_UINT8, data_ptr)
        if not image:
            print("Error: Failed to create smbrr context")
            continue

        # Create wavelet context
        w = smbrr.smbrr_wavelet_new(image, scales)
        if not w:
            print("Error: Failed to create wavelet context")
            smbrr.smbrr_free(image)
            continue

        # Process the frame
        res = smbrr.smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS, SMBRR_WAVELET_MASK_LINEAR)
        if res < 0:
            print(f"Error: Convolution failed with code {res}")
            smbrr.smbrr_wavelet_free(w)
            smbrr.smbrr_free(image)
            continue

        smbrr.smbrr_wavelet_ksigma_clip(w, k_sigma, ctypes.c_float(sigma_delta))

        # Find structures at each scale
        for i in range(scales - 1):
            smbrr.smbrr_wavelet_structure_find(w, i)

        # Connect structures into objects
        num_objects = smbrr.smbrr_wavelet_structure_connect(w, 0, scales - 2)

        # Draw objects on the original RGB frame
        for i in range(num_objects):
            obj_ptr = smbrr.smbrr_wavelet_object_get(w, i)
            if not obj_ptr:
                continue
            
            obj = obj_ptr.contents
            
            # Draw circle at center
            center = (int(obj.pos.x), int(obj.pos.y))
            radius = int(max(obj.object_radius, 2))
            
            # Select color based on scale
            colors = [
                (255, 0, 0),   # B: Scale 0
                (0, 255, 0),   # G: Scale 1
                (0, 0, 255),   # R: Scale 2
                (255, 255, 0), # C: Scale 3
                (255, 0, 255), # M: Scale 4
                (0, 255, 255)  # Y: Scale 5
            ]
            color = colors[obj.scale % len(colors)]
            
            cv2.circle(frame, center, radius, color, 2)
            cv2.putText(frame, f"ID:{obj.id} S:{obj.scale}", (center[0] + radius + 2, center[1]), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1)

        # Display the frame
        cv2.putText(frame, f"Objects: {num_objects}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.imshow("Sombrero Object Detection", frame)

        # Free memory
        smbrr.smbrr_wavelet_free(w)
        smbrr.smbrr_free(image)

        # Check for quit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Cleanup
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
