#include "process-detector.h"
#include <obs-module.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#include <algorithm>
#include <vector>

#pragma comment(lib, "dwmapi.lib")

ProcessDetector::ProcessDetector()
    : target_process_name_("")
    , process_id_(0)
    , target_hwnd_(nullptr)
    , capture_client_area_(true)
    , min_window_width_(100)
    , min_window_height_(100)
    , window_dc_(nullptr)
    , memory_dc_(nullptr)
    , memory_bitmap_(nullptr)
    , old_bitmap_(nullptr)
    , is_initialized_(false)
    , is_capture_ready_(false)
{
    ZeroMemory(&last_window_rect_, sizeof(RECT));
}

ProcessDetector::~ProcessDetector()
{
    cleanup_capture_context();
}

bool ProcessDetector::set_target_process(const std::string& process_name)
{
    if (process_name.empty()) {
        blog(LOG_WARNING, "[ProcessDetector] Empty process name provided");
        return false;
    }

    target_process_name_ = process_name;
    process_id_ = find_process_id(process_name);
    if (process_id_ == 0) {
        blog(LOG_INFO, "[ProcessDetector] Process '%s' not currently running", process_name.c_str());
        target_hwnd_ = nullptr;
        is_initialized_ = false;
        return false;
    }

    target_hwnd_ = find_main_window(process_id_);
    if (target_hwnd_ == nullptr) {
        blog(LOG_WARNING, "[ProcessDetector] Could not find main window for process '%s'", process_name.c_str());
        is_initialized_ = false;
        return false;
    }

    is_initialized_ = setup_capture_context();
    if (is_initialized_) {
        blog(LOG_INFO, "[ProcessDetector] Successfully initialized for process '%s' (PID: %lu)", 
             process_name.c_str(), process_id_);
        log_window_info();
    }

    return is_initialized_;
}

bool ProcessDetector::is_process_running() const
{
    if (process_id_ == 0) return false;

    HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id_);
    if (process_handle == nullptr) return false;

    DWORD exit_code;
    bool running = GetExitCodeProcess(process_handle, &exit_code) && exit_code == STILL_ACTIVE;
    CloseHandle(process_handle);
    return running;
}

bool ProcessDetector::refresh_process_info()
{
    if (target_process_name_.empty()) return false;
    cleanup_capture_context();
    return set_target_process(target_process_name_);
}

bool ProcessDetector::capture_window(cv::Mat& output_image)
{
    if (!is_initialized_ || target_hwnd_ == nullptr) return false;
    if (IsIconic(target_hwnd_) || !IsWindowVisible(target_hwnd_)) return false;

    RECT window_rect;
    if (capture_client_area_) {
        if (!GetClientRect(target_hwnd_, &window_rect)) return false;
    } else {
        if (!GetWindowRect(target_hwnd_, &window_rect)) return false;
    }

    int width = window_rect.right - window_rect.left;
    int height = window_rect.bottom - window_rect.top;

    if (width < min_window_width_ || height < min_window_height_) return false;

    if (memcmp(&window_rect, &last_window_rect_, sizeof(RECT)) != 0) {
        cleanup_capture_context();
        if (!setup_capture_context()) return false;
        last_window_rect_ = window_rect;
    }

    return capture_window_dwm(output_image) || capture_window_gdi(output_image);
}

bool ProcessDetector::is_window_visible() const
{
    return target_hwnd_ != nullptr && IsWindowVisible(target_hwnd_) && !IsIconic(target_hwnd_);
}

RECT ProcessDetector::get_window_rect() const
{
    RECT rect = {0};
    if (target_hwnd_) {
        if (capture_client_area_) {
            GetClientRect(target_hwnd_, &rect);
        } else {
            GetWindowRect(target_hwnd_, &rect);
        }
    }
    return rect;
}

std::string ProcessDetector::get_window_title() const
{
    if (target_hwnd_ == nullptr) return "";
    char title[256];
    int length = GetWindowTextA(target_hwnd_, title, sizeof(title));
    return std::string(title, length);
}

void ProcessDetector::set_capture_client_area(bool client_area_only)
{
    if (capture_client_area_ != client_area_only) {
        capture_client_area_ = client_area_only;
        if (is_initialized_) {
            cleanup_capture_context();
            setup_capture_context();
        }
    }
}

void ProcessDetector::set_min_window_size(int min_width, int min_height)
{
    min_window_width_ = std::max(1, min_width);
    min_window_height_ = std::max(1, min_height);
}

