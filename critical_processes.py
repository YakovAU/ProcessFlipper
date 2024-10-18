# List of critical Windows processes that should not be suspended or killed
CRITICAL_PROCESSES = [
    "System",
    "smss.exe",
    "csrss.exe",
    "wininit.exe",
    "services.exe",
    "lsass.exe",
    "winlogon.exe",
    "explorer.exe",
    "svchost.exe",
    "taskmgr.exe",
    "dwm.exe",
    "conhost.exe",
    "rundll32.exe",
]
"""import unittest
from unittest.mock import patch, MagicMock
from window_handler import HotkeyHandler

class TestCriticalProcesses(unittest.TestCase):
    def setUp(self):
        self.hotkey_handler = HotkeyHandler()

    @patch('window_handler.get_focused_window_pid')
    @patch('process_killer.get_process_name')
    def test_kill_critical_process(self, mock_get_process_name, mock_get_focused_window_pid):
        mock_get_focused_window_pid.return_value = 123
        mock_get_process_name.return_value = 'System'
        
        with self.assertLogs('window_handler', level='WARNING') as cm:
            self.hotkey_handler.on_kill_hotkey()
            self.assertIn('Attempted to kill critical process System (PID 123). Operation aborted.', cm.output[0])

    @patch('window_handler.get_focused_window_pid')
    @patch('process_killer.get_process_name')
    def test_suspend_critical_process(self, mock_get_process_name, mock_get_focused_window_pid):
        mock_get_focused_window_pid.return_value = 456
        mock_get_process_name.return_value = 'explorer.exe'
        
        with self.assertLogs('window_handler', level='WARNING') as cm:
            self.hotkey_handler.on_suspend_hotkey()
            self.assertIn('Attempted to suspend critical process explorer.exe (PID 456). Operation aborted.', cm.output[0])

    @patch('window_handler.get_focused_window_pid')
    @patch('process_killer.get_process_name')
    @patch('process_killer.force_kill_process')
    def test_kill_non_critical_process(self, mock_force_kill_process, mock_get_process_name, mock_get_focused_window_pid):
        mock_get_focused_window_pid.return_value = 789
        mock_get_process_name.return_value = 'non_critical_process.exe'
        
        self.hotkey_handler.on_kill_hotkey()
        mock_force_kill_process.assert_called_once_with(789)

    @patch('window_handler.get_focused_window_pid')
    @patch('process_killer.get_process_name')
    @patch('process_killer.suspend_process')
    def test_suspend_non_critical_process(self, mock_suspend_process, mock_get_process_name, mock_get_focused_window_pid):
        mock_get_focused_window_pid.return_value = 101
        mock_get_process_name.return_value = 'non_critical_process.exe'
        
        self.hotkey_handler.on_suspend_hotkey()
        mock_suspend_process.assert_called_once_with(101)

if __name__ == '__main__':
    unittest.main()"""
