import ctypes
import os
import sys
from ctypes import POINTER, c_int, c_uint, c_float, c_void_p, Structure, byref

# Helper to find and load the library
def _load_libsombrero():
    # Try looking in the build directory relative to this script first (for development)
    local_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", "src", "libsombrero.so")
    if os.path.exists(local_path):
        return ctypes.CDLL(local_path)
    
    # Otherwise try system paths
    try:
        return ctypes.CDLL("libsombrero.so")
    except OSError:
        print(f"Error: Could not find libsombrero.so. Make sure it's installed or in {local_path}.")
        sys.exit(1)

smbrr = _load_libsombrero()

# --- Constants ---
SMBRR_MAX_SCALES = 12

# --- Enumerations ---
# enum smbrr_source_type
SMBRR_SOURCE_UINT8 = 0
SMBRR_SOURCE_UINT16 = 1
SMBRR_SOURCE_UINT32 = 2
SMBRR_SOURCE_FLOAT = 3

# enum smbrr_data_type
SMBRR_DATA_1D_UINT32 = 0
SMBRR_DATA_1D_FLOAT = 1
SMBRR_DATA_2D_UINT32 = 2
SMBRR_DATA_2D_FLOAT = 3

# enum smbrr_conv
SMBRR_CONV_ATROUS = 0
SMBRR_CONV_PSF = 1

# enum smbrr_wavelet_mask
SMBRR_WAVELET_MASK_LINEAR = 0
SMBRR_WAVELET_MASK_BICUBIC = 1

# enum smbrr_clip
SMBRR_CLIP_VGENTLE = 0
SMBRR_CLIP_GENTLE = 1
SMBRR_CLIP_NORMAL = 2
SMBRR_CLIP_STRONG = 3
SMBRR_CLIP_VSTRONG = 4
SMBRR_CLIP_VVSTRONG = 5

# enum smbrr_gain
SMBRR_GAIN_NONE = 0
SMBRR_GAIN_LOW = 1
SMBRR_GAIN_MID = 2
SMBRR_GAIN_HIGH = 3
SMBRR_GAIN_LOWMID = 4

# enum smbrr_object_type
SMBRR_OBJECT_POINT = 0
SMBRR_OBJECT_EXTENDED = 1

# --- Structures ---
class SmbrrCoord(Structure):
    _fields_ = [
        ("x", c_uint),
        ("y", c_uint)
    ]

class SmbrrObject(Structure):
    _fields_ = [
        ("id", c_uint),
        ("type", c_int), # enum smbrr_object_type
        ("pos", SmbrrCoord),
        ("minXy", SmbrrCoord),
        ("minxY", SmbrrCoord),
        ("maxXy", SmbrrCoord),
        ("maxxY", SmbrrCoord),
        ("pa", c_float),
        ("object_adu", c_float),
        ("object_radius", c_float),
        ("object_area", c_uint),
        ("snr", c_float),
        ("error", c_float),
        ("background_area", c_uint),
        ("background_adu", c_float),
        ("max_adu", c_float),
        ("mean_adu", c_float),
        ("sigma_adu", c_float),
        ("mag_delta", c_float),
        ("scale", c_uint)
    ]

class SmbrrClipCoeff(Structure):
    _fields_ = [
        ("coeff", c_float * (SMBRR_MAX_SCALES - 1))
    ]

# --- Functions ---

# Struct pointers (opaque in Python since they are not fully defined in public header)
smbrr_p = c_void_p
smbrr_wavelet_p = c_void_p

# Data Context
smbrr.smbrr_new.argtypes = [c_int, c_uint, c_uint, c_uint, c_int, c_void_p]
smbrr.smbrr_new.restype = smbrr_p

smbrr.smbrr_new_from_area.argtypes = [smbrr_p, c_uint, c_uint, c_uint, c_uint]
smbrr.smbrr_new_from_area.restype = smbrr_p

