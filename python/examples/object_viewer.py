#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright (C) 2026 Liam Girdwood <lgirdwood@gmail.com>

import sys
import os
import ctypes
from ctypes import byref, c_int, c_float

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Gdk", "4.0")
from gi.repository import Gtk, Gdk, Gio, GObject, GLib
import cairo

class SmbrrNode(GObject.Object):
    def __init__(self, name, info, position, depth, internal_id=None, parent_id=None):
        super().__init__()
        self._name = name
        self._info = info
        self._position = position
        self.depth = depth
        self.internal_id = internal_id
        self.parent_id = parent_id
        self.children = Gio.ListStore(item_type=SmbrrNode)

    @GObject.Property(type=str)
    def name(self): return self._name
    @GObject.Property(type=str)
    def info(self): return self._info
    @GObject.Property(type=str)
    def position(self): return self._position

def create_row(item):
    # Always return the ListStore so the TreeListModel can react to it changing.
    # If we return None, it gets permanently marked as a leaf node.
    return item.children

def setup_name_cb(factory, list_item):
    expander = Gtk.TreeExpander()
    label = Gtk.Label(halign=Gtk.Align.START)
    expander.set_child(label)
    list_item.set_child(expander)

def bind_name_cb(factory, list_item):
    expander = list_item.get_child()
    row_item = list_item.get_item()
    if row_item:
        expander.set_list_row(row_item)
        label = expander.get_child()
        node = row_item.get_item()
        label.set_text(node.name)

def setup_info_cb(factory, list_item):
    label = Gtk.Label(halign=Gtk.Align.START)
    list_item.set_child(label)

def bind_info_cb(factory, list_item):
    label = list_item.get_child()
    row_item = list_item.get_item()
    if row_item:
        node = row_item.get_item() if hasattr(row_item, 'get_item') else row_item
        label.set_text(node.info)

def bind_pos_cb(factory, list_item):
    label = list_item.get_child()
    row_item = list_item.get_item()
    if row_item:
        node = row_item.get_item() if hasattr(row_item, 'get_item') else row_item
        label.set_text(node.position)
import numpy as np
try:
    from astropy.io import fits
    HAS_FITS = True
except ImportError:
    HAS_FITS = False

# Import libsombrero Python bindings
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
import sombrero

class ProcessingDialog(Gtk.Dialog):
    def __init__(self, parent):
        super().__init__(title="Processing Parameters", transient_for=parent)
        self.add_button("Cancel", Gtk.ResponseType.CANCEL)
        self.add_button("OK", Gtk.ResponseType.OK)

        box = self.get_content_area()
        grid = Gtk.Grid(column_spacing=10, row_spacing=10, margin_top=10, margin_bottom=10, margin_start=10, margin_end=10)
        box.append(grid)

        # Scales
        grid.attach(Gtk.Label(label="Scales:"), 0, 0, 1, 1)
        self.scales_entry = Gtk.SpinButton.new_with_range(1, 12, 1)
        self.scales_entry.set_value(9)
        grid.attach(self.scales_entry, 1, 0, 1, 1)

        # K-Sigma Clip
        grid.attach(Gtk.Label(label="K-Sigma Clip:"), 0, 1, 1, 1)
        self.k_entry = Gtk.SpinButton.new_with_range(0, 5, 1)
        self.k_entry.set_value(2)
        grid.attach(self.k_entry, 1, 1, 1, 1)

        # Sigma delta
        grid.attach(Gtk.Label(label="Sigma Delta:"), 0, 2, 1, 1)
        self.s_entry = Gtk.SpinButton.new_with_range(0.0, 10.0, 0.001)
        self.s_entry.set_value(0.0)
        grid.attach(self.s_entry, 1, 2, 1, 1)

        # Anscombe
        self.anscombe_check = Gtk.CheckButton(label="Anscombe Transform")
        grid.attach(self.anscombe_check, 0, 3, 2, 1)

        # Gain
        grid.attach(Gtk.Label(label="Gain:"), 0, 4, 1, 1)
        self.gain_entry = Gtk.SpinButton.new_with_range(0.0, 1000.0, 0.1)
        self.gain_entry.set_value(5.0)
        grid.attach(self.gain_entry, 1, 4, 1, 1)

        # Bias
        grid.attach(Gtk.Label(label="Bias:"), 0, 5, 1, 1)
        self.bias_entry = Gtk.SpinButton.new_with_range(0.0, 1000.0, 0.1)
        self.bias_entry.set_value(50.0)
        grid.attach(self.bias_entry, 1, 5, 1, 1)

        # Readout
        grid.attach(Gtk.Label(label="Readout Noise:"), 0, 6, 1, 1)
        self.readout_entry = Gtk.SpinButton.new_with_range(0.0, 1000.0, 0.1)
        self.readout_entry.set_value(100.0)
        grid.attach(self.readout_entry, 1, 6, 1, 1)

    def get_params(self):
        return {
            'scales': self.scales_entry.get_value_as_int(),
            'k': self.k_entry.get_value_as_int(),
            's': self.s_entry.get_value(),
            'anscombe': self.anscombe_check.get_active(),
            'gain': self.gain_entry.get_value(),
            'bias': self.bias_entry.get_value(),
            'readout': self.readout_entry.get_value()
        }

