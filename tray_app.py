import pystray
from PIL import Image
import threading
from window_handler import HotkeyHandler
import logging
import tkinter as tk
from tkinter import messagebox

KILL_HOTKEY = '<ctrl>+<alt>+f4'
SUSPEND_HOTKEY = '<ctrl>+<alt>+f3'

class TrayIcon:
    def __init__(self):
        self.icon = None
        self.hotkey_handler = HotkeyHandler()
        self.hotkey_thread = None

    def create_image(self):
        return Image.open('icon.png')

    def about(self):
        logging.info("Showing About information")
        root = tk.Tk()
        root.withdraw()
        messagebox.showinfo("About Process Killer/Suspender",
                            "Process Killer/Suspender\n"
                            "Version 1.0\n\n"
                            f"Kill Hotkey: Ctrl+Alt+F4\n"
                            f"Suspend/Resume Hotkey: Ctrl+Alt+F3\n\n"
                            "Created by Yakov")

    def exit_app(self):
        logging.info("Exiting application")
        self.icon.stop()
        if self.hotkey_thread:
            self.hotkey_handler.stop()
            self.hotkey_thread.join()

    def setup(self):
        self.icon = pystray.Icon("ProcessKillerSuspender", self.create_image(), "Process Killer/Suspender")
        self.icon.menu = pystray.Menu(
            pystray.MenuItem("About", self.about),
            pystray.MenuItem("Exit", self.exit_app)
        )
        self.hotkey_handler.set_hotkeys(KILL_HOTKEY, SUSPEND_HOTKEY)

    def run(self):
        self.setup()
        self.hotkey_thread = threading.Thread(target=self.hotkey_handler.start)
        self.hotkey_thread.start()
        self.icon.run()

def run_tray_app():
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
    tray_app = TrayIcon()
    tray_app.run()

if __name__ == "__main__":
    run_tray_app()
