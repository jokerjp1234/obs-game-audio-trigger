#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

/**
 * 画像マッチングクラス
 * テンプレートマッチングを使用してゲーム画面内の特定画像を検出
 */
class ImageMatcher {
public:
    enum class MatchMethod {
        TEMPLATE_MATCHING,      // テンプレートマッチング（高速）
        FEATURE_MATCHING,       // 特徴点マッチング（回転・スケールに対応）
        MULTI_SCALE            // マルチスケールマッチング
    };
    
    struct MatchResult {
        bool found;
        float confidence;       // 信頼度 (0.0-1.0)
        cv::Point2f center;     // マッチした中心座標
        cv::Rect bounding_box;  // バウンディングボックス
        float scale;           // 検出されたスケール
        float rotation;        // 検出された回転角度（度）
    };

public:
    ImageMatcher();
    ~ImageMatcher();
    
    // テンプレート画像の設定
    bool load_template(const std::string& image_path);
    bool load_template(const cv::Mat& template_image);
    bool is_template_loaded() const;
    
    // マッチング実行
    MatchResult match(const cv::Mat& target_image, float threshold = 0.8f);
    
    // 設定
    void set_match_method(MatchMethod method);
    void set_scale_range(float min_scale, float max_scale);
    void set_rotation_tolerance(float degrees);
    void set_max_matches(int max_matches);
    
    // 前処理設定
    void enable_grayscale_conversion(bool enable);
    void enable_edge_detection(bool enable);
    void set_gaussian_blur(int kernel_size, double sigma_x = 0);
    
    // デバッグ用
    cv::Mat get_debug_image() const;
    void save_debug_image(const std::string& path) const;
    std::vector<MatchResult> get_all_matches() const;
    
    // 統計情報
    double get_last_processing_time() const;
    size_t get_template_size() const;

private:
    // マッチング手法の実装
    MatchResult template_matching(const cv::Mat& target, float threshold);
    MatchResult feature_matching(const cv::Mat& target, float threshold);
    MatchResult multi_scale_matching(const cv::Mat& target, float threshold);
    
    // 前処理
    cv::Mat preprocess_image(const cv::Mat& image) const;
    void apply_filters(cv::Mat& image) const;
    
    // 後処理
    float calculate_confidence(const cv::Mat& match_result, cv::Point max_loc) const;
    cv::Rect calculate_bounding_box(cv::Point center, cv::Size template_size, float scale = 1.0f) const;
    
    // ヘルパー関数
    bool validate_images(const cv::Mat& target) const;
    void update_debug_image(const cv::Mat& target, const MatchResult& result);

private:
    // テンプレート画像
    cv::Mat template_image_;
    cv::Mat template_gray_;
    cv::Mat template_edges_;
    
    // 特徴点検出器（SIFT/ORB用）
    cv::Ptr<cv::SIFT> sift_detector_;
    cv::Ptr<cv::ORB> orb_detector_;
    cv::Ptr<cv::BFMatcher> matcher_;
    
    // テンプレートの特徴点
    std::vector<cv::KeyPoint> template_keypoints_;
    cv::Mat template_descriptors_;
    
    // 設定
    MatchMethod match_method_;
    float min_scale_;
    float max_scale_;
    float rotation_tolerance_;
    int max_matches_;
    
    // 前処理設定
    bool use_grayscale_;
    bool use_edge_detection_;
    int blur_kernel_size_;
    double blur_sigma_;
    
    // デバッグ用
    cv::Mat debug_image_;
    std::vector<MatchResult> all_matches_;
    
    // パフォーマンス測定
    double last_processing_time_;
    
    // 状態
    bool is_template_loaded_;
};