smbrr.smbrr_new_from_section.argtypes = [smbrr_p, c_uint, c_uint]
smbrr.smbrr_new_from_section.restype = smbrr_p

smbrr.smbrr_new_copy.argtypes = [smbrr_p]
smbrr.smbrr_new_copy.restype = smbrr_p

smbrr.smbrr_free.argtypes = [smbrr_p]
smbrr.smbrr_free.restype = None

smbrr.smbrr_init_opencl.argtypes = [c_int]
smbrr.smbrr_init_opencl.restype = c_int

smbrr.smbrr_free_opencl.argtypes = []
smbrr.smbrr_free_opencl.restype = None

# Element Information
smbrr.smbrr_get_data.argtypes = [smbrr_p, c_int, POINTER(c_void_p)]
smbrr.smbrr_get_data.restype = c_int

smbrr.smbrr_get_size.argtypes = [smbrr_p]
smbrr.smbrr_get_size.restype = c_int

smbrr.smbrr_get_bytes.argtypes = [smbrr_p]
smbrr.smbrr_get_bytes.restype = c_int

smbrr.smbrr_get_stride.argtypes = [smbrr_p]
smbrr.smbrr_get_stride.restype = c_int

smbrr.smbrr_get_width.argtypes = [smbrr_p]
smbrr.smbrr_get_width.restype = c_int

smbrr.smbrr_get_height.argtypes = [smbrr_p]
smbrr.smbrr_get_height.restype = c_int

smbrr.smbrr_find_limits.argtypes = [smbrr_p, POINTER(c_float), POINTER(c_float)]
smbrr.smbrr_find_limits.restype = None

smbrr.smbrr_get_mean.argtypes = [smbrr_p]
smbrr.smbrr_get_mean.restype = c_float

smbrr.smbrr_get_sigma.argtypes = [smbrr_p, c_float]
smbrr.smbrr_get_sigma.restype = c_float

smbrr.smbrr_significant_get_mean.argtypes = [smbrr_p, smbrr_p]
smbrr.smbrr_significant_get_mean.restype = c_float

smbrr.smbrr_significant_get_sigma.argtypes = [smbrr_p, smbrr_p, c_float]
smbrr.smbrr_significant_get_sigma.restype = c_float

smbrr.smbrr_get_norm.argtypes = [smbrr_p]
smbrr.smbrr_get_norm.restype = c_float

# Data Transformations
smbrr.smbrr_normalise.argtypes = [smbrr_p, c_float, c_float]
smbrr.smbrr_normalise.restype = None

smbrr.smbrr_add.argtypes = [smbrr_p, smbrr_p, smbrr_p]
smbrr.smbrr_add.restype = None

smbrr.smbrr_significant_add_value.argtypes = [smbrr_p, smbrr_p, c_float]
smbrr.smbrr_significant_add_value.restype = None

smbrr.smbrr_significant_add.argtypes = [smbrr_p, smbrr_p, smbrr_p, smbrr_p]
smbrr.smbrr_significant_add.restype = None

smbrr.smbrr_subtract.argtypes = [smbrr_p, smbrr_p, smbrr_p]
smbrr.smbrr_subtract.restype = None

smbrr.smbrr_significant_subtract.argtypes = [smbrr_p, smbrr_p, smbrr_p, smbrr_p]
smbrr.smbrr_significant_subtract.restype = None

smbrr.smbrr_add_value.argtypes = [smbrr_p, c_float]
smbrr.smbrr_add_value.restype = None

smbrr.smbrr_subtract_value.argtypes = [smbrr_p, c_float]
smbrr.smbrr_subtract_value.restype = None

smbrr.smbrr_mult_value.argtypes = [smbrr_p, c_float]
smbrr.smbrr_mult_value.restype = None

smbrr.smbrr_set_value.argtypes = [smbrr_p, c_float]
smbrr.smbrr_set_value.restype = None

