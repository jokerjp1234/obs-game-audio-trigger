#include "audio-player.h"
#include <obs-module.h>
#include <algorithm>
#include <filesystem>

// miniaudioの実装をインクルード
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

const std::vector<std::string> AudioPlayer::supported_extensions_ = {
    ".wav", ".mp3", ".ogg", ".flac", ".m4a", ".aac"
};

AudioPlayer::AudioPlayer()
    : audio_engine_(nullptr)
    , current_sound_(nullptr)
    , is_initialized_(false)
    , current_state_(PlaybackState::STOPPED)
    , is_file_loaded_(false)
    , volume_(1.0f)
    , speed_(1.0f)
    , pitch_(1.0f)
    , looping_enabled_(false)
    , auto_stop_duration_(-1.0f)
    , auto_stop_enabled_(false)
    , playback_start_time_(0)
    , current_playlist_index_(-1)
{
    current_audio_info_ = {};
}

AudioPlayer::~AudioPlayer()
{
    shutdown();
}

bool AudioPlayer::initialize()
{
    if (is_initialized_) return true;

    try {
        audio_engine_ = std::make_unique<ma_engine>();
        
        ma_result result = ma_engine_init(nullptr, audio_engine_.get());
        if (result != MA_SUCCESS) {
            handle_audio_error(result, "engine initialization");
            audio_engine_.reset();
            return false;
        }

        is_initialized_ = true;
        current_state_ = PlaybackState::STOPPED;
        
        blog(LOG_INFO, "[AudioPlayer] Successfully initialized miniaudio engine");
        return true;
    }
    catch (const std::exception& e) {
        blog(LOG_ERROR, "[AudioPlayer] Exception during initialization: %s", e.what());
        return false;
    }
}

void AudioPlayer::shutdown()
{
    if (!is_initialized_) return;

    stop();

    if (current_sound_) {
        ma_sound_uninit(current_sound_.get());
        current_sound_.reset();
    }

    if (audio_engine_) {
        ma_engine_uninit(audio_engine_.get());
        audio_engine_.reset();
    }

    is_initialized_ = false;
    is_file_loaded_ = false;
    current_state_ = PlaybackState::STOPPED;
    
    blog(LOG_INFO, "[AudioPlayer] Audio engine shutdown complete");
}

bool AudioPlayer::is_initialized() const
{
    return is_initialized_;
}

bool AudioPlayer::load_audio_file(const std::string& file_path)
{
    if (!is_initialized_) {
        blog(LOG_ERROR, "[AudioPlayer] Not initialized");
        return false;
    }

    if (file_path.empty() || !std::filesystem::exists(file_path)) {
        blog(LOG_ERROR, "[AudioPlayer] File does not exist: %s", file_path.c_str());
        return false;
    }

    if (!is_supported_format(file_path)) {
        blog(LOG_WARNING, "[AudioPlayer] Unsupported file format: %s", file_path.c_str());
        return false;
    }

    stop();
    if (current_sound_) {
        ma_sound_uninit(current_sound_.get());
        current_sound_.reset();
    }

    try {
        current_sound_ = std::make_unique<ma_sound>();
        
        ma_result result = ma_sound_init_from_file(audio_engine_.get(), file_path.c_str(),
                                                  MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC,
                                                  nullptr, nullptr, current_sound_.get());
        
        if (result != MA_SUCCESS) {
            handle_audio_error(result, "sound initialization");
            current_sound_.reset();
            return false;
        }

        current_audio_info_.file_path = file_path;
        
        ma_uint64 length_in_frames;
        result = ma_sound_get_length_in_frames(current_sound_.get(), &length_in_frames);
        if (result == MA_SUCCESS) {
            ma_uint32 sample_rate;
            ma_sound_get_data_format(current_sound_.get(), nullptr, nullptr, &sample_rate, nullptr, 0);
            current_audio_info_.duration_seconds = static_cast<float>(length_in_frames) / sample_rate;
            current_audio_info_.sample_rate = sample_rate;
        } else {
            current_audio_info_.duration_seconds = 0.0f;
            current_audio_info_.sample_rate = 0;
        }

        ma_uint32 channels;
        ma_sound_get_data_format(current_sound_.get(), nullptr, &channels, nullptr, nullptr, 0);
        current_audio_info_.channels = channels;
        current_audio_info_.format = get_file_extension(file_path);

        set_volume(volume_);
        set_speed(speed_);
        ma_sound_set_looping(current_sound_.get(), looping_enabled_ ? MA_TRUE : MA_FALSE);

        is_file_loaded_ = true;
        current_state_ = PlaybackState::STOPPED;

        blog(LOG_INFO, "[AudioPlayer] Successfully loaded: %s (%.2fs, %dHz, %dch)", 
             file_path.c_str(), current_audio_info_.duration_seconds,
             current_audio_info_.sample_rate, current_audio_info_.channels);

        return true;
    }
    catch (const std::exception& e) {
        blog(LOG_ERROR, "[AudioPlayer] Exception while loading file: %s", e.what());
        if (current_sound_) {
            ma_sound_uninit(current_sound_.get());
            current_sound_.reset();
        }
        return false;
    }
}

