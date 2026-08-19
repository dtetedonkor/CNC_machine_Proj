# main.py
from PySide6.QtCore import Signal, QRectF,Qt,QObject
from PySide6.QtGui import QPainter, QColor, QFont
from PySide6.QtWidgets import  QSizePolicy,QWidget
from PySide6.QtSvg import QSvgRenderer
from pathlib import Path
# QSvgRenderer is used to render an SVG into the preview canvas.
# It's provided by the QtSvg module. If not available the preview will fall back to text.
try:
    from PySide6.QtSvg import QSvgRenderer  # type: ignore
except Exception:
    QSvgRenderer = None  # type: ignore


# Worker that converts an SVG into G-code using the project's svg_parser module.
# Runs in a QThread so the GUI stays responsive.
class ProcessorWorker(QObject):
    started = Signal()
    finished = Signal(object)  # will emit dict with 'polylines' and 'gcode' keys
    error = Signal(str)

    def __init__(self, svg_path: str, resolution: float = 0.5):
        super().__init__()
        self.svg_path = svg_path
        self.resolution = resolution

    def run(self) -> None:
        self.started.emit()
        try:
            # Import the local svg_parser module that the project provides.
            # It must expose svg_to_gcode(...) or parse_svg_to_polylines/polylines_to_gcode.
            try:
                import svg_parser  # type: ignore
            except Exception as e:
                self.error.emit(f"Failed to import svg_parser: {e}")
                return

            try:
                # Prefer svg_to_gcode convenience wrapper if present
                if hasattr(svg_parser, "svg_to_gcode"):
                    gcode = svg_parser.svg_to_gcode(self.svg_path, resolution=self.resolution)
                    # We can also return polylines if the module provides parse_svg_to_polylines
                    polylines = None
                    if hasattr(svg_parser, "parse_svg_to_polylines"):
                        try:
                            polylines = svg_parser.parse_svg_to_polylines(self.svg_path, resolution=self.resolution)
                        except Exception:
                            polylines = None
                    self.finished.emit({"polylines": polylines, "gcode": gcode})
                else:
                    # Fallback: call parse_svg_to_polylines then polylines_to_gcode
                    if not hasattr(svg_parser, "parse_svg_to_polylines") or not hasattr(svg_parser, "polylines_to_gcode"):
                        self.error.emit("svg_parser missing expected functions (svg_to_gcode or parse_svg_to_polylines + polylines_to_gcode).")
                        return
                    polylines = svg_parser.parse_svg_to_polylines(self.svg_path, resolution=self.resolution)
                    gcode = svg_parser.polylines_to_gcode(polylines)
                    self.finished.emit({"polylines": polylines, "gcode": gcode})
            except Exception as e:
                self.error.emit(f"Error while generating G-code: {e}")
        except Exception as exc:
            self.error.emit(f"Unexpected processor error: {exc}")


# Preview canvas that can render an SVG file (if QSvgRenderer is available)
# or fall back to the previous placeholder drawing.
class PreviewCanvas(QWidget):
    def __init__(self, parent: QWidget = None):
        super().__init__(parent)
        self.filename: str = ""
        self.svg_path: str = ""
        self.renderer: QSvgRenderer = None
        self.setMinimumSize(400, 300)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.bg_color = QColor("#1e1e1e")
        self.fg_color = QColor("#e6e6e6")
        self.box_color = QColor("#4caf50")

    def set_filename(self, name: str = ""):
        self.filename = name
        self.update()

    def load_svg(self, path: str = "") -> None:
        
        #Load an SVG file for preview. If QSvgRenderer is unavailable or loading fails,
        #the widget falls back to the text placeholder.
        self.svg_path = None
        self.renderer = None
        if not path:
            self.filename = None
            self.update()
            return

        if QSvgRenderer is None:
            # QtSvg not available on this environment
            self.filename = Path(path).name
            self.update()
            return

        try:
            renderer = QSvgRenderer(path)
            if renderer.isValid():
                self.renderer = renderer
                self.svg_path = path
                self.filename = Path(path).name
            else:
                # invalid svg - fall back
                self.renderer = None
                self.svg_path = None
                self.filename = Path(path).name
        except Exception:
            self.renderer = None
            self.svg_path = None
            self.filename = Path(path).name
        self.update()

#this shows the graphics of the ui using the QPainter Class
    def paintEvent(self, event):
        painter = QPainter(self)
        rect = self.rect()

        # Background
        painter.fillRect(rect, self.bg_color)

        # Draw placeholder "canvas" box
        padding = 20
        box = rect.adjusted(padding, padding, -padding, -padding)
        painter.setPen(self.box_color)
        painter.drawRect(box)

        # If an SVG renderer is available and has loaded an SVG, render it scaled into the box.
        if self.renderer:
            painter.setRenderHint(QPainter.Antialiasing)
            # Render the SVG into the box while preserving the box area.
            # QSvgRenderer will scale the SVG to fit the target rectangle.
            try:
                self.renderer.render(painter, QRectF(box))
            except Exception:
                # Fall back to text if render fails
                painter.setPen(self.fg_color)
                font = QFont("Sans", 10)
                painter.setFont(font)
                msg = f"Failed to render SVG: {self.filename}"
                painter.drawText(box, Qt.AlignCenter, msg)
            return

        # Filename text (fallback)
        painter.setPen(self.fg_color)
        font = QFont("Sans", 10)
        painter.setFont(font)
        msg = self.filename or "No SVG loaded"
        painter.drawText(box.adjusted(8, 8, -8, -8), Qt.AlignTop | Qt.AlignLeft, msg)

        # If we had geometry we'd draw it here; placeholder shows instructions
        hint = "Preview placeholder — load an SVG to see it here."
        painter.setPen(QColor("#9aa3ad"))
        painter.drawText(box, Qt.AlignCenter, hint)