smbrr.smbrr_significant_set_value.argtypes = [smbrr_p, smbrr_p, c_float]
smbrr.smbrr_significant_set_value.restype = None

smbrr.smbrr_significant_set_svalue.argtypes = [smbrr_p, c_uint]
smbrr.smbrr_significant_set_svalue.restype = None

smbrr.smbrr_convert.argtypes = [smbrr_p, c_int]
smbrr.smbrr_convert.restype = c_int

smbrr.smbrr_zero_negative.argtypes = [smbrr_p]
smbrr.smbrr_zero_negative.restype = None

smbrr.smbrr_abs.argtypes = [smbrr_p]
smbrr.smbrr_abs.restype = None

smbrr.smbrr_signed.argtypes = [smbrr_p, smbrr_p]
smbrr.smbrr_signed.restype = c_int

smbrr.smbrr_copy.argtypes = [smbrr_p, smbrr_p]
smbrr.smbrr_copy.restype = c_int

smbrr.smbrr_significant_copy.argtypes = [smbrr_p, smbrr_p, smbrr_p]
smbrr.smbrr_significant_copy.restype = c_int

smbrr.smbrr_mult_add.argtypes = [smbrr_p, smbrr_p, smbrr_p, c_float]
smbrr.smbrr_mult_add.restype = None

smbrr.smbrr_mult_subtract.argtypes = [smbrr_p, smbrr_p, smbrr_p, c_float]
smbrr.smbrr_mult_subtract.restype = None

smbrr.smbrr_anscombe.argtypes = [smbrr_p, c_float, c_float, c_float]
smbrr.smbrr_anscombe.restype = None

smbrr.smbrr_significant_new.argtypes = [smbrr_p, smbrr_p, c_float]
smbrr.smbrr_significant_new.restype = None

smbrr.smbrr_psf.argtypes = [smbrr_p, smbrr_p, c_int]
smbrr.smbrr_psf.restype = c_int

smbrr.smbrr_get_adu_at_posn.argtypes = [smbrr_p, c_int, c_int]
smbrr.smbrr_get_adu_at_posn.restype = c_float

smbrr.smbrr_get_adu_at_offset.argtypes = [smbrr_p, c_int]
smbrr.smbrr_get_adu_at_offset.restype = c_float

smbrr.smbrr_reconstruct.argtypes = [smbrr_p, c_int, c_float, c_int, c_int]
smbrr.smbrr_reconstruct.restype = c_int

# Wavelet Operations
smbrr.smbrr_wavelet_new.argtypes = [smbrr_p, c_uint]
smbrr.smbrr_wavelet_new.restype = smbrr_wavelet_p

smbrr.smbrr_wavelet_new_from_object.argtypes = [POINTER(SmbrrObject)]
smbrr.smbrr_wavelet_new_from_object.restype = smbrr_wavelet_p

smbrr.smbrr_wavelet_free.argtypes = [smbrr_wavelet_p]
smbrr.smbrr_wavelet_free.restype = None

smbrr.smbrr_wavelet_convolution.argtypes = [smbrr_wavelet_p, c_int, c_int]
smbrr.smbrr_wavelet_convolution.restype = c_int

smbrr.smbrr_wavelet_significant_convolution.argtypes = [smbrr_wavelet_p, c_int, c_int]
smbrr.smbrr_wavelet_significant_convolution.restype = c_int

smbrr.smbrr_wavelet_deconvolution.argtypes = [smbrr_wavelet_p, c_int, c_int]
smbrr.smbrr_wavelet_deconvolution.restype = c_int

smbrr.smbrr_wavelet_significant_deconvolution.argtypes = [smbrr_wavelet_p, c_int, c_int, c_int]
smbrr.smbrr_wavelet_significant_deconvolution.restype = c_int

smbrr.smbrr_wavelet_deconvolution_object.argtypes = [smbrr_wavelet_p, c_int, c_int, POINTER(SmbrrObject)]
smbrr.smbrr_wavelet_deconvolution_object.restype = c_int

