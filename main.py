from window_handler import run_hotkey_handler
import logging
import sys

def main():
    print("Process Killer/Suspender is now running.")
    print("Use the following hotkeys:")
    print("Ctrl + Alt + F4: Kill focused window's process")
    print("Ctrl + Alt + F3: Suspend/Resume focused window's process")
    print("Press Ctrl+C to exit.")
    print("Check the log file for detailed information.")
    
    # Configure root logger
    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)
    
    # Configure file handler
    file_handler = logging.FileHandler('process_killer.log')
    file_handler.setLevel(logging.DEBUG)
    file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    file_handler.setFormatter(file_formatter)
    root_logger.addHandler(file_handler)
    
    # Configure console handler
    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setLevel(logging.INFO)
    console_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    console_handler.setFormatter(console_formatter)
    root_logger.addHandler(console_handler)
    
    try:
        run_hotkey_handler()
    except KeyboardInterrupt:
        logging.info("KeyboardInterrupt received. Exiting...")
    except Exception as e:
        logging.error(f"An unexpected error occurred: {e}", exc_info=True)
    finally:
        print("\nExiting...")

if __name__ == "__main__":
    main()
