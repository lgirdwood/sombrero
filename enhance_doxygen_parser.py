import textwrap

fun_desc = {
    "smbrr_new": "Create and allocate a new 1D or 2D data context, calculating stride boundaries and optionally initializing it with source data.",
    "smbrr_new_from_area": "Extract a rectangular sub-region from a 2D source context and allocate it into a new context, preserving the original data type.",
    "smbrr_new_from_section": "Extract a linear segment from a 1D source context and allocate it into a new context.",
    "smbrr_new_copy": "Perform a deep copy of an entire 1D or 2D data context into a newly allocated context of identical dimensions and type.",
    "smbrr_free": "Safely deallocate the internal ADU pixel buffers and the context structure itself.",
    "smbrr_get_data": "Retrieve a pointer to the raw internal pixel data buffer, optionally casting it to a specific ADU format.",
    "smbrr_get_size": "Retrieve the total number of initialized elements (width * height) in the data context.",
    "smbrr_get_bytes": "Calculate the total memory footprint in bytes required by the internal elements array.",
    "smbrr_get_stride": "Retrieve the memory alignment stride for the 2D data context to ensure proper row-by-row memory access.",
    "smbrr_get_width": "Retrieve the horizontal width (in pixels) of the 2D element matrix, or the length of a 1D sequence.",
    "smbrr_get_height": "Retrieve the vertical height (in pixels) of the 2D element matrix.",
    "smbrr_find_limits": "Iterate over all elements in the context to find the absolute minimum and maximum floating-point pixel values.",
    "smbrr_get_mean": "Compute the mathematical average (mean) across all pixels in the data context.",
    "smbrr_get_sigma": "Compute the standard deviation (sigma) representing the statistical dispersion of pixel values relative to the mean.",
    "smbrr_significant_get_mean": "Compute the average (mean) across pixels marked as significant (non-zero in the significance map).",
    "smbrr_significant_get_sigma": "Compute the standard deviation across pixels marked as significant (non-zero in the significance map).",
    "smbrr_get_norm": "Compute the Euclidean norm (square root of the sum of squared elements) of the data context.",
    "smbrr_normalise": "Scale and shift all pixel values linearly so that they fall within the specified min and max bounds.",
    "smbrr_add": "Perform element-wise matrix addition: A = B + C.",
    "smbrr_significant_add_value": "Add a constant scalar value exclusively to pixels that are marked as significant in the significance map.",
    "smbrr_significant_add": "Perform conditionally masked addition: add elements of C to B only where C is marked significant.",
    "smbrr_subtract": "Perform element-wise matrix subtraction: A = B - C.",
    "smbrr_significant_subtract": "Perform conditionally masked subtraction: subtract elements of C from B only where C is marked significant.",
    "smbrr_add_value": "Add a constant scalar value to every pixel in the data context.",
    "smbrr_subtract_value": "Subtract a constant scalar value from every pixel in the data context.",
    "smbrr_mult_value": "Multiply every pixel in the data context by a constant scalar value.",
    "smbrr_set_value": "Overwrite every pixel in the data context with a constant scalar value.",
    "smbrr_significant_set_value": "Overwrite pixels in the data context with a constant scalar value only where marked significant.",
    "smbrr_significant_set_svalue": "Assign a fixed threshold value to all elements in a significance map.",
    "smbrr_convert": "Cast the data context between different underlying numerical representations (e.g. from UINT32 to FLOAT).",
    "smbrr_zero_negative": "Perform a mathematical ReLU-like operation, clamping all negative pixel values to zero.",
    "smbrr_abs": "Convert all pixel values in the context to their absolute magnitudes.",
    "smbrr_signed": "Transfer the mathematical sign bit from elements in context N to corresponding elements in context S.",
    "smbrr_copy": "Perform a block memory copy of pixel data between two identical contexts.",
    "smbrr_significant_copy": "Copy pixels from the source context to the destination context exclusively where the significance map is non-zero.",
    "smbrr_mult_add": "Perform a fused multiply-add operation: add the product of context B and scalar C to context A.",
    "smbrr_mult_subtract": "Perform a fused multiply-subtract operation: subtract the product of context B and scalar C from context A.",
    "smbrr_anscombe": "Apply an Anscombe variance-stabilizing transformation to convert Poisson noise into approximately Gaussian noise.",
    "smbrr_significant_new": "Generate a boolean significance map S by thresholding context A against a given sigma value.",
    "smbrr_psf": "Apply a Point Spread Function (PSF) convolution to the source data to simulate or correct optical blurring.",
    "smbrr_get_adu_at_posn": "Extract the pixel value computationally at the specific 2D (x, y) Cartesian coordinate.",
    "smbrr_get_adu_at_offset": "Extract the pixel value computationally at a specific 1D linear offset.",
    "smbrr_reconstruct": "Iteratively rebuild the data context using wavelet convolutions targeting noise-free threshold limits.",
    "smbrr_wavelet_new": "Allocate a hierarchical wavelet context comprising multiple resolution scales decomposed from the original data.",
    "smbrr_wavelet_new_from_object": "Construct a bounding sub-region wavelet scale hierarchy focused exclusively on a pre-detected astronomical object.",
    "smbrr_wavelet_free": "Safely deallocate the wavelet context hierarchy and all its dynamically linked scale objects.",
    "smbrr_wavelet_convolution": "Execute an A'trous or PSF smoothing convolution recursively across the wavelet scales to separate detail frequencies.",
    "smbrr_wavelet_significant_convolution": "Execute smoothing convolution recursively across scales, masking operations strictly within significant pixels to isolate signal from noise.",
    "smbrr_wavelet_deconvolution": "Recombine detail coefficients across the wavelet scales to reconstruct the original merged signal.",
    "smbrr_wavelet_significant_deconvolution": "Recombine detail coefficients across scales to reconstruct a merged signal, filtering out components not marked as significant.",
    "smbrr_wavelet_deconvolution_object": "Recombine coefficients spatially restricted to a segmented object's coordinates, retaining only significant values.",
    "smbrr_wavelet_get_scale": "Retrieve the raw data context representing the specific smoothed resolution level (scale) within the wavelet hierarchy.",
    "smbrr_wavelet_get_wavelet": "Retrieve the data context representing the detail coefficients (wavelet difference) at a specific hierarchical scale.",
    "smbrr_wavelet_get_significant": "Retrieve the binary significance map distinguishing real signal from background noise at a specific wavelet scale.",
    "smbrr_wavelet_add": "Perform element-wise addition recursively across all corresponding scales of the wavelet structures.",
    "smbrr_wavelet_subtract": "Perform element-wise subtraction recursively across all corresponding scales of the wavelet structures.",
    "smbrr_wavelet_significant_subtract": "Subtract coefficients of wavelet C from B across all scales, strictly masked by the significance mapping of C.",
    "smbrr_wavelet_significant_add": "Add coefficients of wavelet C to B across all scales, strictly masked by the significance mapping of C.",
    "smbrr_wavelet_new_significant": "Apply K-sigma thresholding across all wavelet scales to dynamically map statistically significant structures.",
    "smbrr_wavelet_ksigma_clip": "Iteratively threshold each wavelet scale using standard pre-calculated K-sigma deviation coefficients until convergence.",
    "smbrr_wavelet_ksigma_clip_custom": "Iteratively threshold each wavelet scale using user-supplied deviation coefficients until convergence.",
    "smbrr_wavelet_structure_find": "Perform a connected-component analysis on the significance map at a specific scale to logically group contiguous structural pixels.",
    "smbrr_wavelet_structure_connect": "Build a relational tree matching overlapping structures between consecutive wavelet layers to classify multi-scale objects.",
    "smbrr_wavelet_object_get": "Access the classification and boundary parameter data for a globally identified structural object by its ID.",
    "smbrr_wavelet_object_free_all": "Recursively deallocate memory assigned to structural clusters and mathematical objects mapped within the wavelet context.",
    "smbrr_wavelet_object_get_data": "Extract the reconstructed pixel values isolated specifically within the confines of the detected object.",
    "smbrr_wavelet_get_object_at_posn": "Identify and return the highest-priority object whose bounding constraints overlap the given 2D coordinates.",
    "smbrr_wavelet_set_dark_mean": "Manually inject a static background dark noise mean to calibrate object SNR logic.",
    "smbrr_wavelet_set_ccd": "Define device-specific physical parameters (gain, bias, readout constraints) for precise Anscombe variance transformations.",
    "smbrr_wavelet_set_elems": "Seed the foundational layer (Scale 0) of the wavelet hierarchy with raw input signal data."
}