bool AudioPlayer::is_file_loaded() const
{
    return is_file_loaded_ && current_sound_ != nullptr;
}

AudioPlayer::AudioInfo AudioPlayer::get_audio_info() const
{
    return current_audio_info_;
}

bool AudioPlayer::play()
{
    if (!is_file_loaded_ || !current_sound_) {
        blog(LOG_WARNING, "[AudioPlayer] No audio file loaded");
        return false;
    }

    try {
        ma_result result = ma_sound_start(current_sound_.get());
        if (result != MA_SUCCESS) {
            handle_audio_error(result, "sound start");
            return false;
        }

        current_state_ = PlaybackState::PLAYING;
        playback_start_time_ = GetTickCount();
        auto_stop_enabled_ = false;

        blog(LOG_INFO, "[AudioPlayer] Playback started");
        return true;
    }
    catch (const std::exception& e) {
        blog(LOG_ERROR, "[AudioPlayer] Exception during playback: %s", e.what());
        current_state_ = PlaybackState::ERROR;
        return false;
    }
}

bool AudioPlayer::play_with_duration(float duration_seconds)
{
    if (!play()) return false;

    if (duration_seconds > 0) {
        auto_stop_duration_ = duration_seconds;
        auto_stop_enabled_ = true;
        blog(LOG_INFO, "[AudioPlayer] Playback started with %.2fs duration limit", duration_seconds);
    }

    return true;
}

bool AudioPlayer::pause()
{
    if (!is_file_loaded_ || !current_sound_) return false;

    try {
        ma_result result = ma_sound_stop(current_sound_.get());
        if (result != MA_SUCCESS) {
            handle_audio_error(result, "sound pause");
            return false;
        }

        current_state_ = PlaybackState::PAUSED;
        auto_stop_enabled_ = false;
        blog(LOG_INFO, "[AudioPlayer] Playback paused");
        return true;
    }
    catch (const std::exception& e) {
        blog(LOG_ERROR, "[AudioPlayer] Exception during pause: %s", e.what());
        return false;
    }
}

bool AudioPlayer::stop()
{
    if (!is_file_loaded_ || !current_sound_) {
        current_state_ = PlaybackState::STOPPED;
        return true;
    }

    try {
        ma_sound_stop(current_sound_.get());
        ma_sound_seek_to_pcm_frame(current_sound_.get(), 0);
        current_state_ = PlaybackState::STOPPED;
        auto_stop_enabled_ = false;
        blog(LOG_INFO, "[AudioPlayer] Playback stopped");
        return true;
    }
    catch (const std::exception& e) {
        blog(LOG_ERROR, "[AudioPlayer] Exception during stop: %s", e.what());
        current_state_ = PlaybackState::ERROR;
        return false;
    }
}

bool AudioPlayer::is_playing() const
{
    if (!is_file_loaded_ || !current_sound_) return false;

    if (auto_stop_enabled_ && current_state_ == PlaybackState::PLAYING) {
        DWORD current_time = GetTickCount();
        float elapsed_seconds = (current_time - playback_start_time_) / 1000.0f;
        
        if (elapsed_seconds >= auto_stop_duration_) {
            const_cast<AudioPlayer*>(this)->stop();
            return false;
        }
    }

    return ma_sound_is_playing(current_sound_.get()) == MA_TRUE;
}

AudioPlayer::PlaybackState AudioPlayer::get_state() const
{
    if (!is_file_loaded_) return PlaybackState::STOPPED;
    return is_playing() ? PlaybackState::PLAYING : current_state_;
}

void AudioPlayer::set_volume(float volume)
{
    volume_ = std::clamp(volume, 0.0f, 1.0f);
    if (current_sound_) ma_sound_set_volume(current_sound_.get(), volume_);
}

void AudioPlayer::set_speed(float speed)
{
    speed_ = std::clamp(speed, 0.1f, 3.0f);
    if (current_sound_) ma_sound_set_pitch(current_sound_.get(), speed_);
}

void AudioPlayer::set_pitch(float pitch)
{
    pitch_ = std::clamp(pitch, 0.5f, 2.0f);
    if (current_sound_) ma_sound_set_pitch(current_sound_.get(), pitch_);
}

float AudioPlayer::get_volume() const { return volume_; }
float AudioPlayer::get_speed() const { return speed_; }
float AudioPlayer::get_pitch() const { return pitch_; }

void AudioPlayer::set_position(float seconds)
{
    if (!current_sound_) return;
    ma_uint32 sample_rate;
    ma_sound_get_data_format(current_sound_.get(), nullptr, nullptr, &sample_rate, nullptr, 0);
    ma_uint64 frame_position = static_cast<ma_uint64>(seconds * sample_rate);
    ma_sound_seek_to_pcm_frame(current_sound_.get(), frame_position);
}