class SombreroViewer(Gtk.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.set_title("Libsombrero Object Viewer")
        self.set_default_size(800, 600)

        # Main layout
        self.box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.set_child(self.box)

        # Header bar
        header = Gtk.HeaderBar()
        self.set_titlebar(header)

        open_btn = Gtk.Button(label="Open Image")
        open_btn.connect("clicked", self.on_open_clicked)
        header.pack_start(open_btn)

        self.process_btn = Gtk.Button(label="Detect Objects")
        self.process_btn.connect("clicked", self.on_process_clicked)
        self.process_btn.set_sensitive(False)
        header.pack_start(self.process_btn)

        self.show_circles_btn = Gtk.CheckButton(label="Show Structures")
        self.show_circles_btn.set_active(False)
        self.show_circles_btn.connect("toggled", self.on_show_circles_toggled)
        header.pack_start(self.show_circles_btn)

        self.show_objects_btn = Gtk.CheckButton(label="Show Objects")
        self.show_objects_btn.set_active(False)
        self.show_objects_btn.connect("toggled", self.on_show_objects_toggled)
        header.pack_start(self.show_objects_btn)

        # Left panel: ColumnView for structures
        self.tree_store = Gio.ListStore(item_type=SmbrrNode)
        self.tree_model = Gtk.TreeListModel.new(self.tree_store, False, True, create_row)
        self.selection_model = Gtk.SingleSelection.new(self.tree_model)
        self.tree_view = Gtk.ColumnView(model=self.selection_model)

        factory_name = Gtk.SignalListItemFactory()
        factory_name.connect("setup", setup_name_cb)
        factory_name.connect("bind", bind_name_cb)
        col_name = Gtk.ColumnViewColumn(title="Name", factory=factory_name)
        col_name.set_expand(True)
        self.tree_view.append_column(col_name)

        factory_info = Gtk.SignalListItemFactory()
        factory_info.connect("setup", setup_info_cb)
        factory_info.connect("bind", bind_info_cb)
        col_info = Gtk.ColumnViewColumn(title="Info", factory=factory_info)
        self.tree_view.append_column(col_info)

        factory_pos = Gtk.SignalListItemFactory()
        factory_pos.connect("setup", setup_info_cb)
        factory_pos.connect("bind", bind_pos_cb)
        col_pos = Gtk.ColumnViewColumn(title="Position", factory=factory_pos)
        self.tree_view.append_column(col_pos)
        
        self.selection_model.connect("selection-changed", self.on_tree_selection_changed)

        left_scrolled = Gtk.ScrolledWindow()
        left_scrolled.set_child(self.tree_view)
        left_scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        left_scrolled.set_size_request(300, -1)

        # Right panel: Drawing Area to display image + objects
        self.drawing_area = Gtk.DrawingArea()
        self.drawing_area.set_draw_func(self.on_draw)
        self.drawing_area.set_size_request(500, 500)

        right_scrolled = Gtk.ScrolledWindow()
        right_scrolled.set_child(self.drawing_area)
        right_scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)

        # Paned layout
        self.paned = Gtk.Paned(orientation=Gtk.Orientation.HORIZONTAL)
        self.paned.set_start_child(left_scrolled)
        self.paned.set_end_child(right_scrolled)
        self.paned.set_position(300)
        self.paned.set_hexpand(True)
        self.paned.set_vexpand(True)

        self.box.append(self.paned)

        # Status bar
        self.status_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.status_bar.set_margin_start(5)
        self.status_bar.set_margin_end(5)
        self.status_bar.set_margin_top(2)
        self.status_bar.set_margin_bottom(2)
        
        self.status_label = Gtk.Label(label="Ready")
        self.status_label.set_halign(Gtk.Align.START)
        self.status_bar.append(self.status_label)
        
        self.box.append(self.status_bar)

        # Click handling
        self.click_gesture = Gtk.GestureClick.new()
        self.click_gesture.set_button(0) # All buttons
        self.click_gesture.connect("pressed", self.on_click)
        self.drawing_area.add_controller(self.click_gesture)

        # Motion handling
        self.motion_ctrl = Gtk.EventControllerMotion.new()
        self.motion_ctrl.connect("motion", self.on_motion)
        self.motion_ctrl.connect("enter", self.on_enter)
        self.motion_ctrl.connect("leave", self.on_leave)
        self.drawing_area.add_controller(self.motion_ctrl)

        # Application state
        self.image_surface = None
        self.image_width = 0
        self.image_height = 0
        self.objects = []
        self.structures = []
        self.show_structures = False
        self.show_objects = False
        self.tree_root = None

        self.filter_scale = None
        self.filter_id = None
        self.filter_sigall = False
        self.filter_reconstruct = False

        self.scale_surfaces = {}
        self.sigall_surface = None
        self.reconstruct_surface = None

        self.current_filepath = None
        self.current_raw_data = None
        self.current_width = 0
        self.current_height = 0
        self.current_adu_type = 0
        self.current_is_fits = False

    def on_open_clicked(self, button):
        dialog = Gtk.FileDialog(title="Select a FITS or BMP Image")
        filters = Gio.ListStore.new(Gtk.FileFilter)
        
        filter_img = Gtk.FileFilter()
        filter_img.set_name("Images (*.bmp, *.fit, *.fits)")
        filter_img.add_pattern("*.bmp")
        filter_img.add_pattern("*.fit")
        filter_img.add_pattern("*.fits")
        filters.append(filter_img)

        dialog.set_filters(filters)
        dialog.open(self, None, self.on_file_selected)

    def on_file_selected(self, dialog, result):
        try:
            file = dialog.open_finish(result)
            if file:
                self.load_image(file.get_path())
        except Exception as e:
            print(f"Error opening file: {e}")

    def on_process_clicked(self, button):
        self.show_processing_dialog()

    def show_processing_dialog(self):
        dialog = ProcessingDialog(self)
        dialog.connect("response", self.on_processing_dialog_response)
        dialog.present()

    def on_processing_dialog_response(self, dialog, response):
        if response == Gtk.ResponseType.OK:
            params = dialog.get_params()
            dialog.destroy()
            self.process_sombrero(params)
        else:
            dialog.destroy()

    def on_show_circles_toggled(self, button):
        self.show_structures = button.get_active()
        self.drawing_area.queue_draw()

    def on_show_objects_toggled(self, button):
        self.show_objects = button.get_active()
        self.drawing_area.queue_draw()

    def load_image(self, filepath):
        print(f"Loading {filepath}...")
        self.objects = []
        self.structures = []
        self.show_structures = False
        self.show_objects = False
        self.scale_surfaces = {}
        self.surface_buffers = []
        self.sigall_surface = None
        self.reconstruct_surface = None
        if hasattr(self, 'tree_store'):
            self.tree_store.remove_all()
        
        is_fits = filepath.lower().endswith(('.fit', '.fits'))
        is_bmp = filepath.lower().endswith('.bmp')
        
        if is_fits and not HAS_FITS:
            print("Astropy is required to load FITS files. Please install it.")
            return

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
                        
                        img_min = np.min(img_data)
                        img_max = np.max(img_data)
                        if img_max > img_min:
                            norm_data = (img_data - img_min) / (img_max - img_min) * 255.0
                        else:
                            norm_data = img_data
                        
                        vis_data = norm_data.astype(np.uint8)
                        
                        float_data = img_data.astype(np.float32)
                        raw_data = float_data.tobytes()
                        adu_type = 3 # FLOAT type in libsmbrr

                        cairo_data = np.zeros((height, width, 4), dtype=np.uint8)
                        cairo_data[:, :, 0] = vis_data
                        cairo_data[:, :, 1] = vis_data
                        cairo_data[:, :, 2] = vis_data
                        cairo_data[:, :, 3] = 255
                        
                        self.image_width = width
                        self.image_height = height
                        
                        format_val = cairo.FORMAT_ARGB32
                        stride = cairo.ImageSurface.format_stride_for_width(format_val, width)
                        
                        surface_data = cairo_data.tobytes()
                        self.cairo_buffer = bytearray(surface_data)
                        surface = cairo.ImageSurface.create_for_data(self.cairo_buffer, format_val, width, height, stride)
                        self.image_surface = surface

        elif is_bmp:
            try:
                texture = Gdk.Texture.new_from_filename(filepath)
                self.image_width = texture.get_width()
                self.image_height = texture.get_height()
                
                surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, self.image_width, self.image_height)
                cr = cairo.Context(surface)
                
                from gi.repository import GdkPixbuf
                pixbuf = GdkPixbuf.Pixbuf.new_from_file(filepath)
                Gdk.cairo_set_source_pixbuf(cr, pixbuf, 0, 0)
                cr.paint()
                
                self.image_surface = surface

                # We also need to extract BMP data for raw processing like previously
                with open(filepath, "rb") as f:
                    header = f.read(54)
                    if header[:2] == b"BM":
                        off_bits = int.from_bytes(header[10:14], byteorder='little')
                        width = int.from_bytes(header[18:22], byteorder='little')
                        height = int.from_bytes(header[22:26], byteorder='little')
                        f.seek(off_bits)
                        raw_data = f.read()
                        adu_type = 0 # UINT8
                
            except Exception as e:
                print(f"Could not load visual texture: {e}")
                return

        if self.image_surface:
            self.current_filepath = filepath
            self.current_raw_data = raw_data
            self.current_width = width
            self.current_height = height
            self.current_adu_type = adu_type
            self.current_is_fits = is_fits

            self.drawing_area.set_size_request(self.image_width, self.image_height)
            self.drawing_area.queue_draw()
            
            filename = os.path.basename(filepath)
            self.tree_root = SmbrrNode(filename, f"{width}x{height}", "", 1)
            
            self.tree_store.append(self.tree_root)
            
            self.process_btn.set_sensitive(True)
            self.show_processing_dialog()

    def process_sombrero(self, params):
        if not self.current_raw_data:
            print("No raw image data loaded to process.")
            return

        print(f"Invoking libsombrero analysis with params: {params}")
        
        data_buffer = ctypes.create_string_buffer(self.current_raw_data, len(self.current_raw_data))
        
        img = sombrero.smbrr.smbrr_new(3, self.current_width, self.current_height, 0, self.current_adu_type, data_buffer)
        if not img:
            print("Failed to initialize smbrr context")
            return

        if params['anscombe']:
            sombrero.smbrr.smbrr_anscombe(img, params['gain'], params['bias'], params['readout'])

        w = sombrero.smbrr.smbrr_wavelet_new(img, params['scales'])

        # 1. Convolution
        sombrero.smbrr.smbrr_wavelet_convolution(w, sombrero.SMBRR_CONV_ATROUS, sombrero.SMBRR_WAVELET_MASK_LINEAR)

        # 2. k-Sigma Clipping
        sombrero.smbrr.smbrr_wavelet_ksigma_clip(w, params['k'], params['s'])

        # 3. Find structures across scales
        for i in range(params['scales'] - 1):
            sombrero.smbrr.smbrr_wavelet_structure_find(w, i)
        
        # 4. Perform deblending/connecting
        sombrero.smbrr.smbrr_wavelet_structure_connect(w, 0, params['scales'] - 2)

        # 5. Extract images for rendering
        def create_surface_from_smbrr(s_ptr, width, height, name):
            if not s_ptr:
                return None
            
            # Find min and max for normalization
            data_min = ctypes.c_float()
            data_max = ctypes.c_float()
            sombrero.smbrr.smbrr_find_limits(s_ptr, ctypes.byref(data_min), ctypes.byref(data_max))
            
            # Float data ptr
            data_p = ctypes.cast(sombrero.smbrr.smbrr_get_data(s_ptr, 3, None), ctypes.POINTER(ctypes.c_float))
            
            # We must use ctypes to allocate the buffer first! The C code does memcpy(*buf, ...)
            # so *buf must point to valid memory of size (width * height * sizeof(float)).
            buffer_size = width * height * ctypes.sizeof(ctypes.c_float)
            data_buffer = ctypes.create_string_buffer(buffer_size)
            
            data_out = ctypes.cast(data_buffer, ctypes.c_void_p)
            sombrero.smbrr.smbrr_get_data(s_ptr, 3, ctypes.byref(data_out))
            
            float_ptr = ctypes.cast(data_out, ctypes.POINTER(ctypes.c_float))
            
            # We copy data manually into numpy, normally we'd want to avoid copies if possible
            arr = np.ctypeslib.as_array(float_ptr, shape=(height, width))
            
            # libsombrero copies data in a way that renders upside-down when converted
            # back to Cairo/GTK coordinates (which are top-left origin).
            arr = np.flipud(arr)
            
            d_min = data_min.value
            d_max = data_max.value
            
            print(f"create_surface_from_smbrr limits for {name}: {d_min} to {d_max}")
            
            if d_max > d_min:
                norm_data = (arr - d_min) / (d_max - d_min) * 255.0
            else:
                norm_data = np.zeros_like(arr)
                
            vis_data = norm_data.astype(np.uint8)
            
            cairo_data = np.zeros((height, width, 4), dtype=np.uint8)
            cairo_data[:, :, 0] = vis_data
            cairo_data[:, :, 1] = vis_data
            cairo_data[:, :, 2] = vis_data
            cairo_data[:, :, 3] = 255
            
            format_val = cairo.FORMAT_ARGB32
            stride = cairo.ImageSurface.format_stride_for_width(format_val, width)
            surface_data = cairo_data.tobytes()
            # Store buffer to prevent garbage collection
            surface_buffer = bytearray(surface_data)
            surface = cairo.ImageSurface.create_for_data(surface_buffer, format_val, width, height, stride)
            
            return surface, surface_buffer

        self.scale_surfaces = {}
        self.surface_buffers = []
        for i in range(params['scales']):
            s_ptr = sombrero.smbrr.smbrr_wavelet_get_scale(w, i)
            s_res = create_surface_from_smbrr(s_ptr, self.current_width, self.current_height, f"scale {i}")
            if s_res:
                self.scale_surfaces[i] = s_res[0]
                self.surface_buffers.append(s_res[1])

        # For sigall we need to merge the significant maps or create a presentation map
        # Build the composite sigall image by accumulating the significant maps with weightings,
        # identical to how usb_camera_sigall.py and structures.c do it.
        sigall_img = sombrero.smbrr.smbrr_new(3, self.current_width, self.current_height, 0, self.current_adu_type, None)
        
        for i in range(params['scales'] - 1):
            simage = sombrero.smbrr.smbrr_wavelet_get_significant(w, ctypes.c_uint(i))
            if simage:
                val = 16.0 + (1 << ((params['scales'] - 1) - i))
                sombrero.smbrr.smbrr_significant_add_value(sigall_img, simage, ctypes.c_float(val))

        sombrero.smbrr.smbrr_normalise(sigall_img, ctypes.c_float(0.0), ctypes.c_float(250.0))
        
        s_res = create_surface_from_smbrr(sigall_img, self.current_width, self.current_height, "sigall")
        if s_res:
            self.sigall_surface = s_res[0]
            self.surface_buffers.append(s_res[1])
        sombrero.smbrr.smbrr_free(sigall_img)

        # Build reconstructed image
        reconstructed = sombrero.smbrr.smbrr_new(3, self.current_width, self.current_height, 0, self.current_adu_type, None)
        sombrero.smbrr.smbrr_copy(reconstructed, img)
        sombrero.smbrr.smbrr_reconstruct(reconstructed, sombrero.SMBRR_WAVELET_MASK_LINEAR, ctypes.c_float(1.0e-4), params['scales'], params['k'])
        
        sombrero.smbrr.smbrr_normalise(reconstructed, ctypes.c_float(0.0), ctypes.c_float(255.0))
        
        s_res = create_surface_from_smbrr(reconstructed, self.current_width, self.current_height, "reconstruct")
        if s_res:
            self.reconstruct_surface = s_res[0]
            self.surface_buffers.append(s_res[1])
        sombrero.smbrr.smbrr_free(reconstructed)

        # Populate tree with structures
        self.tree_store.remove_all()
        basename = os.path.basename(self.current_filepath) if self.current_filepath else "Image"
        self.tree_root = SmbrrNode(basename, "", "", 1)
        
        # Add a 'sigall' node
        sigall_iter = SmbrrNode("sigall", "All Scales", "", 2)
        self.tree_root.children.append(sigall_iter)
        
        # Add 'reconstruct' node
        reconstruct_iter = SmbrrNode("reconstruct", "Reconstructed Image", "", 2)
        self.tree_root.children.append(reconstruct_iter)
                
        for i in range(params['scales'] - 1):
            num_structures = sombrero.smbrr.smbrr_wavelet_get_num_structures(w, i)
            print(f"Scale {i} found {num_structures} structures.")
            scale_iter = SmbrrNode(f"Scale {i}", f"{num_structures} structs", "", 2)
            for j in range(num_structures):
                struct_data = sombrero.SmbrrStructure()
                if sombrero.smbrr.smbrr_wavelet_get_structure(w, i, j, ctypes.byref(struct_data)) == 0:
                    sy = self.current_height - int(struct_data.pos.y) if not self.current_is_fits else int(struct_data.pos.y)
                    struct_node = SmbrrNode(
                        f"Id: {struct_data.id}",
                        f"Max: {struct_data.max_value:.1f}, Area: {struct_data.size}",
                        f"({struct_data.pos.x}, {sy})",
                        3,
                        struct_data.id,
                        i
                    )
                    scale_iter.children.append(struct_node)
                    self.structures.append({
                        "x": int(struct_data.pos.x),
                        "y": sy,
                        "radius": max(1.0, (struct_data.size / 3.14159) ** 0.5 * 2.0),
                        "id": struct_data.id,
                        "scale": i
                    })
            self.tree_root.children.append(scale_iter)
            print(f"Appended scale {i} to root with {scale_iter.children.get_n_items()} child structs.")
            
        print(f"Adding tree_root ({self.tree_root.name}) to tree_store with {self.tree_root.children.get_n_items()} children")
        self.tree_store.append(self.tree_root)
        self.tree_view.queue_draw()
        
        objects_found = []
        for i in range(10000): # Hard limit guard
            obj_ptr = sombrero.smbrr.smbrr_wavelet_object_get(w, i)
            if not obj_ptr:
                break
            
            obj = obj_ptr.contents
            y_coord = self.current_height - int(obj.pos.y) if not self.current_is_fits else int(obj.pos.y)
            objects_found.append({
                "x": int(obj.pos.x),
                "y": y_coord,
                "radius": float(obj.object_radius),
                "id": obj.id,
                "scale": obj.scale
            })
            
        print(f"Found {len(objects_found)} objects.")
        self.objects = objects_found
        
        self.show_structures = True
        self.show_circles_btn.set_active(True)
        self.show_objects = True
        self.show_objects_btn.set_active(True)
        
        self.drawing_area.queue_draw()
        
        # Cleanup
        sombrero.smbrr.smbrr_wavelet_free(w)
        sombrero.smbrr.smbrr_free(img)

    def on_click(self, gesture, n_press, x, y):
        if self.image_surface:
            print(f"Clicked at {x}, {y}")
            # Toggle structures on click
            self.show_circles_btn.set_active(not self.show_structures)

    def on_motion(self, controller, x, y):
        if not self.image_surface:
            return
            
        widget_width = self.drawing_area.get_width()
        widget_height = self.drawing_area.get_height()
        
        if widget_width == 0 or widget_height == 0:
            return
            
        scale_x = widget_width / self.image_width
        scale_y = widget_height / self.image_height
        
        img_x = int(x / scale_x)
        img_y = int(y / scale_y)
        
        val_str = ""
        if 0 <= img_x < self.current_width and 0 <= img_y < self.current_height:
            if self.current_is_fits and self.current_raw_data:
                import struct
                offset = (img_y * self.current_width + img_x) * 4
                if offset + 4 <= len(self.current_raw_data):
                    val = struct.unpack('f', self.current_raw_data[offset:offset+4])[0]
                    val_str = f" | Pixel Value: {val:.4f}"
            else:
                # For BMPs or any other format, read directly from the Cairo surface data we created
                # when loading the texture/surface.
                if self.image_surface:
                    try:
                        surface_data = self.image_surface.get_data()
                        stride = self.image_surface.get_stride()
                        offset = img_y * stride + img_x * 4
                        if offset + 4 <= len(surface_data):
                            b = surface_data[offset]
                            g = surface_data[offset+1]
                            r = surface_data[offset+2]
                            val_str = f" | RGB: ({r}, {g}, {b})"
                    except Exception:
                        pass
                elif hasattr(self, 'cairo_buffer') and self.cairo_buffer:
                    stride = self.image_surface.get_stride()
                    offset = img_y * stride + img_x * 4
                    if offset + 4 <= len(self.cairo_buffer):
                        b = self.cairo_buffer[offset]
                        g = self.cairo_buffer[offset+1]
                        r = self.cairo_buffer[offset+2]
                        val_str = f" | RGB: ({r}, {g}, {b})"
                
        self.status_label.set_text(f"X: {img_x}, Y: {img_y}{val_str}")
        
    def on_enter(self, controller, x, y):
        self.drawing_area.set_cursor(Gdk.Cursor.new_from_name("crosshair"))
        
    def on_leave(self, controller):
        self.drawing_area.set_cursor(None)
        self.status_label.set_text("Ready")

    def on_tree_selection_changed(self, selection, position, n_items):
        item = selection.get_selected_item()
        if not item:
            return
            
        node = item.get_item()
        if not node:
            return
            
        depth = node.depth
        
        if depth == 1:
            # Root image selected
            self.filter_scale = None
            self.filter_id = None
            self.filter_sigall = False
            self.filter_reconstruct = False
        elif depth == 2:
            # Scale selected or sigall
            scale_str = node.name
            if scale_str == "sigall":
                self.filter_scale = None
                self.filter_id = None
                self.filter_sigall = True
                self.filter_reconstruct = False
            elif scale_str == "reconstruct":
                self.filter_scale = None
                self.filter_id = None
                self.filter_sigall = False
                self.filter_reconstruct = True
            else:
                self.filter_sigall = False
                self.filter_reconstruct = False
                try:
                    self.filter_scale = int(scale_str.replace("Scale ", ""))
                    self.filter_id = None
                except ValueError:
                    pass
        elif depth == 3:
            # Individual structure selected
            self.filter_sigall = False
            self.filter_reconstruct = False
            self.filter_id = node.internal_id
            self.filter_scale = node.parent_id
        
        # The user requested checkboxes not to change state when the list view selection changes.
        # So we just request a redraw to respect the existing toggle states.
        self.drawing_area.queue_draw()

    def on_draw(self, area, cr, width, height):
        print(f"on_draw: {width}x{height}, filters={self.filter_scale}/{self.filter_id}, objects={len(self.objects)} structs={len(self.structures)}")
        if self.image_surface:
            scale_x = width / self.image_width
            scale_y = height / self.image_height

            cr.save()
            # cr.scale(scale_x, scale_y) # Removed because GTK4 snapshot clips scaled cairo contexts unpredictably
            
            surface_to_draw = self.image_surface
            if self.filter_sigall and self.sigall_surface:
                surface_to_draw = self.sigall_surface
            elif self.filter_reconstruct and self.reconstruct_surface:
                surface_to_draw = self.reconstruct_surface
            elif self.filter_scale is not None and self.filter_scale in self.scale_surfaces:
                surface_to_draw = self.scale_surfaces[self.filter_scale]

            # Scale the surface matrix before painting
            matrix = cairo.Matrix()
            matrix.scale(1.0 / scale_x, 1.0 / scale_y)
            surface_to_draw.set_device_offset(0, 0)
            
            cr.set_source_surface(surface_to_draw, 0, 0)
            cr.get_source().set_matrix(matrix)
            cr.paint()

            if self.show_structures:
                line_scale = max(scale_x, scale_y)
                if line_scale > 0:
                    cr.set_line_width(2.0)

                colors = [
                    (1.0, 0.0, 0.0, 0.8), # Red
                    (0.0, 1.0, 0.0, 0.8), # Green
                    (0.3, 0.3, 1.0, 0.8), # Blue
                    (1.0, 1.0, 0.0, 0.8), # Yellow
                    (1.0, 0.0, 1.0, 0.8), # Magenta
                    (0.0, 1.0, 1.0, 0.8), # Cyan
                    (1.0, 0.5, 0.0, 0.8), # Orange
                    (0.5, 0.0, 1.0, 0.8), # Purple
                    (0.0, 1.0, 0.5, 0.8), # Spring Green
                    (1.0, 0.0, 0.5, 0.8), # Pink
                ]

                items_to_draw = self.structures
                
                for item in items_to_draw:
                    if self.filter_sigall:
                        # Draw everything if sigall is selected
                        pass
                    elif self.filter_id is not None:
                        if item["id"] != self.filter_id:
                            continue
                    elif self.filter_scale is not None:
                        if item["scale"] != self.filter_scale:
                            continue
                            
                    scale = int(item.get("scale", 0))
                    r, g, b, a = colors[scale % len(colors)]
                    cr.set_source_rgba(r, g, b, a)

                    cr.new_path()
                    cr.arc(item["x"] * scale_x, item["y"] * scale_y, item["radius"] * line_scale, 0, 2 * 3.14159)
                    cr.stroke()

            if self.show_objects:
                line_scale = max(scale_x, scale_y)
                if line_scale > 0:
                    cr.set_line_width(2.0)
                
                # Dashed blue circles for true extracted objects
                cr.set_source_rgba(0.0, 0.0, 1.0, 0.8) # Blue
                cr.set_dash([5.0, 5.0])
                
                for item in self.objects:
                    # Filter logic if you want to apply tree filtering to detected objects as well
                    if self.filter_scale is not None and item["scale"] != self.filter_scale:
                        continue
                        
                    cr.new_path()
                    cr.arc(item["x"] * scale_x, item["y"] * scale_y, item["radius"] * line_scale, 0, 2 * 3.14159)
                    cr.stroke()
                
                # Reset dash for subsequent drawing operations
                cr.set_dash([])
            
            cr.restore()

class SombreroApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id="com.sombrero.viewer",
                         flags=Gio.ApplicationFlags.HANDLES_OPEN)

    def do_activate(self):
        win = self.props.active_window
        if not win:
            win = SombreroViewer(application=self)
        win.present()

    def do_open(self, files, n_files, hint):
        win = self.props.active_window
        if not win:
            win = SombreroViewer(application=self)
        
        if n_files > 0:
            win.load_image(files[0].get_path())
        
        win.present()

if __name__ == "__main__":
    app = SombreroApp()
    try:
        sys.exit(app.run(sys.argv))
    except KeyboardInterrupt:
        sys.exit(0)
