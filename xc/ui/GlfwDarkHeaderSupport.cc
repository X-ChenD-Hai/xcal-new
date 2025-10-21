#include "./GlfwDarkHeaderSupport.h"
// 平台检测宏
#if defined(_WIN32) || defined(_WIN64)
#define XC_PLATFORM_WINDOWS 1
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#include <windows.h>
#pragma comment(lib, "dwmapi.lib")
#elif defined(__APPLE__)
#define XC_PLATFORM_MACOS 1
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#define XC_PLATFORM_MAC 1
#endif
#elif defined(__linux__)
#define XC_PLATFORM_LINUX 1
// Linux 平台可能需要额外的头文件
#endif

// Windows 深色模式常量定义
#ifdef XC_PLATFORM_WINDOWS
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif
#endif

/**
 * @brief 为 GLFW 窗口启用深色标题栏模式（跨平台）
 * @param window GLFW 窗口指针
 * @return 成功返回 true，失败或平台不支持返回 false
 */
 bool enable_window_dark_titlebar(GLFWwindow* window) {
    if (!window) return false;

#if defined(XC_PLATFORM_WINDOWS)
    // Windows 平台实现
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) return false;

    // 尝试使用较新的 API (Windows 10 20H1+)
    BOOL useDarkMode = TRUE;
    HRESULT result = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                                           &useDarkMode, sizeof(useDarkMode));

    if (FAILED(result)) {
        // 回退到旧版 API (Windows 10 1809-1909)
        result = DwmSetWindowAttribute(
            hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &useDarkMode,
            sizeof(useDarkMode));
    }

    if (SUCCEEDED(result)) {
        // 强制窗口重绘以应用新样式
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            SetWindowPos(hwnd, NULL, rect.left, rect.top,
                         rect.right - rect.left, rect.bottom - rect.top - 1,
                         SWP_NOZORDER | SWP_FRAMECHANGED);
            SetWindowPos(hwnd, NULL, rect.left, rect.top,
                         rect.right - rect.left, rect.bottom - rect.top,
                         SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        return true;
    }
    return false;

#elif defined(XC_PLATFORM_MAC)
// macOS 平台实现
// 注意：GLFW 在 macOS 上使用 NSWindow，需要 Objective-C 或 Swift 代码
// 这里只是一个占位符，实际需要 Objective-C++ 实现
#pragma message( \
    "macOS dark titlebar support requires Objective-C implementation")
    return false;

#elif defined(XC_PLATFORM_LINUX)
// Linux 平台实现
// Linux 桌面环境多样，需要根据不同的桌面环境实现
// 常见的如 GNOME、KDE、Xfce 等各有不同的方法
#pragma message("Linux dark titlebar support depends on desktop environment")
    return false;

#else
// 其他不支持的平台
#pragma message("Dark titlebar not supported on this platform")
    return false;
#endif
}

/**
 * @brief 为 GLFW 窗口禁用深色标题栏模式
 * @param window GLFW 窗口指针
 * @return 成功返回 true，失败或平台不支持返回 false
 */
 bool disable_window_dark_titlebar(GLFWwindow* window) {
    if (!window) return false;

#if defined(XC_PLATFORM_WINDOWS)
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) return false;

    BOOL useDarkMode = FALSE;
    HRESULT result = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                                           &useDarkMode, sizeof(useDarkMode));

    if (FAILED(result)) {
        result = DwmSetWindowAttribute(
            hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &useDarkMode,
            sizeof(useDarkMode));
    }

    if (SUCCEEDED(result)) {
        // 强制重绘
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            SetWindowPos(hwnd, NULL, rect.left, rect.top,
                         rect.right - rect.left, rect.bottom - rect.top,
                         SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        return true;
    }
    return false;

#else
    // 其他平台暂不支持或需要相应实现
    return false;
#endif
}

// 平台检测辅助函数
inline const char* get_platform_name() {
#if defined(XC_PLATFORM_WINDOWS)
    return "Windows";
#elif defined(XC_PLATFORM_MAC)
    return "macOS";
#elif defined(XC_PLATFORM_LINUX)
    return "Linux";
#else
    return "Unknown";
#endif
}
