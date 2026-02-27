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
import numpy as np

# Add python directory to path so we can import sombrero
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "python"))
import sombrero as smbrr


def main():
    # Configuration
    scales = 5
    k_sigma = 1
    sigma_delta = 0.001
    gain_strength = 0

    # Open the camera
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open camera.")
        sys.exit(1)

    print("Starting USB camera structure detection (sigall)...")
    print("Press 'q' to quit.")

    # Create a persistent numpy array for the output buffer
    out_buf = None

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Could not read frame.")
            break

        # Convert to grayscale for libsombrero
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        height, width = gray.shape
        stride = gray.strides[0]

        if out_buf is None:
            out_buf = np.zeros((height, width), dtype=np.uint8)
        elif out_buf.shape != (height, width):
            out_buf = np.zeros((height, width), dtype=np.uint8)

        # Get C pointer to image data
        data_ptr = gray.ctypes.data_as(ctypes.c_void_p)

        # Create smbrr image context
        image = smbrr.smbrr.smbrr_new(smbrr.SMBRR_DATA_2D_FLOAT, width, height, stride, smbrr.SMBRR_SOURCE_UINT8, data_ptr)
        if not image:
            print("Error: Failed to create smbrr context")
            continue
            
        # Create output image context
        oimage = smbrr.smbrr.smbrr_new(smbrr.SMBRR_DATA_2D_FLOAT, width, height, stride, smbrr.SMBRR_SOURCE_UINT8, None)
        if not oimage:
            print("Error: Failed to create output smbrr context")
            smbrr.smbrr.smbrr_free(image)
            continue

        # Create wavelet context
        w = smbrr.smbrr.smbrr_wavelet_new(image, scales)
        if not w:
            print("Error: Failed to create wavelet context")
            smbrr.smbrr.smbrr_free(oimage)
            smbrr.smbrr.smbrr_free(image)
            continue

        # Process the frame
        res = smbrr.smbrr.smbrr_wavelet_convolution(w, smbrr.SMBRR_CONV_ATROUS, smbrr.SMBRR_WAVELET_MASK_LINEAR)
        if res < 0:
            print(f"Error: Convolution failed with code {res}")
            smbrr.smbrr.smbrr_wavelet_free(w)
            smbrr.smbrr.smbrr_free(oimage)
            smbrr.smbrr.smbrr_free(image)
            continue

        smbrr.smbrr.smbrr_wavelet_ksigma_clip(w, k_sigma, ctypes.c_float(sigma_delta))

        res = smbrr.smbrr.smbrr_wavelet_significant_deconvolution(w, smbrr.SMBRR_CONV_ATROUS, smbrr.SMBRR_WAVELET_MASK_LINEAR, gain_strength)
        
        # Accumulate significant structures from all configured scales
        for i in range(scales - 1):
            simage = smbrr.smbrr.smbrr_wavelet_get_significant(w, ctypes.c_uint(i))
            if not simage:
                continue
            val = 16 + (1 << ((scales - 1) - i))
            smbrr.smbrr.smbrr_significant_add_value(oimage, simage, ctypes.c_float(val))

        smbrr.smbrr.smbrr_normalise(oimage, ctypes.c_float(0.0), ctypes.c_float(250.0))

        # Extract the processed pixel data
        
        # We need to pass the memory address of our numpy array directly to the C layer.
        # smbrr_get_data expects a void** that will be dereferenced by C to write to the memory.
        # Wait, the signature of smbrr_get_data in C is: int smbrr_get_data(struct smbrr *s, enum smbrr_source_type adu, void **data);
        # In bmp.c it's called with: smbrr_get_data(i, SMBRR_SOURCE_UINT8, &buf); where buf is a void* allocated by malloc.
        out_buf_ptr = out_buf.ctypes.data_as(ctypes.c_void_p)
        smbrr.smbrr.smbrr_get_data(oimage, smbrr.SMBRR_SOURCE_UINT8, ctypes.byref(out_buf_ptr))

        # Map to pseudo-colors for visualization since it's just intensity peaks now
        out_colored = cv2.applyColorMap(out_buf, cv2.COLORMAP_JET)

        # Display the frame
        cv2.imshow("Sombrero Structures (sigall)", out_colored)

        # Free memory
        smbrr.smbrr.smbrr_wavelet_free(w)
        smbrr.smbrr.smbrr_free(oimage)
        smbrr.smbrr.smbrr_free(image)

        # Check for quit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Cleanup
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
