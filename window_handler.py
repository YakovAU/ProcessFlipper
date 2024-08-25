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
        self.kill_hotkey = None
        self.suspend_hotkey = None
        self.listener = None

    def set_hotkeys(self, kill_hotkey, suspend_hotkey):
        self.kill_hotkey = keyboard.HotKey.parse(kill_hotkey)
        self.suspend_hotkey = keyboard.HotKey.parse(suspend_hotkey)
        
        if self.listener:
            self.stop()
        
        self.listener = keyboard.GlobalHotKeys({
            kill_hotkey: self.on_kill_hotkey,
            suspend_hotkey: self.on_suspend_hotkey
        })

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
        if self.listener:
            logging.info("Starting hotkey listener...")
            self.listener.start()
        else:
            logging.warning("No hotkeys set. Use set_hotkeys() before starting.")

    def stop(self):
        if self.listener:
            logging.info("Stopping hotkey listener...")
            self.listener.stop()
