import json
from pathlib import Path
from typing import Any, Optional, List

from PySide6.QtCore import QThread, QTimer
from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QPushButton,
    QLabel,
    QFileDialog,
    QMessageBox,
    QTextEdit,
    QGroupBox,
    QSpacerItem,
    QSizePolicy,
    QInputDialog,
)

from main import ProcessorWorker, PreviewCanvas
from streaming import GrblStreamer, StreamState, StreamError


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Signature Engravers Program v1")
        self.resize(1100, 700)

        # Top-level layout
        central = QWidget()
        main_layout = QHBoxLayout()
        central.setLayout(main_layout)
        self.setCentralWidget(central)

        # Left: preview canvas
        self.preview = PreviewCanvas()
        main_layout.addWidget(self.preview, stretch=3)

        # Right: controls & console
        right_col = QVBoxLayout()
        main_layout.addLayout(right_col, stretch=2)

        # File / selection group
        file_group = QGroupBox("File")
        fg_layout = QVBoxLayout()
        file_group.setLayout(fg_layout)
        self.lbl_file = QLabel("No file selected.")
        self.btn_open = QPushButton("Open SVG...")
        self.btn_open.clicked.connect(self.open_svg_dialog)

        # Save G-code button
        self.btn_save = QPushButton("Save G-code...")
        self.btn_save.setEnabled(False)
        self.btn_save.clicked.connect(self.save_gcode_dialog)

        # Stream button
        self.btn_stream = QPushButton("Start Streaming...")
        self.btn_stream.setEnabled(False)
        self.btn_stream.clicked.connect(self.start_streaming)

        fg_layout.addWidget(self.lbl_file)
        fg_layout.addWidget(self.btn_open)
        fg_layout.addWidget(self.btn_save)
        fg_layout.addWidget(self.btn_stream)
        right_col.addWidget(file_group)

        # Console / log
        console_group = QGroupBox("Console")
        c_layout = QVBoxLayout()
        console_group.setLayout(c_layout)
        self.console = QTextEdit()
        self.console.setReadOnly(True)
        c_layout.addWidget(self.console)
        right_col.addWidget(console_group, stretch=1)

        # Status bar
        self.status_label = QLabel("Idle")
        self.status_label.setMinimumHeight(24)
        self.statusBar().addPermanentWidget(self.status_label)

        # Spacer to push controls up
        right_col.addItem(QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding))

        # State
        self.current_svg: Optional[Path] = None
        self._worker: Optional[ProcessorWorker] = None
        self._thread: Optional[QThread] = None
        self._last_polylines: Optional[List[List[tuple]]] = None
        self._last_gcode: Optional[List[str]] = None
        self._streamer: Optional[GrblStreamer] = None

    # ---------------- Console helper ----------------
    def console_append(self, text: str) -> None:
        self.console.append(text)
        self.console.verticalScrollBar().setValue(self.console.verticalScrollBar().maximum())

    # ---------------- SVG / G-code flow ----------------
    def open_svg_dialog(self) -> None:
        fname, _ = QFileDialog.getOpenFileName(
            self, "Open SVG file", "", "SVG Files (*.svg);;All Files (*)"
        )
        if not fname:
            return

        path = Path(fname)

        # Validation
        if path.suffix.lower() != ".svg":
            QMessageBox.warning(self, "Invalid file", "Please select an .svg file.")
            return
        if not path.exists():
            QMessageBox.critical(self, "File missing", f"File not found:\n{fname}")
            return

        # Optional size guard
        max_mb = 100
        try:
            if path.stat().st_size > max_mb * 1024 * 1024:
                ok = QMessageBox.question(
                    self,
                    "Large file",
                    f"The selected file is > {max_mb} MB. Continue?",
                    QMessageBox.Yes | QMessageBox.No,
                )
                if ok != QMessageBox.Yes:
                    return
        except Exception:
            pass

        # Update UI, preview, and start processing
        self.current_svg = path
        self.lbl_file.setText(str(path))
        try:
            self.preview.load_svg(str(path))
        except Exception:
            self.preview.set_filename(path.name)
        self.console_append(f"Selected SVG: {path}")

        self.run_processor_worker(str(path))

    def run_processor_worker(self, svg_path: str, resolution: float = 0.5) -> None:
        # Disable open while processing
        self.btn_open.setEnabled(False)
        self.btn_save.setEnabled(False)
        self.btn_stream.setEnabled(False)

        self.status_label.setText("Processing SVG...")
        self.console_append("Starting SVG -> G-code processing...")

        self._worker = ProcessorWorker(svg_path, resolution=resolution)
        self._thread = QThread()
        self._worker.moveToThread(self._thread)

        self._thread.started.connect(self._worker.run)
        self._worker.started.connect(lambda: self.console_append("Processor started"))
        self._worker.finished.connect(self.on_processing_finished)
        self._worker.error.connect(self.on_processing_error)

        # Clean up thread/worker when done
        self._worker.finished.connect(self._thread.quit)
        self._worker.error.connect(self._thread.quit)
        self._thread.finished.connect(self._thread.deleteLater)

        self._thread.start()

    def on_processing_finished(self, result: Any) -> None:
        self.btn_open.setEnabled(True)
        self.status_label.setText("SVG processed")
        try:
            polylines = result.get("polylines", None) if isinstance(result, dict) else None
            gcode = result.get("gcode", None) if isinstance(result, dict) else None
        except Exception:
            polylines = None
            gcode = result

        self._last_polylines = polylines
        self._last_gcode = gcode if isinstance(gcode, list) else None

        if polylines is not None:
            self.console_append(f"Parsed {len(polylines)} polylines.")
            try:
                self.console_append(json.dumps(polylines[:5], indent=2))
            except Exception:
                self.console_append(str(polylines))
        else:
            self.console_append("Polylines not available from svg_parser.")

        if self._last_gcode:
            self.console_append("Generated G-code:")
            for ln in self._last_gcode:
                self.console_append(ln)
            self.btn_save.setEnabled(True)
            self.btn_stream.setEnabled(True)
            QMessageBox.information(self, "Processing finished", "SVG parsed and G-code generated. See console.")
        else:
            self.console_append("No G-code generated.")
            self.btn_save.setEnabled(False)
            self.btn_stream.setEnabled(False)
            QMessageBox.information(self, "Processing finished", "SVG parsed but no G-code produced.")

    def on_processing_error(self, message: str) -> None:
        self.btn_open.setEnabled(True)
        self.status_label.setText("Processing failed")
        self.console_append(f"Processor error: {message}")
        QMessageBox.critical(self, "Processing error", message)
        self._last_gcode = None
        self.btn_save.setEnabled(False)
        self.btn_stream.setEnabled(False)

    def save_gcode_dialog(self) -> None:
        if not self._last_gcode:
            QMessageBox.information(self, "No G-code", "No G-code available to save.")
            return
        suggested = "output.gcode"
        fname, _ = QFileDialog.getSaveFileName(
            self,
            "Save G-code",
            suggested,
            "G-code Files (*.gcode);;All Files (*)",
        )
        if not fname:
            return
        try:
            with open(fname, "w", encoding="utf-8") as fh:
                for ln in self._last_gcode:
                    fh.write(ln + "\n")
            QMessageBox.information(self, "Saved", f"G-code written to: {fname}")
            self.console_append(f"G-code saved to {fname}")
        except Exception as e:
            QMessageBox.critical(self, "Save error", f"Could not save file: {e}")
            self.console_append(f"Failed to save G-code: {e}")

    # ---------------- Streaming integration ----------------
    def start_streaming(self) -> None:
        """
        Start streaming the generated G-code to a grblHAL-compatible controller.
        Uses a preamble + last generated G-code and only reacts to job-level events.
        """
        if not self._last_gcode:
            QMessageBox.information(self, "No G-code", "Generate G-code before starting streaming.")
            return

        # Preamble + job G-code
        preamble = ["$X", "G21", "G90"]
        lines = preamble + self._last_gcode

        # Port (default COM11)
        port, ok = QInputDialog.getText(
            self,
            "Serial Port",
            "Enter COM port (e.g. COM11):",
            text="COM11",
        )
        if not ok or not port.strip():
            return
        port = port.strip()

        # Baudrate (default 115200)
        baud_str, ok = QInputDialog.getText(
            self,
            "Baudrate",
            "Enter baudrate:",
            text="115200",
        )
        if not ok or not baud_str.strip():
            return
        try:
            baudrate = int(baud_str.strip())
        except ValueError:
            QMessageBox.warning(self, "Invalid baudrate", "Baudrate must be an integer.")
            return

        # Lock UI during job
        self.btn_stream.setEnabled(False)
        self.btn_open.setEnabled(False)
        self.btn_save.setEnabled(False)
        self.status_label.setText(f"Connecting to {port}...")
        self.console_append(f"Starting streaming on {port} @ {baudrate}...")
        self.console_append("Preamble: " + ", ".join(preamble))

        # Thread-safe wrappers
        def state_cb(state: StreamState) -> None:
            QTimer.singleShot(0, lambda: self._on_stream_state(state))

        def error_cb(err: StreamError) -> None:
            QTimer.singleShot(0, lambda: self._on_stream_error(err))

        self._streamer = GrblStreamer(
            port=port,
            baudrate=baudrate,
            lines=lines,
            state_callback=state_cb,
            error_callback=error_cb,
            startup_drain_time=2.0,
        )
        self._streamer.start()

    def _on_stream_state(self, state: StreamState) -> None:
        if state == StreamState.SENDING:
            self.status_label.setText("Streaming in progress…")
        elif state == StreamState.DONE:
            self.status_label.setText("Streaming complete")
            self.console_append("G-code streaming completed successfully.")
            QMessageBox.information(
                self,
                "Streaming finished",
                "G-code was sent successfully to the controller.",
            )
            self.btn_stream.setEnabled(True)
            self.btn_open.setEnabled(True)
            self.btn_save.setEnabled(bool(self._last_gcode))
        elif state == StreamState.ERROR:
            self.status_label.setText("Streaming failed")
        elif state == StreamState.IDLE:
            self.status_label.setText("Idle")
            self.btn_stream.setEnabled(True)
            self.btn_open.setEnabled(True)
            self.btn_save.setEnabled(bool(self._last_gcode))

    def _on_stream_error(self, err: StreamError) -> None:
        if err.line_index >= 0:
            msg = (
                f"Streaming failed.\n\n"
                f"Line {err.line_index + 1}:\n"
                f"{err.line_text}\n\n"
                f"Controller response: {err.raw_line}"
            )
        else:
            msg = f"Streaming failed.\n\n{err.raw_line}"
        self.console_append("Streaming failed.")
        QMessageBox.critical(self, "Streaming error", msg)
        self.btn_stream.setEnabled(True)
        self.btn_open.setEnabled(True)
        self.btn_save.setEnabled(bool(self._last_gcode))


if __name__ == "__main__":
    import sys

    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec())
