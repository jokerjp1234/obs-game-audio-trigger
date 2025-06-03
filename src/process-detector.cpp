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
    
    // プロセスIDを検索
    process_id_ = find_process_id(process_name);
    if (process_id_ == 0) {
        blog(LOG_INFO, "[ProcessDetector] Process '%s' not currently running", process_name.c_str());
        target_hwnd_ = nullptr;
        is_initialized_ = false;
        return false;
    }

    // メインウィンドウを検索
    target_hwnd_ = find_main_window(process_id_);
    if (target_hwnd_ == nullptr) {
        blog(LOG_WARNING, "[ProcessDetector] Could not find main window for process '%s'", process_name.c_str());
        is_initialized_ = false;
        return false;
    }

    // キャプチャコンテキストをセットアップ
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
    if (!is_initialized_ || target_hwnd_ == nullptr) {
        return false;
    }

    // ウィンドウが最小化されているかチェック
    if (IsIconic(target_hwnd_)) {
        return false;
    }

    // ウィンドウが表示されているかチェック
    if (!IsWindowVisible(target_hwnd_)) {
        return false;
    }

    // ウィンドウの位置とサイズを取得
    RECT window_rect;
    if (capture_client_area_) {
        if (!GetClientRect(target_hwnd_, &window_rect)) {
            blog(LOG_WARNING, "[ProcessDetector] Failed to get client rect");
            return false;
        }
    } else {
        if (!GetWindowRect(target_hwnd_, &window_rect)) {
            blog(LOG_WARNING, "[ProcessDetector] Failed to get window rect");
            return false;
        }
    }

    int width = window_rect.right - window_rect.left;
    int height = window_rect.bottom - window_rect.top;

    // 最小サイズチェック
    if (width < min_window_width_ || height < min_window_height_) {
        return false;
    }

    // ウィンドウサイズが変わった場合、キャプチャコンテキストを再設定
    if (memcmp(&window_rect, &last_window_rect_, sizeof(RECT)) != 0) {
        cleanup_capture_context();
        if (!setup_capture_context()) {
            return false;
        }
        last_window_rect_ = window_rect;
    }

    // Windows 8以降ではDWM APIを優先使用
    if (capture_window_dwm(output_image)) {
        return true;
    }

    // フォールバックとしてGDIを使用
    return capture_window_gdi(output_image);
}

// 続きは次のファイルで..."