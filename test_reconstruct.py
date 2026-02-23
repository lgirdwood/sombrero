import cv2
import ctypes
import numpy as np

smbrr = ctypes.CDLL('build/src/libsombrero.so')
smbrr.smbrr_new.argtypes = [ctypes.c_int, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_int, ctypes.c_void_p]
smbrr.smbrr_new.restype = ctypes.c_void_p
smbrr.smbrr_reconstruct.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_float, ctypes.c_int, ctypes.c_int]
smbrr.smbrr_get_data.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p]

cap = cv2.VideoCapture(0)
ret, frame = cap.read()
gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
print("Original min:", gray.min(), "max:", gray.max())

height, width = gray.shape
stride = gray.strides[0]
out_buf = np.zeros((height, width), dtype=np.uint8)

data_ptr = gray.ctypes.data_as(ctypes.c_void_p)
image = smbrr.smbrr_new(3, width, height, stride, 0, data_ptr)
smbrr.smbrr_reconstruct(image, 0, ctypes.c_float(1.0e-4), 8, 0)

out_buf_ptr = out_buf.ctypes.data_as(ctypes.c_void_p)
smbrr.smbrr_get_data(image, 0, ctypes.byref(out_buf_ptr))

print("Reconstructed min:", out_buf.min(), "max:", out_buf.max())

# Let's also extract floats to see the real values
out_float = np.zeros((height, width), dtype=np.float32)
out_float_ptr = out_float.ctypes.data_as(ctypes.c_void_p)
smbrr.smbrr_get_data(image, 1, ctypes.byref(out_float_ptr)) # wait, what is FLOAT adu?
# In sombrero.h: SMBRR_SOURCE_FLOAT is 3? Let's check.