with open("include/sombrero.h", "r") as f:
    lines = f.readlines()

for fun, desc in fun_desc.items():
    # Find the line with the function declaration
    fun_line_idx = -1
    for i, line in enumerate(lines):
        # A simple check: if fun is in the line string, and it is followed by "(" optionally with spaces
        # because the function prototype might be split over multiple lines
        idx = line.find(fun)
        if idx != -1:
            rest = line[idx + len(fun):].lstrip()
            if rest.startswith("("):
                fun_line_idx = i
                break

    if fun_line_idx == -1:
        continue # Function not found?

    # Go backwards to find \brief
    brief_start = -1
    for j in range(fun_line_idx - 1, -1, -1):
        if r"\brief" in lines[j]:
            brief_start = j
            break
        elif "*/" in lines[j] and j != fun_line_idx - 1:
             # Stop if we cross into another comment block
             break

    if brief_start != -1:
        # Find where brief ends
        brief_end = -1
        for k in range(brief_start + 1, fun_line_idx):
            if any(tag in lines[k] for tag in [r"\param", r"\return", r"\ingroup", "*/", r"\defgroup"]):
                brief_end = k
                break
        
        if brief_end == -1:
            continue

        # create properly padded text
        indent = lines[brief_start].find("*")
        prefix = " " * indent + "* "
        wrapped = textwrap.wrap(r"\brief " + desc, width=80 - len(prefix))
        
        new_lines = []
        for w in wrapped:
            new_lines.append(prefix + w + "\n")
            
        # replace the lines from brief_start to brief_end
        lines[brief_start:brief_end] = new_lines

with open("include/sombrero.h", "w") as f:
    f.writelines(lines)
