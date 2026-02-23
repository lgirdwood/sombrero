import cv2
import ctypes
import os
import sys
import numpy as np

# Define constants
SMBRR_DATA_2D_FLOAT = 3
SMBRR_SOURCE_UINT8 = 0
SMBRR_WAVELET_MASK_LINEAR = 0
SMBRR_CLIP_VGENTLE = 0

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

# int smbrr_reconstruct(struct smbrr *O, enum smbrr_wavelet_mask mask, float threshold, int scales, enum smbrr_clip sigma_clip);
smbrr.smbrr_reconstruct.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_float, ctypes.c_int, ctypes.c_int]
smbrr.smbrr_reconstruct.restype = ctypes.c_int

smbrr.smbrr_normalise.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float]
smbrr.smbrr_normalise.restype = None

smbrr.smbrr_get_data.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p]
smbrr.smbrr_get_data.restype = ctypes.c_int

def main():
    # Configuration
    scales = 8
    threshold = 1.0e-4
    sigma_clip = SMBRR_CLIP_VGENTLE

    # Open the camera
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open camera.")
        sys.exit(1)

    print("Starting USB camera image reconstruction...")
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

        if out_buf is None or out_buf.shape != (height, width):
            out_buf = np.zeros((height, width), dtype=np.uint8)

        # Get C pointer to image data
        data_ptr = gray.ctypes.data_as(ctypes.c_void_p)

        # Create smbrr image context
        image = smbrr.smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, SMBRR_SOURCE_UINT8, data_ptr)
        if not image:
            print("Error: Failed to create smbrr context")
            continue

        # Process the frame using reconstruct
        res = smbrr.smbrr_reconstruct(image, SMBRR_WAVELET_MASK_LINEAR, ctypes.c_float(threshold), scales, sigma_clip)
        if res < 0:
            print(f"Error: Reconstruct failed with code {res}")
            smbrr.smbrr_free(image)
            continue
            
        # Normalize the internal float reconstructed image to 0-255 before extracting uint8 
        smbrr.smbrr_normalise(image, ctypes.c_float(0.0), ctypes.c_float(255.0))

        # Extract the processed pixel data
        out_buf_ptr = out_buf.ctypes.data_as(ctypes.c_void_p)
        smbrr.smbrr_get_data(image, SMBRR_SOURCE_UINT8, ctypes.byref(out_buf_ptr))

        # Display the result side-by-side with original for comparison
        # out_buf holds the reconstructed grayscale image
        
        # Convert output back to BGR so we can hstack it cleanly with the original
        out_bgr = cv2.cvtColor(out_buf, cv2.COLOR_GRAY2BGR)
        
        # Draw labels
        cv2.putText(frame, "Original", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.putText(out_bgr, "Reconstructed", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        combined = np.hstack((frame, out_bgr))
        cv2.imshow("Sombrero Image Reconstruction", combined)

        # Free memory
        smbrr.smbrr_free(image)

        # Check for quit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Cleanup
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
