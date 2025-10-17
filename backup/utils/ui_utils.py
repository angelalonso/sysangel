import customtkinter as ctk
from typing import Callable

def create_responsive_grid(parent, rows: int, cols: int):
    """Configure a grid layout that expands properly"""
    for i in range(rows):
        parent.grid_rowconfigure(i, weight=1)
    for j in range(cols):
        parent.grid_columnconfigure(j, weight=1)

def create_section_header(parent, text: str, row: int, column: int = 0, columnspan: int = 1):
    """Create a consistent section header"""
    label = ctk.CTkLabel(parent, 
                        text=text,
                        font=("Arial", 16, "bold"))
    label.grid(row=row, column=column, columnspan=columnspan, sticky="w", pady=(10, 5), padx=10)
    return label

def create_form_field(parent, label_text: str, row: int, widget_creator: Callable, 
                     sticky="w", padx=10, pady=5):
    """Create a consistent form field with label and widget"""
    # Label
    label = ctk.CTkLabel(parent, 
                        text=label_text,
                        font=("Arial", 14))
    label.grid(row=row, column=0, sticky=sticky, padx=padx, pady=pady)
    
    # Widget
    widget = widget_creator(parent)
    widget.grid(row=row, column=1, sticky="ew", padx=padx, pady=pady, columnspan=2)
    
    return widget

def bind_mouse_wheel(widget, scrollable_frame):
    """Bind mouse wheel events to a widget for scrolling"""
    # Bind to the widget itself
    widget.bind("<MouseWheel>", lambda event: _on_mousewheel(event, scrollable_frame))
    widget.bind("<Button-4>", lambda event: _on_mousewheel(event, scrollable_frame))  # Linux scroll up
    widget.bind("<Button-5>", lambda event: _on_mousewheel(event, scrollable_frame))  # Linux scroll down
    
    # Also bind to all existing children
    for child in widget.winfo_children():
        child.bind("<MouseWheel>", lambda event: _on_mousewheel(event, scrollable_frame))
        child.bind("<Button-4>", lambda event: _on_mousewheel(event, scrollable_frame))
        child.bind("<Button-5>", lambda event: _on_mousewheel(event, scrollable_frame))

def _on_mousewheel(event, scrollable_frame):
    """Handle mouse wheel events for scrolling"""
    if event.delta:
        # Windows and macOS
        scrollable_frame._parent_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
    else:
        # Linux
        if event.num == 4:
            scrollable_frame._parent_canvas.yview_scroll(-1, "units")
        elif event.num == 5:
            scrollable_frame._parent_canvas.yview_scroll(1, "units")

def create_modal_dialog(parent, title: str, width: int = 600, height: int = 400):
    """Create a centered modal dialog"""
    dialog = ctk.CTkToplevel(parent)
    dialog.title(title)
    dialog.geometry(f"{width}x{height}")
    dialog.resizable(False, False)  # Prevent resizing for dialogs
    dialog.transient(parent)
    
    # Configure dialog to expand properly
    dialog.grid_rowconfigure(0, weight=1)
    dialog.grid_columnconfigure(0, weight=1)
    
    # Center the dialog
    dialog.update_idletasks()
    x = parent.winfo_x() + (parent.winfo_width() - dialog.winfo_width()) // 2
    y = parent.winfo_y() + (parent.winfo_height() - dialog.winfo_height()) // 2
    dialog.geometry(f"+{x}+{y}")
    
    # Make dialog modal
    dialog.focus_set()
    dialog.grab_set()
    
    return dialog

def setup_scrollable_content(parent):
    """Setup a properly configured scrollable content area"""
    # Main content container that fills available space
    main_content = ctk.CTkFrame(parent)
    main_content.pack(fill="both", expand=True, padx=10, pady=10)
    main_content.grid_rowconfigure(0, weight=1)
    main_content.grid_columnconfigure(0, weight=1)
    
    # Scrollable frame - properly configured to expand
    scrollable_frame = ctk.CTkScrollableFrame(main_content)
    scrollable_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
    
    # Configure scrollable frame to expand
    scrollable_frame.grid_rowconfigure(0, weight=1)
    scrollable_frame.grid_columnconfigure(0, weight=1)
    
    # Bind mouse wheel events
    bind_mouse_wheel(scrollable_frame, scrollable_frame)
    
    return main_content, scrollable_frame
