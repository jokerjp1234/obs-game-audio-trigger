#include "game-audio-trigger.h"
#include "image-matcher.h"
#include "audio-player.h"
#include "process-detector.h"
#include <obs-module.h>
#include <util/platform.h>
#include <cstdarg>

// ソース名の取得
const char *game_audio_trigger_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return obs_module_text("GameAudioTrigger");
}

// ソースの作成
void *game_audio_trigger_create(obs_data_t *settings, obs_source_t *source)
{
    auto *context = new game_audio_trigger_data();
    if (!context) {
        blog(LOG_ERROR, "[Game Audio Trigger] Failed to allocate context");
        return nullptr;
    }

    // 基本初期化
    context->source = source;
    context->frame_width = 1920;
    context->frame_height = 1080;
    context->output_texture = nullptr;
    context->is_process_running = false;
    context->is_template_loaded = false;
    context->last_trigger_time = std::chrono::steady_clock::now();

    // コンポーネントの初期化
    try {
        context->image_matcher = std::make_unique<ImageMatcher>();
        context->audio_player = std::make_unique<AudioPlayer>();
        context->process_detector = std::make_unique<ProcessDetector>();

        if (!context->audio_player->initialize()) {
            blog(LOG_WARNING, "[Game Audio Trigger] Failed to initialize audio player");
        }
    }
    catch (const std::exception& e) {
        blog(LOG_ERROR, "[Game Audio Trigger] Exception during initialization: %s", e.what());
        delete context;
        return nullptr;
    }

    // 設定の適用
    game_audio_trigger_update(context, settings);
    
    blog(LOG_INFO, "[Game Audio Trigger] Source created successfully");
    return context;
}

// ソースの破棄
void game_audio_trigger_destroy(void *data)
{
    auto *context = static_cast<game_audio_trigger_data*>(data);
    if (!context) return;

    // OpenGLテクスチャの解放
    obs_enter_graphics();
    if (context->output_texture) {
        gs_texture_destroy(context->output_texture);
        context->output_texture = nullptr;
    }
    obs_leave_graphics();

    // オーディオプレイヤーの停止
    if (context->audio_player) {
        context->audio_player->stop();
        context->audio_player->shutdown();
    }

    // コンポーネントの破棄
    context->image_matcher.reset();
    context->audio_player.reset();
    context->process_detector.reset();

    blog(LOG_INFO, "[Game Audio Trigger] Source destroyed");
    delete context;
}

// 設定の更新
void game_audio_trigger_update(void *data, obs_data_t *settings)
{
    auto *context = static_cast<game_audio_trigger_data*>(data);
    if (!context) return;

    // 設定値の読み込み
    context->target_process_name = obs_data_get_string(settings, SETTING_PROCESS_NAME);
    context->template_image_path = obs_data_get_string(settings, SETTING_TEMPLATE_IMAGE);
    context->audio_file_path = obs_data_get_string(settings, SETTING_AUDIO_FILE);
    
    context->match_threshold = static_cast<float>(obs_data_get_double(settings, SETTING_MATCH_THRESHOLD));
    context->audio_volume = static_cast<float>(obs_data_get_double(settings, SETTING_AUDIO_VOLUME));
    context->audio_speed = static_cast<float>(obs_data_get_double(settings, SETTING_AUDIO_SPEED));
    context->audio_duration = static_cast<float>(obs_data_get_double(settings, SETTING_AUDIO_DURATION));
    context->cooldown_ms = static_cast<int>(obs_data_get_int(settings, SETTING_COOLDOWN_MS));
    
    context->is_enabled = obs_data_get_bool(settings, SETTING_ENABLED);
    context->debug_mode = obs_data_get_bool(settings, SETTING_DEBUG_MODE);

    // プロセス設定の更新
    if (!context->target_process_name.empty() && context->process_detector) {
        context->process_detector->set_target_process(context->target_process_name);
        log_debug(context, "Target process set to: %s", context->target_process_name.c_str());
    }

    // テンプレート画像の読み込み
    if (!context->template_image_path.empty() && context->image_matcher) {
        if (context->image_matcher->load_template(context->template_image_path)) {
            context->is_template_loaded = true;
            log_debug(context, "Template image loaded: %s", context->template_image_path.c_str());
        } else {
            context->is_template_loaded = false;
            blog(LOG_WARNING, "[Game Audio Trigger] Failed to load template image: %s", 
                 context->template_image_path.c_str());
        }
    }

    // オーディオファイルの読み込み
    if (!context->audio_file_path.empty() && context->audio_player) {
        if (context->audio_player->load_audio_file(context->audio_file_path)) {
            context->audio_player->set_volume(context->audio_volume);
            context->audio_player->set_speed(context->audio_speed);
            log_debug(context, "Audio file loaded: %s", context->audio_file_path.c_str());
        } else {
            blog(LOG_WARNING, "[Game Audio Trigger] Failed to load audio file: %s",
                 context->audio_file_path.c_str());
        }
    }

    log_debug(context, "Settings updated - Enabled: %s, Threshold: %.2f, Volume: %.2f",
              context->is_enabled ? "true" : "false",
              context->match_threshold,
              context->audio_volume);
}

