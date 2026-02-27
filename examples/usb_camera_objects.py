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
import sys
import os

# Add python directory to path so we can import sombrero
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "python"))
import sombrero as smbrr


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
        image = smbrr.smbrr.smbrr_new(smbrr.SMBRR_DATA_2D_FLOAT, width, height, stride, smbrr.SMBRR_SOURCE_UINT8, data_ptr)
        if not image:
            print("Error: Failed to create smbrr context")
            continue

        # Create wavelet context
        w = smbrr.smbrr.smbrr_wavelet_new(image, scales)
        if not w:
            print("Error: Failed to create wavelet context")
            smbrr.smbrr.smbrr_free(image)
            continue

        # Process the frame
        res = smbrr.smbrr.smbrr_wavelet_convolution(w, smbrr.SMBRR_CONV_ATROUS, smbrr.SMBRR_WAVELET_MASK_LINEAR)
        if res < 0:
            print(f"Error: Convolution failed with code {res}")
            smbrr.smbrr.smbrr_wavelet_free(w)
            smbrr.smbrr.smbrr_free(image)
            continue

        smbrr.smbrr.smbrr_wavelet_ksigma_clip(w, k_sigma, ctypes.c_float(sigma_delta))

        # Find structures at each scale
        for i in range(scales - 1):
            smbrr.smbrr.smbrr_wavelet_structure_find(w, i)

        # Connect structures into objects
        num_objects = smbrr.smbrr.smbrr_wavelet_structure_connect(w, 0, scales - 2)

        # Draw objects on the original RGB frame
        for i in range(num_objects):
            obj_ptr = smbrr.smbrr.smbrr_wavelet_object_get(w, i)
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
        smbrr.smbrr.smbrr_wavelet_free(w)
        smbrr.smbrr.smbrr_free(image)

        # Check for quit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Cleanup
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
