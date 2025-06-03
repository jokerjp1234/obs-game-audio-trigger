#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <opencv2/opencv.hpp>

/**
 * プロセス検出およびウィンドウキャプチャクラス
 */
class ProcessDetector {
public:
    ProcessDetector();
    ~ProcessDetector();
    
    // プロセス関連
    bool set_target_process(const std::string& process_name);
    bool is_process_running() const;
    bool refresh_process_info();
    
    // ウィンドウキャプチャ関連
    bool capture_window(cv::Mat& output_image);
    bool is_window_visible() const;
    
    // ウィンドウ情報取得
    RECT get_window_rect() const;
    std::string get_window_title() const;
    
    // 設定
    void set_capture_client_area(bool client_area_only);
    void set_min_window_size(int min_width, int min_height);
    
    // デバッグ用
    std::vector<std::string> get_running_processes() const;
    void log_window_info() const;

private:
    // Windows API関連
    DWORD find_process_id(const std::string& process_name);
    HWND find_main_window(DWORD process_id);
    bool setup_capture_context();
    void cleanup_capture_context();
    
    // キャプチャ関連
    bool capture_window_gdi(cv::Mat& output_image);
    bool capture_window_dwm(cv::Mat& output_image); // Windows 8+用
    
    // ヘルパー関数
    static BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lparam);
    bool is_valid_window(HWND hwnd) const;
    
private:
    // プロセス情報
    std::string target_process_name_;
    DWORD process_id_;
    HWND target_hwnd_;
    
    // キャプチャ設定
    bool capture_client_area_;
    int min_window_width_;
    int min_window_height_;
    
    // キャプチャコンテキスト
    HDC window_dc_;
    HDC memory_dc_;
    HBITMAP memory_bitmap_;
    HBITMAP old_bitmap_;
    
    // 状態管理
    bool is_initialized_;
    bool is_capture_ready_;
    RECT last_window_rect_;
};