// デフォルト設定
void game_audio_trigger_get_defaults(obs_data_t *settings)
{
    obs_data_set_string(settings, SETTING_PROCESS_NAME, "");
    obs_data_set_string(settings, SETTING_TEMPLATE_IMAGE, "");
    obs_data_set_string(settings, SETTING_AUDIO_FILE, "");
    
    obs_data_set_double(settings, SETTING_MATCH_THRESHOLD, DEFAULT_MATCH_THRESHOLD);
    obs_data_set_double(settings, SETTING_AUDIO_VOLUME, DEFAULT_AUDIO_VOLUME);
    obs_data_set_double(settings, SETTING_AUDIO_SPEED, DEFAULT_AUDIO_SPEED);
    obs_data_set_double(settings, SETTING_AUDIO_DURATION, DEFAULT_AUDIO_DURATION);
    obs_data_set_int(settings, SETTING_COOLDOWN_MS, DEFAULT_COOLDOWN_MS);
    
    obs_data_set_bool(settings, SETTING_ENABLED, DEFAULT_ENABLED);
    obs_data_set_bool(settings, SETTING_DEBUG_MODE, DEFAULT_DEBUG_MODE);
}

// プロパティの取得（UI設定画面）
obs_properties_t *game_audio_trigger_get_properties(void *data)
{
    UNUSED_PARAMETER(data);
    
    obs_properties_t *props = obs_properties_create();

    // 基本設定グループ
    obs_property_t *group_basic = obs_properties_add_group(props, "basic_group", 
                                                          obs_module_text("BasicSettings"), 
                                                          OBS_GROUP_NORMAL, nullptr);
    obs_properties_t *basic_props = obs_property_group_content(group_basic);

    // 有効/無効
    obs_properties_add_bool(basic_props, SETTING_ENABLED, obs_module_text("Enabled"));

    // プロセス名
    obs_properties_add_text(basic_props, SETTING_PROCESS_NAME, 
                           obs_module_text("ProcessName"), OBS_TEXT_DEFAULT);

    // テンプレート画像
    obs_properties_add_path(basic_props, SETTING_TEMPLATE_IMAGE, 
                           obs_module_text("TemplateImage"), OBS_PATH_FILE, 
                           "Image files (*.png *.jpg *.jpeg *.bmp);;All files (*.*)");

    // オーディオファイル
    obs_properties_add_path(basic_props, SETTING_AUDIO_FILE, 
                           obs_module_text("AudioFile"), OBS_PATH_FILE, 
                           "Audio files (*.wav *.mp3 *.ogg);;All files (*.*)");

    // マッチング設定グループ
    obs_property_t *group_matching = obs_properties_add_group(props, "matching_group", 
                                                             obs_module_text("MatchingSettings"), 
                                                             OBS_GROUP_NORMAL, nullptr);
    obs_properties_t *matching_props = obs_property_group_content(group_matching);

    // マッチング閾値
    obs_property_t *threshold_prop = obs_properties_add_float_slider(matching_props, SETTING_MATCH_THRESHOLD,
                                                                   obs_module_text("MatchThreshold"), 
                                                                   0.0, 1.0, 0.01);

    // クールダウン時間
    obs_properties_add_int(matching_props, SETTING_COOLDOWN_MS, 
                          obs_module_text("CooldownMs"), 0, 10000, 100);

    // オーディオ設定グループ
    obs_property_t *group_audio = obs_properties_add_group(props, "audio_group", 
                                                          obs_module_text("AudioSettings"), 
                                                          OBS_GROUP_NORMAL, nullptr);
    obs_properties_t *audio_props = obs_property_group_content(group_audio);

    // 音量
    obs_properties_add_float_slider(audio_props, SETTING_AUDIO_VOLUME,
                                   obs_module_text("Volume"), 0.0, 1.0, 0.01);

    // 再生速度
    obs_properties_add_float_slider(audio_props, SETTING_AUDIO_SPEED,
                                   obs_module_text("Speed"), 0.1, 3.0, 0.1);

    // 再生時間
    obs_properties_add_float(audio_props, SETTING_AUDIO_DURATION,
                            obs_module_text("Duration"), -1.0, 300.0, 0.1);

    // デバッグ設定
    obs_properties_add_bool(props, SETTING_DEBUG_MODE, obs_module_text("DebugMode"));

    return props;
}

