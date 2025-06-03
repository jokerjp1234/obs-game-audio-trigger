#include "audio-player.h"
#include <obs-module.h>
#include <algorithm>
#include <filesystem>

// 簡易版AudioPlayer - Windows PlaySoundを使用

const std::vector<std::string> AudioPlayer::supported_extensions_ = {
    ".wav"  // 簡易版ではWAVのみサポート
};

AudioPlayer::AudioPlayer()
    : is_playing_(false)
    , current_state_(PlaybackState::STOPPED)
    , volume_(1.0f)
    , speed_(1.0f)
    , pitch_(1.0f)
    , looping_(false)
{
    audio_info_ = {};
}

AudioPlayer::~AudioPlayer()
{
    stop();
}

bool AudioPlayer::load_audio_file(const std::string& file_path)
{
    if (file_path.empty()) {
        blog(LOG_WARNING, "[AudioPlayer] Empty file path provided");
        return false;
    }

    if (!std::filesystem::exists(file_path)) {
        blog(LOG_ERROR, "[AudioPlayer] File does not exist: %s", file_path.c_str());
        return false;
    }

    if (!is_supported_format(file_path)) {
        blog(LOG_WARNING, "[AudioPlayer] Unsupported file format: %s", file_path.c_str());
        return false;
    }

    current_file_ = file_path;
    audio_info_.file_path = file_path;
    audio_info_.format = get_file_extension(file_path);
    audio_info_.duration_seconds = 0.0f;  // 簡易版では取得しない
    audio_info_.sample_rate = 44100;      // デフォルト値
    audio_info_.channels = 2;             // デフォルト値

    blog(LOG_INFO, "[AudioPlayer] Successfully loaded: %s", file_path.c_str());
    return true;
}

bool AudioPlayer::play()
{
    if (current_file_.empty()) {
        blog(LOG_WARNING, "[AudioPlayer] No audio file loaded");
        return false;
    }

    // Windows PlaySound APIを使用（簡易実装）
    std::wstring wide_path(current_file_.begin(), current_file_.end());
    
    DWORD flags = SND_FILENAME | SND_ASYNC;
    if (looping_) {
        flags |= SND_LOOP;
    }

    BOOL result = PlaySoundW(wide_path.c_str(), NULL, flags);
    
    if (result) {
        is_playing_ = true;
        current_state_ = PlaybackState::PLAYING;
        blog(LOG_INFO, "[AudioPlayer] Playback started");
        return true;
    } else {
        blog(LOG_ERROR, "[AudioPlayer] Failed to play audio");
        current_state_ = PlaybackState::ERROR;
        return false;
    }
}

bool AudioPlayer::stop()
{
    // PlaySoundを停止
    PlaySoundW(NULL, NULL, SND_PURGE);
    
    is_playing_ = false;
    current_state_ = PlaybackState::STOPPED;
    
    blog(LOG_INFO, "[AudioPlayer] Playback stopped");
    return true;
}

std::vector<std::string> AudioPlayer::get_supported_formats() const
{
    return supported_extensions_;
}

void AudioPlayer::log_audio_info() const
{
    if (current_file_.empty()) {
        blog(LOG_INFO, "[AudioPlayer] No file loaded");
        return;
    }
    
    blog(LOG_INFO, "[AudioPlayer] Current file info:");
    blog(LOG_INFO, "  Path: %s", audio_info_.file_path.c_str());
    blog(LOG_INFO, "  Format: %s", audio_info_.format.c_str());
    blog(LOG_INFO, "  Volume: %.2f", volume_);
    blog(LOG_INFO, "  Looping: %s", looping_ ? "Yes" : "No");
}

bool AudioPlayer::is_supported_format(const std::string& file_path) const
{
    std::string extension = get_file_extension(file_path);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return std::find(supported_extensions_.begin(), supported_extensions_.end(), extension) 
           != supported_extensions_.end();
}

std::string AudioPlayer::get_file_extension(const std::string& file_path) const
{
    size_t last_dot = file_path.find_last_of('.');
    if (last_dot == std::string::npos) {
        return "";
    }
    
    std::string extension = file_path.substr(last_dot);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}