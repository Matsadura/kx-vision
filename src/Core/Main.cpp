#include <cstdio> // Required for fclose
#include <windows.h>

#include "AddressManager.h"
#include "AppState.h"   // Include for AppState singleton
#include "Console.h"
#include "Hooks.h"
#include "../Rendering/ImGuiManager.h"
#include "../Utils/DebugLogger.h" // Include for logger initialization

HINSTANCE dll_handle;
static HANDLE g_hSingleInstanceMutex = NULL;

// Eject thread to free the DLL
DWORD WINAPI EjectThread(LPVOID lpParameter) {
#ifdef _DEBUG
    // In Debug builds, close standard streams and free the console
    if (stdout) fclose(stdout);
    if (stderr) fclose(stderr);
    if (stdin) fclose(stdin);

    if (!FreeConsole()) {
        OutputDebugStringA("kx-vision: FreeConsole() failed.\n");
    }
#endif // _DEBUG
    Sleep(100);
    FreeLibraryAndExitThread(dll_handle, 0);
    return 0;
}

// Main function that runs in a separate thread
DWORD WINAPI MainThread(LPVOID lpParameter) {
    // Initialize debug logger first for early logging capability
    LOG_INIT();
    
#ifdef _DEBUG
    kx::SetupConsole(); // Only setup console in Debug builds
#endif // _DEBUG

    LOG_INFO("KX Vision starting up...");
    
    // Initialize hooks first (this sets up D3D hook which creates ImGuiManager)
    if (!kx::InitializeHooks()) {
        LOG_ERROR("Failed to initialize hooks.");
        return 1;
    }
    
    LOG_INFO("KX Vision hooks initialized successfully");
    
    // Now wait for player to be in-game before initializing AddressManager
    bool is_address_manager_initialized = false;
    LOG_INFO("Waiting for player to enter a map before initializing AddressManager...");

    // Main loop - handles deferred AddressManager initialization and runtime operation
    while (kx::AppState::Get().IsVisionWindowOpen() && !(GetAsyncKeyState(VK_DELETE) & 0x8000)) {
        
        // Deferred AddressManager initialization: Wait until player is actually in-game
        if (!is_address_manager_initialized) {
            // Check if ImGuiManager has been initialized by the Present hook
            if (ImGuiManager::IsImGuiInitialized()) {
                // Now we can safely access the MumbleLinkManager
                kx::MumbleLinkManager& mumbleLinkManager = ImGuiManager::GetMumbleLinkManager();
                const kx::MumbleLinkData* data = mumbleLinkManager.GetData();
                
                // Check if MumbleLink is connected and player is in a map (mapId != 0)
                if (data && mumbleLinkManager.IsInitialized() && data->context.mapId != 0) {
                    LOG_INFO("[Main] Player is in-map (Map ID: %u). Initializing AddressManager...", 
                        data->context.mapId);
                    
                    // Initialize AddressManager now that we're safely in-game
                    kx::AddressManager::Initialize();
                    
                    // Now initialize the game thread hook (requires AddressManager)
                    if (kx::InitializeGameThreadHook()) {
                        LOG_INFO("Game thread hook initialized successfully");
                    } else {
                        LOG_WARN("Game thread hook initialization failed - ESP may not work");
                    }
                    
                    is_address_manager_initialized = true;
                    LOG_INFO("AddressManager initialized successfully");
                } else {
                    // Not in-game yet, wait a bit before checking again
                    Sleep(500);
                    continue;
                }
            } else {
                // ImGui not initialized yet (waiting for first Present call), wait a bit
                Sleep(500);
                continue;
            }
        }
        
        // Normal operation once initialized
        Sleep(100); // Sleep to avoid busy-waiting
    }

    // Signal hooks to stop processing before actual cleanup
    kx::AppState::Get().SetShuttingDown(true);

    // Give hooks a moment to recognize the flag before cleanup starts
	// This helps prevent calls into ImGui after it's destroyed.
    Sleep(250);

    // Cleanup hooks and ImGui
    kx::CleanupHooks();

    LOG_INFO("KX Vision shutting down...");
    
    // Cleanup logger (close log file)
    LOG_CLEANUP();

    // Eject the DLL and exit the thread
    CreateThread(0, 0, EjectThread, 0, 0, 0);

    return 0;
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Create a uniquely named mutex.
        // Using a GUID is a good way to ensure the name is unique.
        const wchar_t* mutexName = L"kx-vision-instance-mutex-9A8B7C6D";

        g_hSingleInstanceMutex = CreateMutexW(NULL, TRUE, mutexName);

        // Check if the mutex already exists.
        if (g_hSingleInstanceMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
            // Another instance is already running. Close the handle we just tried to create
            // and signal the loader to NOT load this DLL.
            if (g_hSingleInstanceMutex) {
                CloseHandle(g_hSingleInstanceMutex);
            }
            return FALSE; // Prevents the DLL from being loaded.
        }

        // If we're here, we are the first and only instance. Proceed with initialization.
        dll_handle = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
    }
    break;

    case DLL_PROCESS_DETACH:
    {
        // It's good practice to release and close the mutex on shutdown.
        // This ensures it's cleaned up properly if the DLL is ever unloaded
        // and then reloaded without the process restarting.
        if (g_hSingleInstanceMutex) {
            ReleaseMutex(g_hSingleInstanceMutex);
            CloseHandle(g_hSingleInstanceMutex);
        }
    }
    break;
    }
    return TRUE;
}
