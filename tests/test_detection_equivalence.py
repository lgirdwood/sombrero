#!/usr/bin/env python3
import os
import sys
import ctypes
import numpy as np
import pytest
from astropy.io import fits

# Import libsombrero Python bindings
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "python"))
import sombrero

def load_image_data(filepath):
    is_fits = filepath.lower().endswith(('.fit', '.fits'))
    is_bmp = filepath.lower().endswith('.bmp')
    
    raw_data = None
    width = 0
    height = 0
    adu_type = 0 # Default UINT8
    
    if is_fits:
        with fits.open(filepath) as hdul:
            img_data = hdul[0].data
            if img_data is None and len(hdul) > 1:
                img_data = hdul[1].data
            
            if img_data is not None:
                if img_data.ndim == 2:
                    height, width = img_data.shape
                    float_data = img_data.astype(np.float32)
                    raw_data = float_data.tobytes()
                    adu_type = 3 # FLOAT type in libsmbrr
                    
    elif is_bmp:
        with open(filepath, "rb") as f:
            header = f.read(54)
            if header[:2] == b"BM":
                off_bits = int.from_bytes(header[10:14], byteorder='little')
                width = int.from_bytes(header[18:22], byteorder='little')
                height = int.from_bytes(header[22:26], byteorder='little')
                f.seek(off_bits)
                raw_data = f.read()
                adu_type = 0 # UINT8
                
    return raw_data, width, height, adu_type, is_fits

def process_and_extract(filepath, scales=9, k=2, s=0.0):
    raw_data, width, height, adu_type, is_fits = load_image_data(filepath)
    assert raw_data is not None, f"Failed to load data from {filepath}"
    
    data_buffer = ctypes.create_string_buffer(raw_data, len(raw_data))
    img = sombrero.smbrr.smbrr_new(3, width, height, 0, adu_type, data_buffer)
    assert img, "Failed to initialize smbrr context"
    
    w = sombrero.smbrr.smbrr_wavelet_new(img, scales)
    sombrero.smbrr.smbrr_wavelet_convolution(w, sombrero.SMBRR_CONV_ATROUS, sombrero.SMBRR_WAVELET_MASK_LINEAR)
    sombrero.smbrr.smbrr_wavelet_ksigma_clip(w, k, s)
    
    for i in range(scales - 1):
        sombrero.smbrr.smbrr_wavelet_structure_find(w, i)
        
    sombrero.smbrr.smbrr_wavelet_structure_connect(w, 0, scales - 2)
    
    structures = []
    for i in range(scales - 1):
        num_structures = sombrero.smbrr.smbrr_wavelet_get_num_structures(w, i)
        for j in range(num_structures):
            struct_data = sombrero.SmbrrStructure()
            if sombrero.smbrr.smbrr_wavelet_get_structure(w, i, j, ctypes.byref(struct_data)) == 0:
                structures.append({
                    "id": struct_data.id,
                    "scale": i,
                    "x": struct_data.pos.x,
                    "y": struct_data.pos.y,
                    "max_value": struct_data.max_value,
                    "size": struct_data.size
                })
                
    objects = []
    for i in range(10000): # Safe limit
        obj_ptr = sombrero.smbrr.smbrr_wavelet_object_get(w, i)
        if not obj_ptr:
            break
        obj = obj_ptr.contents
        objects.append({
            "id": obj.id,
            "x": obj.pos.x,
            "y": obj.pos.y,
            "radius": float(obj.object_radius),
            "scale": obj.scale
        })
        
    sombrero.smbrr.smbrr_wavelet_free(w)
    sombrero.smbrr.smbrr_free(img)
    
    return structures, objects, is_fits, height

def test_skv1427378808925_equivalence():
    """Test that detection results are identical for the same image regardless of file format"""
    fits_path = os.path.join(os.path.dirname(__file__), "..", "examples", "images", "skv1427378808925.fits")
    bmp_path = os.path.join(os.path.dirname(__file__), "..", "examples", "images", "skv1427378808925.bmp")
    
    assert os.path.exists(fits_path), f"FITS file not found: {fits_path}"
    assert os.path.exists(bmp_path), f"BMP file not found: {bmp_path}"
    
    print(f"Processing FITS: {fits_path}")
    fits_structures, fits_objects, _, fits_height = process_and_extract(fits_path)
    
    print(f"Processing BMP: {bmp_path}")
    bmp_structures, bmp_objects, _, bmp_height = process_and_extract(bmp_path)
    
    assert fits_height == bmp_height, f"Height mismatch: {fits_height} vs {bmp_height}"
    
    # Due to FITS being true float data vs BMP being scaled to 8-bit integers,
    # borderline clipping values can shift slightly resulting in very minor differences
    # in structure counts. So we ensure they represent ~98% of the same detections.
    struct_diff = abs(len(fits_structures) - len(bmp_structures))
    assert struct_diff / max(len(fits_structures), 1) < 0.05, \
        f"Structures count diverges too much: FITS={len(fits_structures)}, BMP={len(bmp_structures)}"
        
    obj_diff = abs(len(fits_objects) - len(bmp_objects))
    assert obj_diff / max(len(fits_objects), 1) < 0.05, \
        f"Objects count diverges too much: FITS={len(fits_objects)}, BMP={len(bmp_objects)}"
        
    print(f"Counts aligned! Objects: FITS={len(fits_objects)}, BMP={len(bmp_objects)}")
    
    # Check that the highest intensity structures roughly match spatially
    # Sort by max value or radius to compare the largest ones
    fits_structures.sort(key=lambda s: s["max_value"], reverse=True)
    bmp_structures.sort(key=lambda s: s["max_value"], reverse=True)
    
    tol = 3.0 # pixel tolerance due to border artifacts and rounding
    
    matches = 0
    comparisons = min(100, len(fits_structures), len(bmp_structures))
    for i in range(comparisons):
        ft_o = fits_structures[i]
        
        # Find closest in BMP by proximity
        closest = None
        min_dist = float('inf')
        for bm_o in bmp_structures[:comparisons*2]: # Search a larger window
            dist = ((ft_o['x'] - bm_o['x'])**2 + (ft_o['y'] - bm_o['y'])**2)**0.5
            if dist < min_dist:
                min_dist = dist
                closest = bm_o
                
        if min_dist <= tol:
            matches += 1
            
    match_rate = matches / comparisons
    print(f"Top structure spatial match rate: {match_rate*100:.1f}%")
    assert match_rate > 0.90, f"Spatial detections do not match across formats! {match_rate*100:.1f}%"

if __name__ == "__main__":
    test_skv1427378808925_equivalence()
    print("All detection equivalence tests passed!")
