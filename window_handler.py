import win32gui
import win32process
from pynput import keyboard
import process_killer
import logging

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

def get_focused_window_pid():
    hwnd = win32gui.GetForegroundWindow()
    _, pid = win32process.GetWindowThreadProcessId(hwnd)
    return pid

from pynput import keyboard
import process_killer
import logging

def get_focused_window_pid():
    hwnd = win32gui.GetForegroundWindow()
    _, pid = win32process.GetWindowThreadProcessId(hwnd)
    return pid

class HotkeyHandler:
    def __init__(self):
        self.suspended_pids = set()
        self.listener = None
        self.kill_key = None
        self.suspend_key = None
        self.ctrl_pressed = False
        self.alt_pressed = False

    def set_hotkeys(self, kill_hotkey, suspend_hotkey):
        self.kill_key = kill_hotkey.lower()
        self.suspend_key = suspend_hotkey.lower()
        logging.info(f"Hotkeys set - Kill: Ctrl+Alt+{self.kill_key}, Suspend/Resume: Ctrl+Alt+{self.suspend_key}")

    def on_press(self, key):
        try:
            key_char = key.char.lower()
        except AttributeError:
            key_char = key

        if key == keyboard.Key.ctrl:
            self.ctrl_pressed = True
        elif key == keyboard.Key.alt:
            self.alt_pressed = True
        elif self.ctrl_pressed and self.alt_pressed:
            if key_char == self.kill_key:
                self.on_kill_hotkey()
            elif key_char == self.suspend_key:
                self.on_suspend_hotkey()

    def on_release(self, key):
        if key == keyboard.Key.ctrl:
            self.ctrl_pressed = False
        elif key == keyboard.Key.alt:
            self.alt_pressed = False

    def on_kill_hotkey(self):
        pid = get_focused_window_pid()
        logging.info(f"Kill hotkey pressed. Attempting to kill process with PID: {pid}")
        process_killer.force_kill_process(pid)

    def on_suspend_hotkey(self):
        pid = get_focused_window_pid()
        logging.info(f"Suspend/Resume hotkey pressed. Focused window PID: {pid}")
        if pid in self.suspended_pids:
            logging.info(f"Attempting to resume process with PID: {pid}")
            process_killer.resume_process(pid)
            self.suspended_pids.remove(pid)
            logging.info(f"Process with PID {pid} resumed. Removed from suspended_pids.")
        else:
            logging.info(f"Attempting to suspend process with PID: {pid}")
            process_killer.suspend_process(pid)
            self.suspended_pids.add(pid)
            logging.info(f"Process with PID {pid} suspended. Added to suspended_pids.")
        logging.debug(f"Current suspended PIDs: {self.suspended_pids}")

    def start(self):
        if self.kill_key and self.suspend_key:
            logging.info("Starting hotkey listener...")
            self.listener = keyboard.Listener(on_press=self.on_press, on_release=self.on_release)
            self.listener.start()
        else:
            logging.warning("No hotkeys set. Use set_hotkeys() before starting.")

    def stop(self):
        if self.listener:
            logging.info("Stopping hotkey listener...")
            self.listener.stop()