std::vector<std::string> ProcessDetector::get_running_processes() const
{
    std::vector<std::string> processes;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return processes;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &pe32)) {
        do {
            processes.push_back(pe32.szExeFile);
        } while (Process32Next(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    return processes;
}

void ProcessDetector::log_window_info() const
{
    if (target_hwnd_ == nullptr) return;

    RECT window_rect = get_window_rect();
    std::string title = get_window_title();
    
    blog(LOG_INFO, "[ProcessDetector] Window Info:");
    blog(LOG_INFO, "  Title: %s", title.c_str());
    blog(LOG_INFO, "  Size: %dx%d", window_rect.right - window_rect.left, window_rect.bottom - window_rect.top);
    blog(LOG_INFO, "  HWND: 0x%p", target_hwnd_);
    blog(LOG_INFO, "  Client Area Only: %s", capture_client_area_ ? "Yes" : "No");
}

DWORD ProcessDetector::find_process_id(const std::string& process_name)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    DWORD found_pid = 0;

    if (Process32First(snapshot, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, process_name.c_str()) == 0) {
                found_pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    return found_pid;
}

struct EnumWindowsData {
    DWORD process_id;
    HWND result_hwnd;
};

BOOL CALLBACK ProcessDetector::enum_windows_proc(HWND hwnd, LPARAM lparam)
{
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lparam);
    DWORD window_process_id;
    GetWindowThreadProcessId(hwnd, &window_process_id);
    
    if (window_process_id == data->process_id) {
        if (IsWindowVisible(hwnd) && GetWindowTextLengthA(hwnd) > 0) {
            if (GetParent(hwnd) == nullptr) {
                data->result_hwnd = hwnd;
                return FALSE;
            }
        }
    }
    return TRUE;
}

HWND ProcessDetector::find_main_window(DWORD process_id)
{
    EnumWindowsData data;
    data.process_id = process_id;
    data.result_hwnd = nullptr;
    EnumWindows(enum_windows_proc, reinterpret_cast<LPARAM>(&data));
    return data.result_hwnd;
}

bool ProcessDetector::setup_capture_context()
{
    if (target_hwnd_ == nullptr) return false;
    cleanup_capture_context();

    window_dc_ = GetDC(target_hwnd_);
    if (window_dc_ == nullptr) {
        blog(LOG_ERROR, "[ProcessDetector] Failed to get window DC");
        return false;
    }

    memory_dc_ = CreateCompatibleDC(window_dc_);
    if (memory_dc_ == nullptr) {
        blog(LOG_ERROR, "[ProcessDetector] Failed to create memory DC");
        cleanup_capture_context();
        return false;
    }

    is_capture_ready_ = true;
    return true;
}

void ProcessDetector::cleanup_capture_context()
{
    if (old_bitmap_ && memory_dc_) {
        SelectObject(memory_dc_, old_bitmap_);
        old_bitmap_ = nullptr;
    }
    if (memory_bitmap_) {
        DeleteObject(memory_bitmap_);
        memory_bitmap_ = nullptr;
    }
    if (memory_dc_) {
        DeleteDC(memory_dc_);
        memory_dc_ = nullptr;
    }
    if (window_dc_ && target_hwnd_) {
        ReleaseDC(target_hwnd_, window_dc_);
        window_dc_ = nullptr;
    }
    is_capture_ready_ = false;
}

bool ProcessDetector::capture_window_gdi(cv::Mat& output_image)
{
    if (!is_capture_ready_) return false;

    RECT window_rect = get_window_rect();
    int width = window_rect.right - window_rect.left;
    int height = window_rect.bottom - window_rect.top;
    if (width <= 0 || height <= 0) return false;

    if (memory_bitmap_ == nullptr) {
        memory_bitmap_ = CreateCompatibleBitmap(window_dc_, width, height);
        if (memory_bitmap_ == nullptr) {
            blog(LOG_ERROR, "[ProcessDetector] Failed to create compatible bitmap");
            return false;
        }
        old_bitmap_ = static_cast<HBITMAP>(SelectObject(memory_dc_, memory_bitmap_));
    }

    if (!BitBlt(memory_dc_, 0, 0, width, height, window_dc_, 0, 0, SRCCOPY)) {
        blog(LOG_WARNING, "[ProcessDetector] BitBlt failed");
        return false;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    output_image.create(height, width, CV_8UC4);
    int result = GetDIBits(memory_dc_, memory_bitmap_, 0, height, output_image.data, &bmi, DIB_RGB_COLORS);
    
    if (result == 0) {
        blog(LOG_WARNING, "[ProcessDetector] GetDIBits failed");
        return false;
    }

    cv::cvtColor(output_image, output_image, cv::COLOR_BGRA2BGR);
    return true;
}

bool ProcessDetector::capture_window_dwm(cv::Mat& output_image)
{
    RECT window_rect = get_window_rect();
    int width = window_rect.right - window_rect.left;
    int height = window_rect.bottom - window_rect.top;
    if (width <= 0 || height <= 0) return false;

    HDC screen_dc = GetDC(nullptr);
    HDC mem_dc = CreateCompatibleDC(screen_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(screen_dc, width, height);
    HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(mem_dc, bitmap));

    BOOL print_result = PrintWindow(target_hwnd_, mem_dc, capture_client_area_ ? PW_CLIENTONLY : 0);

    if (print_result) {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        output_image.create(height, width, CV_8UC4);
        int result = GetDIBits(mem_dc, bitmap, 0, height, output_image.data, &bmi, DIB_RGB_COLORS);
        
        if (result > 0) {
            cv::cvtColor(output_image, output_image, cv::COLOR_BGRA2BGR);
        } else {
            print_result = FALSE;
        }
    }

    SelectObject(mem_dc, old_bmp);
    DeleteObject(bitmap);
    DeleteDC(mem_dc);
    ReleaseDC(nullptr, screen_dc);
    return print_result != FALSE;
}

bool ProcessDetector::is_valid_window(HWND hwnd) const
{
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd)) return false;
    
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) return false;
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    return width >= min_window_width_ && height >= min_window_height_;
}