// ビデオティック（メインループ処理）
void game_audio_trigger_video_tick(void *data, float seconds)
{
    UNUSED_PARAMETER(seconds);
    auto *context = static_cast<game_audio_trigger_data*>(data);
    if (!context || !context->is_enabled) return;

    check_process_and_match(context);
}

// ビデオレンダリング（表示用）
void game_audio_trigger_video_render(void *data, gs_effect_t *effect)
{
    UNUSED_PARAMETER(effect);
    auto *context = static_cast<game_audio_trigger_data*>(data);
    if (!context) return;

    // デバッグモードの場合、マッチング結果を可視化
    if (context->debug_mode && context->output_texture) {
        gs_effect_t *default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
        gs_technique_t *tech = gs_effect_get_technique(default_effect, "Draw");
        
        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);
        
        gs_effect_set_texture(gs_effect_get_param_by_name(default_effect, "image"), 
                             context->output_texture);
        
        gs_draw_sprite(context->output_texture, 0, context->frame_width, context->frame_height);
        
        gs_technique_end_pass(tech);
        gs_technique_end(tech);
    }
}

// 幅の取得
uint32_t game_audio_trigger_get_width(void *data)
{
    auto *context = static_cast<game_audio_trigger_data*>(data);
    return context ? context->frame_width : 0;
}

// 高さの取得
uint32_t game_audio_trigger_get_height(void *data)
{
    auto *context = static_cast<game_audio_trigger_data*>(data);
    return context ? context->frame_height : 0;
}

// プロセス検出とマッチング処理
void check_process_and_match(game_audio_trigger_data *context)
{
    if (!context || !context->process_detector || !context->image_matcher) return;

    // プロセスの状態を更新
    bool process_running = context->process_detector->is_process_running();
    if (!process_running) {
        context->process_detector->refresh_process_info();
        process_running = context->process_detector->is_process_running();
    }

    context->is_process_running = process_running;

    if (!process_running || !context->is_template_loaded) {
        return;
    }

    // クールダウン確認
    if (is_cooldown_active(context)) {
        return;
    }

    // ウィンドウキャプチャ
    cv::Mat captured_image;
    if (!context->process_detector->capture_window(captured_image)) {
        log_debug(context, "Failed to capture window");
        return;
    }

    if (captured_image.empty()) {
        return;
    }

    // 画像マッチング実行
    auto match_result = context->image_matcher->match(captured_image, context->match_threshold);
    
    if (match_result.found) {
        log_debug(context, "Match found! Confidence: %.3f at (%.1f, %.1f)", 
                 match_result.confidence, match_result.center.x, match_result.center.y);
        trigger_audio_playback(context);
    }
}

// オーディオ再生トリガー
void trigger_audio_playback(game_audio_trigger_data *context)
{
    if (!context || !context->audio_player) return;

    // クールダウン時間を更新
    context->last_trigger_time = std::chrono::steady_clock::now();

    // オーディオ再生
    bool play_result = false;
    if (context->audio_duration > 0) {
        play_result = context->audio_player->play_with_duration(context->audio_duration);
    } else {
        play_result = context->audio_player->play();
    }

    if (play_result) {
        log_debug(context, "Audio playback triggered successfully");
    } else {
        blog(LOG_WARNING, "[Game Audio Trigger] Failed to play audio");
    }
}

// クールダウン状態確認
bool is_cooldown_active(game_audio_trigger_data *context)
{
    if (!context || context->cooldown_ms <= 0) return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - context->last_trigger_time).count();
    
    return elapsed < context->cooldown_ms;
}

// デバッグログ出力
void log_debug(game_audio_trigger_data *context, const char *format, ...)
{
    if (!context || !context->debug_mode) return;

    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    blog(LOG_INFO, "[Game Audio Trigger Debug] %s", buffer);
    
    va_end(args);
}