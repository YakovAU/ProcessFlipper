import win32gui
import win32process
from pynput import keyboard
import process_killer
import logging
from critical_processes import CRITICAL_PROCESSES

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

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
        self.kill_key = kill_hotkey
        self.suspend_key = suspend_hotkey
        logging.info(f"Hotkeys set - Kill: Ctrl+Alt+{self.kill_key}, Suspend/Resume: Ctrl+Alt+{self.suspend_key}")

    def on_press(self, key):
        if key in (keyboard.Key.ctrl_l, keyboard.Key.ctrl_r):
            self.ctrl_pressed = True
        elif key in (keyboard.Key.alt_l, keyboard.Key.alt_r):
            self.alt_pressed = True
        elif self.ctrl_pressed and self.alt_pressed:
            if key == self.kill_key:
                self.on_kill_hotkey()
            elif key == self.suspend_key:
                self.on_suspend_hotkey()

    def on_release(self, key):
        # Only reset the flag for the specific key that was released
        if key in (keyboard.Key.ctrl_l, keyboard.Key.ctrl_r):
            self.ctrl_pressed = False
        elif key in (keyboard.Key.alt_l, keyboard.Key.alt_r):
            self.alt_pressed = False

    def on_kill_hotkey(self):
        pid = get_focused_window_pid()
        logging.info(f"Kill hotkey pressed. Attempting to kill process with PID: {pid}")
        process_name = process_killer.get_process_name(pid)
        if process_name.lower() in (p.lower() for p in CRITICAL_PROCESSES):
            logging.warning(f"Attempted to kill critical process {process_name} (PID {pid}). Operation aborted.")
            return
        process_killer.force_kill_process(pid)

    def on_suspend_hotkey(self):
        pid = get_focused_window_pid()
        logging.info(f"Suspend/Resume hotkey pressed. Focused window PID: {pid}")
        process_name = process_killer.get_process_name(pid)
        if process_name.lower() in (p.lower() for p in CRITICAL_PROCESSES):
            logging.warning(f"Attempted to suspend critical process {process_name} (PID {pid}). Operation aborted.")
            return
        if pid in self.suspended_pids:
            logging.info(f"Attempting to resume process with PID: {pid}")
            process_killer.resume_process(pid)
            self.suspended_pids.remove(pid)
            logging.info(f"Process {process_name} with PID {pid} resumed. Removed from suspended_pids.")
        else:
            logging.info(f"Attempting to suspend process with PID: {pid}")
            process_killer.suspend_process(pid)
            self.suspended_pids.add(pid)
            logging.info(f"Process {process_name} with PID {pid} suspended. Added to suspended_pids.")
        logging.debug(f"Current suspended PIDs: {self.suspended_pids}")

    def start(self):
        self.set_hotkeys(keyboard.Key.f3, keyboard.Key.f4)
        logging.info("Starting hotkey listener...")
        self.listener = keyboard.Listener(on_press=self.on_press, on_release=self.on_release)
        self.listener.start()

    def stop(self):
        if self.listener:
            logging.info("Stopping hotkey listener...")
            self.listener.stop()