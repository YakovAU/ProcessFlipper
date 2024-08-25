from window_handler import run_hotkey_handler
import logging

def main():
    print("Process Killer/Suspender is now running.")
    print("Use the following hotkeys:")
    print("Ctrl + Alt + F4: Kill focused window's process")
    print("Ctrl + Alt + F3: Suspend/Resume focused window's process")
    print("Press Ctrl+C to exit.")
    print("Check the log file for detailed information.")
    
    # Configure file logging
    logging.basicConfig(filename='process_killer.log', level=logging.DEBUG, 
                        format='%(asctime)s - %(levelname)s - %(message)s')
    
    # Add a stream handler to also log to console
    console = logging.StreamHandler()
    console.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    console.setFormatter(formatter)
    logging.getLogger('').addHandler(console)
    
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
