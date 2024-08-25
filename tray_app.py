import pystray
from PIL import Image
import threading
from window_handler import HotkeyHandler
import logging
import tkinter as tk
from tkinter import simpledialog

class TrayIcon:
    def __init__(self):
        self.icon = None
        self.hotkey_handler = HotkeyHandler()
        self.hotkey_thread = None

    def create_image(self):
        # Replace 'icon.png' with the path to your icon image
        return Image.open('icon.png')

    def set_hotkeys(self):
        root = tk.Tk()
        root.withdraw()
        
        kill_hotkey = simpledialog.askstring("Set Hotkeys", "Enter kill hotkey (e.g., '<ctrl>+<alt>+f4'):")
        suspend_hotkey = simpledialog.askstring("Set Hotkeys", "Enter suspend/resume hotkey (e.g., '<ctrl>+<alt>+f3'):")
        
        if kill_hotkey and suspend_hotkey:
            self.hotkey_handler.set_hotkeys(kill_hotkey, suspend_hotkey)
            logging.info(f"Hotkeys set - Kill: {kill_hotkey}, Suspend/Resume: {suspend_hotkey}")

    def about(self):
        logging.info("Showing About information")
        # You can replace this with a proper dialog if needed
        print("Process Killer/Suspender")
        print("Version 1.0")
        print("Use the tray icon to set hotkeys and control the application.")

    def exit_app(self):
        logging.info("Exiting application")
        self.icon.stop()
        if self.hotkey_thread:
            self.hotkey_handler.stop()
            self.hotkey_thread.join()

    def setup(self):
        self.icon = pystray.Icon("ProcessKillerSuspender", self.create_image(), "Process Killer/Suspender")
        self.icon.menu = pystray.Menu(
            pystray.MenuItem("Set Hotkeys", self.set_hotkeys),
            pystray.MenuItem("About", self.about),
            pystray.MenuItem("Exit", self.exit_app)
        )

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
