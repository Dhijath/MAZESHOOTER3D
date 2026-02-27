/*==============================================================================

   ゲームウィンドウ生成とWndProc [game_window.cpp]
                                                         Author : 51106
                                                         Date   : 2025/12/19
--------------------------------------------------------------------------------

   ・GameWindow_Create でウィンドウ生成
   ・WndProc で Keyboard / Mouse にメッセージを転送
   ・RawInput（WM_INPUT）を取りこぼさないよう WndProc 冒頭で常に転送

==============================================================================*/

#include "game_window.h"
#include <algorithm>
#include <windows.h>

#include "keyboard.h"
#include "mouse.h"



/* ウィンドウプロシージャ プロトタイプ宣言 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* ウィンドウ情報 */
static constexpr char WINDOW_CLASS[] = "GameWindow";
static constexpr char TITLE[] = "ウィンドウ表示";

bool g_IsExitDialogOpen = false;
bool g_ExitDialogJustClosed = false;



HWND GameWindow_Create(HINSTANCE hInstance)
{
    WNDCLASSEX wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = WINDOW_CLASS;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    /* メインウィンドウの作成 */
    constexpr int SCREEN_WIDTH = 1600;
    constexpr int SCREEN_HEIGHT = 900;

    RECT window_rect{ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

    DWORD style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);

    AdjustWindowRect(&window_rect, style, FALSE);

    const int WINDOW_WIDTH = window_rect.right - window_rect.left;
    const int WINDOW_HEIGHT = window_rect.bottom - window_rect.top;

    int desktop_width = GetSystemMetrics(SM_CXSCREEN);
    int desktop_height = GetSystemMetrics(SM_CYSCREEN);

    /* ウィンドウの表示位置（中央） */
    const int WINDOW_X = std::max((desktop_width - WINDOW_WIDTH) / 2, 0);
    const int WINDOW_Y = std::max((desktop_height - WINDOW_HEIGHT) / 2, 0);

    HWND hWnd = CreateWindow(
        WINDOW_CLASS,
        TITLE,
        style,
        WINDOW_X,
        WINDOW_Y,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr);

    return hWnd;
}

/* ウィンドウプロシージャ */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //==========================================================================
    // 入力モジュールへ「全メッセージ」を常に転送（取りこぼし防止）
    //==========================================================================
    Keyboard_ProcessMessage(message, wParam, lParam);
    Mouse_ProcessMessage(message, wParam, lParam);

    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            g_IsExitDialogOpen = true;
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_CLOSE:
    {
        int r = MessageBox(hWnd, "本当に終了しますか？", "確認",
            MB_OKCANCEL | MB_DEFBUTTON2);

        if (r == IDOK)
        {
            DestroyWindow(hWnd);
        }
        else
        {
            //  閉じた直後に main 側で時刻補正させる
            g_IsExitDialogOpen = false;
            g_ExitDialogJustClosed = true;
        }
        return 0;
    }



    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
