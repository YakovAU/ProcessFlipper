#pragma once
#include <windows.h>
#include <unordered_map>
#include <functional>
#include <iostream> // For logging

class HotkeyManager {
public:
    HotkeyManager() : nextHotkeyId(1) {
        std::cout << "HotkeyManager initialized." << std::endl;
    }
    
    ~HotkeyManager() {
        // Clean up all registered hotkeys
        for (const auto& pair : hotkeyCallbacks) {
            UnregisterHotKey(nullptr, pair.first);
            std::cout << "Unregistered hotkey ID: " << pair.first << std::endl;
        }
        hotkeyCallbacks.clear();
    }

    int registerHotkey(const UINT modifiers, const UINT vk) {
        const int currentIdAttempt = nextHotkeyId; // Use a temporary variable for logging before incrementing
        if (RegisterHotKey(nullptr, currentIdAttempt, modifiers, vk)) {
            nextHotkeyId++; // Increment only on success
            std::cout << "Hotkey registered successfully: ID=" << currentIdAttempt
                      << ", Modifiers=0x" << std::hex << modifiers
                      << ", VK=0x" << std::hex << vk << std::dec << std::endl;
            return currentIdAttempt;
        }
        DWORD errorCode = GetLastError();
        std::cerr << "Failed to register hotkey: Modifiers=0x" << std::hex << modifiers
                  << ", VK=0x" << std::hex << vk << std::dec
                  << ". Error code: " << errorCode << std::endl;
        return -1;
    }

    void setCallback(int id, const std::function<void()>& callback) {
        if (id != -1) {
            hotkeyCallbacks[id] = callback;
            std::cout << "Callback set for hotkey ID: " << id << std::endl;
        } else {
            std::cerr << "Attempted to set callback for invalid hotkey ID (-1)." << std::endl;
        }
    }

    // Processes a given MSG struct. Returns true if it was a hotkey message and handled, false otherwise.
    bool processMessage(const MSG& msg) { // Go back to const reference but handle carefully
        if (msg.message == WM_HOTKEY) {
            // Extract only what we need - the ID
            int hotkeyId = static_cast<int>(msg.wParam);
            
            std::cout << "HotkeyManager::processMessage - WM_HOTKEY received. ID: " << hotkeyId << std::endl;
            
            // Find the callback
            auto it = hotkeyCallbacks.find(hotkeyId);
            if (it != hotkeyCallbacks.end() && it->second) {
                std::cout << "HotkeyManager::processMessage - Executing callback for hotkey ID: " << hotkeyId << std::endl;
                
                // Execute the callback directly - no need for a local copy
                it->second();
            } else {
                std::cerr << "HotkeyManager::processMessage - No callback found for hotkey ID: " << hotkeyId << std::endl;
            }
            
            return true; // Hotkey message was processed
        }
        
        return false; // Not a hotkey message
    }

    /*
    // The handleHotkeys method with PeekMessage is not suitable when GetMessage is used in the main loop
    // as GetMessage will consume the WM_HOTKEY before PeekMessage in this function can see it.
    // This function would be more appropriate if HotkeyManager ran its own message loop
    // or if PeekMessage was used exclusively in the main loop.
    void handleHotkeys() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY) {
                int hotkeyId = static_cast<int>(msg.wParam);
                std::cout << "WM_HOTKEY received. ID: " << hotkeyId << std::endl;
                if (hotkeyCallbacks.count(hotkeyId) > 0) {
                    std::cout << "Executing callback for hotkey ID: " << hotkeyId << std::endl;
                    hotkeyCallbacks[hotkeyId]();
                } else {
                    std::cerr << "No callback found for hotkey ID: " << hotkeyId << std::endl;
                }
            }
        }
    }
    */

private:
    int nextHotkeyId;
    std::unordered_map<int, std::function<void()>> hotkeyCallbacks;
};