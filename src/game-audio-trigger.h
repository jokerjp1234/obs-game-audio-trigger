#pragma once

#include <obs-module.h>
#include <string>
#include <chrono>
#include <memory>

// 前方宣言
class ImageMatcher;
class AudioPlayer;
class ProcessDetector;

// プラグインのデータ構造体
struct game_audio_trigger_data {
    // OBS関連
    obs_source_t *source;
    
    // 設定値
    std::string target_process_name;    // 対象プロセス名
    std::string template_image_path;    // テンプレート画像パス
    std::string audio_file_path;        // 音声ファイルパス
    
    float match_threshold;              // マッチング閾値 (0.0-1.0)
    float audio_volume;                 // 音量 (0.0-1.0)
    float audio_speed;                  // 再生速度 (0.1-3.0)
    float audio_duration;               // 再生時間(秒) (-1で全体)
    int cooldown_ms;                    // クールダウン時間(ミリ秒)
    
    bool is_enabled;                    // 有効/無効
    bool debug_mode;                    // デバッグモード
    
    // 実行時データ
    std::unique_ptr<ImageMatcher> image_matcher;
    std::unique_ptr<AudioPlayer> audio_player;
    std::unique_ptr<ProcessDetector> process_detector;
    
    std::chrono::steady_clock::time_point last_trigger_time;
    bool is_process_running;
    bool is_template_loaded;
    
    // フレーム関連
    uint32_t frame_width;
    uint32_t frame_height;
    gs_texture_t *output_texture;
};

// プラグイン関数の宣言
extern "C" {
    // 基本的なソース関数
    const char *game_audio_trigger_get_name(void *unused);
    void *game_audio_trigger_create(obs_data_t *settings, obs_source_t *source);
    void game_audio_trigger_destroy(void *data);
    void game_audio_trigger_update(void *data, obs_data_t *settings);
    void game_audio_trigger_get_defaults(obs_data_t *settings);
    obs_properties_t *game_audio_trigger_get_properties(void *data);
    
    // ビデオ関連関数
    void game_audio_trigger_video_tick(void *data, float seconds);
    void game_audio_trigger_video_render(void *data, gs_effect_t *effect);
    uint32_t game_audio_trigger_get_width(void *data);
    uint32_t game_audio_trigger_get_height(void *data);
}

// 内部ヘルパー関数
void check_process_and_match(game_audio_trigger_data *context);
void trigger_audio_playback(game_audio_trigger_data *context);
bool is_cooldown_active(game_audio_trigger_data *context);
void log_debug(game_audio_trigger_data *context, const char *format, ...);

// 設定キー定義
#define SETTING_PROCESS_NAME        "process_name"
#define SETTING_TEMPLATE_IMAGE      "template_image"
#define SETTING_AUDIO_FILE          "audio_file"
#define SETTING_MATCH_THRESHOLD     "match_threshold"
#define SETTING_AUDIO_VOLUME        "audio_volume"
#define SETTING_AUDIO_SPEED         "audio_speed"
#define SETTING_AUDIO_DURATION      "audio_duration"
#define SETTING_COOLDOWN_MS         "cooldown_ms"
#define SETTING_ENABLED             "enabled"
#define SETTING_DEBUG_MODE          "debug_mode"

// デフォルト値
#define DEFAULT_MATCH_THRESHOLD     0.8f
#define DEFAULT_AUDIO_VOLUME        1.0f
#define DEFAULT_AUDIO_SPEED         1.0f
#define DEFAULT_AUDIO_DURATION      -1.0f
#define DEFAULT_COOLDOWN_MS         1000
#define DEFAULT_ENABLED             true
#define DEFAULT_DEBUG_MODE          false