smbrr.smbrr_wavelet_get_scale.argtypes = [smbrr_wavelet_p, c_uint]
smbrr.smbrr_wavelet_get_scale.restype = smbrr_p

smbrr.smbrr_wavelet_get_wavelet.argtypes = [smbrr_wavelet_p, c_uint]
smbrr.smbrr_wavelet_get_wavelet.restype = smbrr_p

smbrr.smbrr_wavelet_get_significant.argtypes = [smbrr_wavelet_p, c_uint]
smbrr.smbrr_wavelet_get_significant.restype = smbrr_p

smbrr.smbrr_wavelet_add.argtypes = [smbrr_wavelet_p, smbrr_wavelet_p, smbrr_wavelet_p]
smbrr.smbrr_wavelet_add.restype = None

smbrr.smbrr_wavelet_subtract.argtypes = [smbrr_wavelet_p, smbrr_wavelet_p, smbrr_wavelet_p]
smbrr.smbrr_wavelet_subtract.restype = None

smbrr.smbrr_wavelet_significant_subtract.argtypes = [smbrr_wavelet_p, smbrr_wavelet_p, smbrr_wavelet_p]
smbrr.smbrr_wavelet_significant_subtract.restype = None

smbrr.smbrr_wavelet_significant_add.argtypes = [smbrr_wavelet_p, smbrr_wavelet_p, smbrr_wavelet_p]
smbrr.smbrr_wavelet_significant_add.restype = None

smbrr.smbrr_wavelet_new_significant.argtypes = [smbrr_wavelet_p, c_int]
smbrr.smbrr_wavelet_new_significant.restype = c_int

smbrr.smbrr_wavelet_ksigma_clip.argtypes = [smbrr_wavelet_p, c_int, c_float]
smbrr.smbrr_wavelet_ksigma_clip.restype = c_int

smbrr.smbrr_wavelet_ksigma_clip_custom.argtypes = [smbrr_wavelet_p, POINTER(SmbrrClipCoeff), c_float]
smbrr.smbrr_wavelet_ksigma_clip_custom.restype = c_int

# Object Finding
smbrr.smbrr_wavelet_structure_find.argtypes = [smbrr_wavelet_p, c_uint]
smbrr.smbrr_wavelet_structure_find.restype = c_int

smbrr.smbrr_wavelet_structure_connect.argtypes = [smbrr_wavelet_p, c_uint, c_uint]
smbrr.smbrr_wavelet_structure_connect.restype = c_int

smbrr.smbrr_wavelet_object_get.argtypes = [smbrr_wavelet_p, c_uint]
smbrr.smbrr_wavelet_object_get.restype = POINTER(SmbrrObject)

smbrr.smbrr_wavelet_object_free_all.argtypes = [smbrr_wavelet_p]
smbrr.smbrr_wavelet_object_free_all.restype = None

smbrr.smbrr_wavelet_object_get_data.argtypes = [smbrr_wavelet_p, POINTER(SmbrrObject), POINTER(smbrr_p)]
smbrr.smbrr_wavelet_object_get_data.restype = c_int

smbrr.smbrr_wavelet_get_object_at_posn.argtypes = [smbrr_wavelet_p, c_int, c_int]
smbrr.smbrr_wavelet_get_object_at_posn.restype = POINTER(SmbrrObject)

smbrr.smbrr_wavelet_set_dark_mean.argtypes = [smbrr_wavelet_p, c_float]
smbrr.smbrr_wavelet_set_dark_mean.restype = c_int

smbrr.smbrr_wavelet_set_ccd.argtypes = [smbrr_wavelet_p, c_float, c_float, c_float]
smbrr.smbrr_wavelet_set_ccd.restype = None

smbrr.smbrr_wavelet_set_elems.argtypes = [smbrr_wavelet_p, smbrr_p]
smbrr.smbrr_wavelet_set_elems.restype = c_int
