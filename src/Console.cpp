#include "Console.h"
#include <windows.h>
#include <iostream>
#include <cstdio>

namespace kx {
void SetupConsole() {
    AllocConsole();  // Allocates a new console for the calling process

    // Redirect the standard input/output/error streams to the console
    FILE* file;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONOUT$", "w", stderr);
    freopen_s(&file, "CONIN$", "r", stdin);

    // Optional: Disable the close button of the console to prevent accidental closure
    HWND consoleWindow = GetConsoleWindow();
    HMENU hMenu = GetSystemMenu(consoleWindow, FALSE);
    if (hMenu) {
        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    }

    std::cout << "Console initialized!" << std::endl;
}
}
