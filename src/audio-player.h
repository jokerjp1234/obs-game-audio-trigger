#pragma once

#include <string>
#include <memory>
#include <vector>
#include <windows.h>
#include <mmsystem.h>

// 前方宣言
struct ma_engine;
struct ma_sound;

/**
 * 音声ファイル再生クラス
 * WAV, MP3, OGGなどの音声ファイルを再生
 * 音量、速度、再生時間の制御が可能
 */
class AudioPlayer {
public:
    enum class PlaybackState {
        STOPPED,
        PLAYING,
        PAUSED,
        ERROR
    };
    
    struct AudioInfo {
        std::string file_path;
        float duration_seconds;
        int sample_rate;
        int channels;
        std::string format;
    };

public:
    AudioPlayer();
    ~AudioPlayer();
    
    // 初期化
    bool initialize();
    void shutdown();
    bool is_initialized() const;
    
    // 音声ファイルの読み込み
    bool load_audio_file(const std::string& file_path);
    bool is_file_loaded() const;
    AudioInfo get_audio_info() const;
    
    // 再生制御
    bool play();
    bool play_with_duration(float duration_seconds);
    bool pause();
    bool stop();
    bool is_playing() const;
    PlaybackState get_state() const;
    
    // 再生パラメータ
    void set_volume(float volume);          // 0.0-1.0
    void set_speed(float speed);            // 0.1-3.0
    void set_pitch(float pitch);            // 0.5-2.0
    float get_volume() const;
    float get_speed() const;
    float get_pitch() const;
    
    // 再生位置
    void set_position(float seconds);
    float get_position() const;
    float get_duration() const;
    
    // フェード効果
    void fade_in(float duration_seconds);
    void fade_out(float duration_seconds);
    
    // 複数ファイル対応
    bool add_to_playlist(const std::string& file_path);
    void clear_playlist();
    bool play_next();
    bool play_random();
    
    // 設定
    void set_looping(bool enable);
    void set_auto_stop_duration(float seconds);
    
    // デバッグ・統計
    std::vector<std::string> get_supported_formats() const;
    void log_audio_info() const;

private:
    // miniaudio関連
    bool initialize_miniaudio();
    void cleanup_miniaudio();
    
    // ファイル形式判定
    bool is_supported_format(const std::string& file_path) const;
    std::string get_file_extension(const std::string& file_path) const;
    
    // エラーハンドリング
    void handle_audio_error(int result, const std::string& operation);
    std::string get_error_string(int error_code) const;
    
    // コールバック関数
    static void end_callback(void* user_data, ma_sound* sound);
    void on_playback_end();

private:
    // miniaudio エンジン
    std::unique_ptr<ma_engine> audio_engine_;
    std::unique_ptr<ma_sound> current_sound_;
    
    // 状態管理
    bool is_initialized_;
    PlaybackState current_state_;
    
    // 音声ファイル情報
    std::string current_file_path_;
    AudioInfo current_audio_info_;
    bool is_file_loaded_;
    
    // 再生パラメータ
    float volume_;
    float speed_;
    float pitch_;
    bool looping_enabled_;
    
    // 自動停止
    float auto_stop_duration_;
    bool auto_stop_enabled_;
    DWORD playback_start_time_;
    
    // プレイリスト
    std::vector<std::string> playlist_;
    int current_playlist_index_;
    
    // サポートする形式
    static const std::vector<std::string> supported_extensions_;
};