float AudioPlayer::get_position() const
{
    if (!current_sound_) return 0.0f;
    ma_uint64 cursor_pos;
    if (ma_sound_get_cursor_in_pcm_frames(current_sound_.get(), &cursor_pos) != MA_SUCCESS) return 0.0f;
    ma_uint32 sample_rate;
    ma_sound_get_data_format(current_sound_.get(), nullptr, nullptr, &sample_rate, nullptr, 0);
    return static_cast<float>(cursor_pos) / sample_rate;
}

float AudioPlayer::get_duration() const { return current_audio_info_.duration_seconds; }

void AudioPlayer::fade_in(float duration_seconds)
{
    if (!current_sound_) return;
    float original_volume = volume_;
    set_volume(0.0f);
    play();
    set_volume(original_volume);
}

void AudioPlayer::fade_out(float duration_seconds) { if (current_sound_) stop(); }

bool AudioPlayer::add_to_playlist(const std::string& file_path)
{
    if (is_supported_format(file_path)) {
        playlist_.push_back(file_path);
        return true;
    }
    return false;
}

void AudioPlayer::clear_playlist()
{
    playlist_.clear();
    current_playlist_index_ = -1;
}

bool AudioPlayer::play_next()
{
    if (playlist_.empty()) return false;
    current_playlist_index_ = (current_playlist_index_ + 1) % playlist_.size();
    return load_audio_file(playlist_[current_playlist_index_]) && play();
}

bool AudioPlayer::play_random()
{
    if (playlist_.empty()) return false;
    current_playlist_index_ = rand() % playlist_.size();
    return load_audio_file(playlist_[current_playlist_index_]) && play();
}

void AudioPlayer::set_looping(bool enable)
{
    looping_enabled_ = enable;
    if (current_sound_) ma_sound_set_looping(current_sound_.get(), enable ? MA_TRUE : MA_FALSE);
}

void AudioPlayer::set_auto_stop_duration(float seconds) { auto_stop_duration_ = seconds; }

std::vector<std::string> AudioPlayer::get_supported_formats() const { return supported_extensions_; }

void AudioPlayer::log_audio_info() const
{
    if (!is_file_loaded_) {
        blog(LOG_INFO, "[AudioPlayer] No file loaded");
        return;
    }
    blog(LOG_INFO, "[AudioPlayer] Current file info:");
    blog(LOG_INFO, "  Path: %s", current_audio_info_.file_path.c_str());
    blog(LOG_INFO, "  Duration: %.2f seconds", current_audio_info_.duration_seconds);
    blog(LOG_INFO, "  Sample Rate: %d Hz", current_audio_info_.sample_rate);
    blog(LOG_INFO, "  Channels: %d", current_audio_info_.channels);
    blog(LOG_INFO, "  Format: %s", current_audio_info_.format.c_str());
    blog(LOG_INFO, "  Volume: %.2f", volume_);
    blog(LOG_INFO, "  Speed: %.2f", speed_);
    blog(LOG_INFO, "  Looping: %s", looping_enabled_ ? "Yes" : "No");
}

bool AudioPlayer::is_supported_format(const std::string& file_path) const
{
    std::string extension = get_file_extension(file_path);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return std::find(supported_extensions_.begin(), supported_extensions_.end(), extension) != supported_extensions_.end();
}

std::string AudioPlayer::get_file_extension(const std::string& file_path) const
{
    size_t last_dot = file_path.find_last_of('.');
    if (last_dot == std::string::npos) return "";
    std::string extension = file_path.substr(last_dot);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}

void AudioPlayer::handle_audio_error(int result, const std::string& operation)
{
    std::string error_msg = get_error_string(result);
    blog(LOG_ERROR, "[AudioPlayer] %s failed: %s (code: %d)", operation.c_str(), error_msg.c_str(), result);
    current_state_ = PlaybackState::ERROR;
}

std::string AudioPlayer::get_error_string(int error_code) const
{
    switch (error_code) {
        case MA_SUCCESS: return "Success";
        case MA_ERROR: return "Generic error";
        case MA_INVALID_ARGS: return "Invalid arguments";
        case MA_INVALID_OPERATION: return "Invalid operation";
        case MA_OUT_OF_MEMORY: return "Out of memory";
        case MA_OUT_OF_RANGE: return "Out of range";
        case MA_ACCESS_DENIED: return "Access denied";
        case MA_DOES_NOT_EXIST: return "File does not exist";
        case MA_INVALID_FILE: return "Invalid file";
        case MA_TOO_BIG: return "File too big";
        case MA_IO_ERROR: return "IO error";
        case MA_TIMEOUT: return "Timeout";
        case MA_NOT_IMPLEMENTED: return "Not implemented";
        default: return "Unknown error";
    }
}

void AudioPlayer::end_callback(void* user_data, ma_sound* sound)
{
    AudioPlayer* player = static_cast<AudioPlayer*>(user_data);
    if (player) player->on_playback_end();
}

void AudioPlayer::on_playback_end()
{
    current_state_ = PlaybackState::STOPPED;
    auto_stop_enabled_ = false;
    blog(LOG_INFO, "[AudioPlayer] Playback ended");
}