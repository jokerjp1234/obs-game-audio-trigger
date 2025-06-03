#pragma once

#include <string>
#include <memory>
#include <vector>
#include <windows.h>

// 簡易版AudioPlayer - miniaudioを使わずにWindows APIで実装

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
    
    // 初期化（簡易版は常に成功）
    bool initialize() { return true; }
    void shutdown() {}
    bool is_initialized() const { return true; }
    
    // 音声ファイルの読み込み
    bool load_audio_file(const std::string& file_path);
    bool is_file_loaded() const { return !current_file_.empty(); }
    AudioInfo get_audio_info() const { return audio_info_; }
    
    // 再生制御（Windows PlaySoundを使用）
    bool play();
    bool play_with_duration(float duration_seconds) { return play(); }
    bool pause() { return true; }
    bool stop();
    bool is_playing() const { return is_playing_; }
    PlaybackState get_state() const { return current_state_; }
    
    // 再生パラメータ（簡易版では保存のみ）
    void set_volume(float volume) { volume_ = volume; }
    void set_speed(float speed) { speed_ = speed; }
    void set_pitch(float pitch) { pitch_ = pitch; }
    float get_volume() const { return volume_; }
    float get_speed() const { return speed_; }
    float get_pitch() const { return pitch_; }
    
    // 再生位置（簡易版では未実装）
    void set_position(float seconds) {}
    float get_position() const { return 0.0f; }
    float get_duration() const { return audio_info_.duration_seconds; }
    
    // フェード効果（簡易版では未実装）
    void fade_in(float duration_seconds) {}
    void fade_out(float duration_seconds) {}
    
    // プレイリスト（簡易版では未実装）
    bool add_to_playlist(const std::string& file_path) { return true; }
    void clear_playlist() {}
    bool play_next() { return true; }
    bool play_random() { return true; }
    
    // 設定
    void set_looping(bool enable) { looping_ = enable; }
    void set_auto_stop_duration(float seconds) {}
    
    // サポート形式
    std::vector<std::string> get_supported_formats() const;
    void log_audio_info() const;

private:
    // ファイル形式判定
    bool is_supported_format(const std::string& file_path) const;
    std::string get_file_extension(const std::string& file_path) const;

private:
    // 状態管理
    bool is_playing_;
    PlaybackState current_state_;
    
    // 音声ファイル情報
    std::string current_file_;
    AudioInfo audio_info_;
    
    // 再生パラメータ
    float volume_;
    float speed_;
    float pitch_;
    bool looping_;
    
    // サポートする形式
    static const std::vector<std::string> supported_extensions_;
};