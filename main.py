from window_handler import run_hotkey_handler

def main():
    print("Process Killer/Suspender is now running.")
    print("Use the following hotkeys:")
    print("Ctrl + Alt + F4: Kill focused window's process")
    print("Ctrl + Alt + F3: Suspend/Resume focused window's process")
    print("Press Ctrl+C to exit.")
    
    try:
        run_hotkey_handler()
    except KeyboardInterrupt:
        print("\nExiting...")

if __name__ == "__main__":
    main()
