import process_killer

def main():
    pid = int(input("Enter the PID of the process: "))
    action = input("Do you want to kill or suspend the process? (k/s): ").lower()

    try:
        if action == 'k':
            process_killer.force_kill_process(pid)
        elif action == 's':
            process_killer.suspend_process(pid)
            print("Process suspended. Resume or kill? (r/k): ")
            resume_action = input().lower()
            if resume_action == 'r':
                process_killer.resume_process(pid)
            elif resume_action == 'k':
                process_killer.force_kill_process(pid)
            else:
                print("Invalid input. Leaving process suspended.")
        else:
            print("Invalid action. Please enter 'k' for kill or 's' for suspend.")
    except OSError as e:
        print(f"Failed to perform action: {e}")
        print("Note: This tool may require administrator privileges to work effectively.")

if __name__ == "__main__":
    main()
