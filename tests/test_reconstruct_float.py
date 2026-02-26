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
import numpy as np

smbrr = ctypes.CDLL('build/src/libsombrero.so')
smbrr.smbrr_new.argtypes = [ctypes.c_int, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_int, ctypes.c_void_p]
smbrr.smbrr_new.restype = ctypes.c_void_p
smbrr.smbrr_reconstruct.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_float, ctypes.c_int, ctypes.c_int]
smbrr.smbrr_get_data.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p]

width, height = 320, 240
gray = np.zeros((height, width), dtype=np.uint8)
for y in range(height):
    for x in range(width):
        gray[y, x] = (x + y) % 256

stride = gray.strides[0]

data_ptr = gray.ctypes.data_as(ctypes.c_void_p)
image = smbrr.smbrr_new(3, width, height, stride, 0, data_ptr)
smbrr.smbrr_reconstruct(image, 0, ctypes.c_float(1.0e-4), 8, 0)

# Try getting float data
out_float = np.zeros((height, width), dtype=np.float32)
out_float_ptr = out_float.ctypes.data_as(ctypes.c_void_p)
res = smbrr.smbrr_get_data(image, 6, ctypes.byref(out_float_ptr)) # 6 is FLOAT
if res != 0:
    print(f"Error: {res}")
else:
    print(out_float.min(), out_float.max())
    out_uint8 = np.clip(out_float, 0, 255).astype(np.uint8)
    print("Example uint8:", out_uint8[0, 0:5])
