#pragma once

//= INCLUDES =====================
#include <string>
#include <Windows.h>
#include <functional>
#include "FileSystem/FileSystem.h"
#include "Core/Engine.h"
//================================

namespace Window
{
	static HINSTANCE g_instance;
	static HWND g_handle;
	static std::function<void(Spartan::WindowData& window_data)> g_on_message;

    inline void GetWindowSize(float* width, float* height)
    {
        RECT rect;
        ::GetClientRect(g_handle, &rect);
        *width  = static_cast<float>(rect.right - rect.left);
        *height = static_cast<float>(rect.bottom - rect.top);
    }

    inline uint32_t GetWidth()
    {
        RECT rect;
        GetClientRect(g_handle, &rect);
        return static_cast<uint32_t>(rect.right - rect.left);
    }

    inline uint32_t GetHeight()
    {
        RECT rect;
        GetClientRect(g_handle, &rect);
        return static_cast<uint32_t>(rect.bottom - rect.top);
    }

	// Window Procedure
	inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
        LRESULT result = 0;

        Spartan::WindowData window_data;
        window_data.handle      = static_cast<void*>(g_handle);
        window_data.instance    = static_cast<void*>(g_instance);
        window_data.message     = static_cast<uint32_t>(msg);
        window_data.wparam      = static_cast<int64_t>(wParam);
        window_data.lparam      = static_cast<uint64_t>(lParam);
        GetWindowSize(&window_data.width, &window_data.height);

        if (msg == WM_DISPLAYCHANGE || msg == WM_SIZE)
        { 
            window_data.width   = static_cast<float>(lParam & 0xffff);
            window_data.height  = static_cast<float>((lParam >> 16) & 0xffff);
        }
        else if (msg == WM_CLOSE)
        {
            PostQuitMessage(0);
        }
        else
        {
            result = DefWindowProc(hwnd, msg, wParam, lParam);
        }

        if (g_on_message)
        {
            g_on_message(window_data);
        }

        return result;
	}

	inline bool Create(HINSTANCE instance, const std::string& title)
	{
		g_instance = instance;
		std::wstring windowTitle	= Spartan::FileSystem::StringToWstring(title);
		int windowWidth				= GetSystemMetrics(SM_CXSCREEN);
		int windowHeight			= GetSystemMetrics(SM_CYSCREEN);
		LPCWSTR className			= L"myWindowClass";
	
		// Register the Window Class
		WNDCLASSEX wc;
		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style         = 0;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = g_instance;
		wc.hIcon         = LoadIcon(instance, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(instance, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName  = nullptr;
		wc.lpszClassName = className;
		wc.hIconSm       = LoadIcon(instance, IDI_APPLICATION);
	
		if(!RegisterClassEx(&wc))
		{
			MessageBox(nullptr, L"Window registration failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			return false;
		}
	
		// Create the Window
		g_handle = CreateWindowEx
		(
			WS_EX_CLIENTEDGE,
			className,
			windowTitle.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
			nullptr, nullptr, g_instance, nullptr
		);
	
		if(!g_handle)
		{
			MessageBox(nullptr, L"Window creation failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			return false;
		}

		return true;
	}

	inline void Show()
	{
		ShowWindow(g_handle, SW_MAXIMIZE);
		UpdateWindow(g_handle);
		SetFocus(g_handle);	
	}

	inline bool Tick()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return msg.message != WM_QUIT;
	}

    inline void Destroy()
	{
		DestroyWindow(g_handle);
	}
}
