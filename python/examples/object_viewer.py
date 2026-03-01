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
from gi.repository import Gtk, Gdk, Gio, GLib

import cairo
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

        # Drawing Area to display image + objects
        self.drawing_area = Gtk.DrawingArea()
        self.drawing_area.set_draw_func(self.on_draw)
        self.drawing_area.set_hexpand(True)
        self.drawing_area.set_vexpand(True)

        scrolled = Gtk.ScrolledWindow()
        scrolled.set_child(self.drawing_area)
        self.box.append(scrolled)

        # Click handling
        self.click_gesture = Gtk.GestureClick.new()
        self.click_gesture.set_button(0) # All buttons
        self.click_gesture.connect("pressed", self.on_click)
        self.drawing_area.add_controller(self.click_gesture)

        # Application state
        self.image_surface = None
        self.image_width = 0
        self.image_height = 0
        self.objects = []
        self.show_objects = False

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

    def load_image(self, filepath):
        print(f"Loading {filepath}...")
        self.objects = []
        self.show_objects = False
        
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
                "id": obj.id
            })
            
        print(f"Found {len(objects_found)} objects.")
        self.objects = objects_found
        self.show_objects = True # Auto-show them after process
        self.drawing_area.queue_draw()
        
        # Cleanup
        sombrero.smbrr.smbrr_wavelet_free(w)
        sombrero.smbrr.smbrr_free(img)

    def on_click(self, gesture, n_press, x, y):
        if self.image_surface:
            print(f"Clicked at {x}, {y}")
            self.show_objects = not self.show_objects
            self.drawing_area.queue_draw()

    def on_draw(self, area, cr, width, height):
        if self.image_surface:
            cr.set_source_surface(self.image_surface, 0, 0)
            cr.paint()

            if self.show_objects:
                cr.set_line_width(2.0)
                cr.set_source_rgba(1.0, 0.0, 0.0, 0.8) # Red circles

                for obj in self.objects:
                    cr.arc(obj["x"], obj["y"], obj["radius"], 0, 2 * 3.14159)
                    cr.stroke()

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
