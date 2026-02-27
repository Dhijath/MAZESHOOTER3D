#pragma once

#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HWND GameWindow_Create(HINSTANCE hInstance);

extern bool g_ExitDialogJustClosed;



#endif