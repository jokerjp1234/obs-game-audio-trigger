#include <obs-module.h>
#include <obs-frontend-api.h>
#include "game-audio-trigger.h"

// プラグイン情報の定義
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-game-audio-trigger", "en-US")

// モジュール名
MODULE_EXPORT const char *obs_module_description(void)
{
    return "Game Audio Trigger Plugin - Plays audio when specific game screen is detected";
}

// プラグインロード時の処理
bool obs_module_load(void)
{
    blog(LOG_INFO, "[Game Audio Trigger] Plugin loaded successfully");
    
    // ソースタイプの登録
    obs_source_info game_audio_trigger_info = {};
    game_audio_trigger_info.id = "game_audio_trigger";
    game_audio_trigger_info.type = OBS_SOURCE_TYPE_INPUT;
    game_audio_trigger_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    
    // コールバック関数の設定
    game_audio_trigger_info.get_name = game_audio_trigger_get_name;
    game_audio_trigger_info.create = game_audio_trigger_create;
    game_audio_trigger_info.destroy = game_audio_trigger_destroy;
    game_audio_trigger_info.update = game_audio_trigger_update;
    game_audio_trigger_info.get_defaults = game_audio_trigger_get_defaults;
    game_audio_trigger_info.get_properties = game_audio_trigger_get_properties;
    game_audio_trigger_info.video_tick = game_audio_trigger_video_tick;
    game_audio_trigger_info.video_render = game_audio_trigger_video_render;
    game_audio_trigger_info.get_width = game_audio_trigger_get_width;
    game_audio_trigger_info.get_height = game_audio_trigger_get_height;
    
    // ソースの登録
    obs_register_source(&game_audio_trigger_info);
    
    return true;
}

// プラグインアンロード時の処理
void obs_module_unload(void)
{
    blog(LOG_INFO, "[Game Audio Trigger] Plugin unloaded");
}

// モジュール設定のロード
void obs_module_set_config_dir(const char *config_dir)
{
    UNUSED_PARAMETER(config_dir);
}

// 必要なOBSバージョンの指定
uint32_t obs_module_ver(void)
{
    return LIBOBS_API_VER;
}