import gi
gi.require_version('Gtk', '4.0')
from gi.repository import Gtk

store = Gtk.StringList.new(["a", "b", "c"])
sel = Gtk.SingleSelection.new(store)
cv = Gtk.ColumnView(model=sel)

try:
    cv.scroll_to(2, None, Gtk.ListScrollFlags.FOCUS, None)
    print("GTK 4.12+ signature worked")
except TypeError:
    try:
        cv.scroll_to(2)
        print("GTK 4.0 basic signature worked")
    except Exception as e:
        print(f"Fallback didn't work: {e}")
except Exception as e:
    print(f"Failed: {e}")
