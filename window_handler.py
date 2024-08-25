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

class HotkeyHandler:
    def __init__(self):
        self.suspended_pids = set()
        self.listener = keyboard.Listener(on_press=self.on_press)

    def on_press(self, key):
        try:
            logging.debug(f"Key pressed: {key}")
            logging.debug(f"Current keys: {self.current_keys}")
            
            if key == keyboard.Key.f4 and keyboard.Key.ctrl in self.current_keys and keyboard.Key.alt in self.current_keys:
                pid = get_focused_window_pid()
                logging.info(f"Attempting to kill process with PID: {pid}")
                process_killer.force_kill_process(pid)
            elif key == keyboard.Key.f3 and keyboard.Key.ctrl in self.current_keys and keyboard.Key.alt in self.current_keys:
                pid = get_focused_window_pid()
                logging.info(f"Ctrl + Alt + F3 pressed. Focused window PID: {pid}")
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
        except Exception as e:
            logging.error(f"Error in on_press: {e}", exc_info=True)

    def start(self):
        with keyboard.Events() as events:
            self.current_keys = set()
            for event in events:
                if isinstance(event, keyboard.Events.Press):
                    self.current_keys.add(event.key)
                    self.on_press(event.key)
                elif isinstance(event, keyboard.Events.Release):
                    self.current_keys.discard(event.key)

def run_hotkey_handler():
    handler = HotkeyHandler()
    handler.start()
