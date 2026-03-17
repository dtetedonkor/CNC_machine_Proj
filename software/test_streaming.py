import unittest
from unittest.mock import patch

from streaming import GrblStreamer, StreamState


class _FakeSerial:
    def __init__(self, responses):
        self._responses = list(responses)
        self.writes = []

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        return False

    def reset_input_buffer(self):
        return None

    def write(self, payload):
        self.writes.append(payload)

    def readline(self):
        if self._responses:
            return self._responses.pop(0)
        return b""


class StreamingTests(unittest.TestCase):
    def test_streamer_sends_lines_and_finishes(self):
        states = []
        fake = _FakeSerial([b"ok\n", b"ok\n"])

        with patch("streaming.serial.Serial", return_value=fake):
            streamer = GrblStreamer(
                port="COM11",
                baudrate=115200,
                lines=[";comment", "G21", "G90"],
                state_callback=states.append,
                startup_drain_time=0.0,
            )
            streamer.run()

        self.assertEqual(fake.writes, [b"G21\n", b"G90\n"])
        self.assertEqual(states, [StreamState.SENDING, StreamState.DONE])

    def test_streamer_reports_controller_error(self):
        states = []
        errors = []
        fake = _FakeSerial([b"error:2\n"])

        with patch("streaming.serial.Serial", return_value=fake):
            streamer = GrblStreamer(
                port="COM11",
                baudrate=115200,
                lines=["G1 X1"],
                state_callback=states.append,
                error_callback=errors.append,
                startup_drain_time=0.0,
            )
            streamer.run()

        self.assertEqual(states, [StreamState.SENDING, StreamState.ERROR])
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].line_text, "G1 X1")
        self.assertEqual(errors[0].raw_line, "error:2")

    def test_streamer_reports_timeout(self):
        states = []
        errors = []
        fake = _FakeSerial([])

        with patch("streaming.serial.Serial", return_value=fake):
            streamer = GrblStreamer(
                port="COM11",
                baudrate=115200,
                lines=["G1 X1"],
                state_callback=states.append,
                error_callback=errors.append,
                startup_drain_time=0.0,
                timeout_per_line=0.01,
            )
            streamer.run()

        self.assertEqual(states, [StreamState.SENDING, StreamState.ERROR])
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].line_text, "G1 X1")
        self.assertIn("Timeout waiting for OK", errors[0].raw_line)

    def test_streamer_reports_encoding_error(self):
        states = []
        errors = []
        fake = _FakeSerial([])

        with patch("streaming.serial.Serial", return_value=fake):
            streamer = GrblStreamer(
                port="COM11",
                baudrate=115200,
                lines=["G1 X1 Ā"],
                state_callback=states.append,
                error_callback=errors.append,
                startup_drain_time=0.0,
            )
            streamer.run()

        self.assertEqual(states, [StreamState.SENDING, StreamState.ERROR])
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].line_text, "G1 X1 Ā")
        self.assertIn("Encoding error", errors[0].raw_line)


if __name__ == "__main__":
    unittest.main()
