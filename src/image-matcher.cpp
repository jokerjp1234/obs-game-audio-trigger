#include "image-matcher.h"
#include <obs-module.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <chrono>
#include <algorithm>

ImageMatcher::ImageMatcher()
    : match_method_(MatchMethod::TEMPLATE_MATCHING)
    , min_scale_(0.8f)
    , max_scale_(1.2f)
    , rotation_tolerance_(5.0f)
    , max_matches_(1)
    , use_grayscale_(true)
    , use_edge_detection_(false)
    , blur_kernel_size_(0)
    , blur_sigma_(0.0)
    , last_processing_time_(0.0)
    , is_template_loaded_(false)
{
    try {
        sift_detector_ = cv::SIFT::create();
        orb_detector_ = cv::ORB::create();
        matcher_ = cv::BFMatcher::create();
    }
    catch (const cv::Exception& e) {
        blog(LOG_WARNING, "[ImageMatcher] Failed to create feature detectors: %s", e.what());
    }
}

ImageMatcher::~ImageMatcher() = default;

bool ImageMatcher::load_template(const std::string& image_path)
{
    if (image_path.empty()) {
        blog(LOG_WARNING, "[ImageMatcher] Empty image path provided");
        return false;
    }

    try {
        cv::Mat loaded_image = cv::imread(image_path, cv::IMREAD_COLOR);
        if (loaded_image.empty()) {
            blog(LOG_ERROR, "[ImageMatcher] Failed to load template image: %s", image_path.c_str());
            return false;
        }
        return load_template(loaded_image);
    }
    catch (const cv::Exception& e) {
        blog(LOG_ERROR, "[ImageMatcher] OpenCV exception while loading template: %s", e.what());
        return false;
    }
}

bool ImageMatcher::load_template(const cv::Mat& template_image)
{
    if (template_image.empty()) {
        blog(LOG_WARNING, "[ImageMatcher] Empty template image provided");
        return false;
    }

    try {
        template_image_ = template_image.clone();
        
        if (template_image_.channels() == 3) {
            cv::cvtColor(template_image_, template_gray_, cv::COLOR_BGR2GRAY);
        } else {
            template_gray_ = template_image_.clone();
        }

        if (use_edge_detection_) {
            cv::Canny(template_gray_, template_edges_, 50, 150);
        }

        if (match_method_ == MatchMethod::FEATURE_MATCHING && sift_detector_) {
            template_keypoints_.clear();
            sift_detector_->detectAndCompute(template_gray_, cv::noArray(), 
                                           template_keypoints_, template_descriptors_);
            
            blog(LOG_INFO, "[ImageMatcher] Extracted %zu keypoints from template", 
                 template_keypoints_.size());
        }

        is_template_loaded_ = true;
        
        blog(LOG_INFO, "[ImageMatcher] Template loaded successfully - Size: %dx%d, Channels: %d",
             template_image_.cols, template_image_.rows, template_image_.channels());
        
        return true;
    }
    catch (const cv::Exception& e) {
        blog(LOG_ERROR, "[ImageMatcher] OpenCV exception while processing template: %s", e.what());
        is_template_loaded_ = false;
        return false;
    }
}

bool ImageMatcher::is_template_loaded() const
{
    return is_template_loaded_ && !template_image_.empty();
}

ImageMatcher::MatchResult ImageMatcher::match(const cv::Mat& target_image, float threshold)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    
    MatchResult result = {};
    result.found = false;
    result.confidence = 0.0f;

    if (!is_template_loaded_ || target_image.empty()) {
        return result;
    }

    if (!validate_images(target_image)) {
        return result;
    }

    try {
        switch (match_method_) {
            case MatchMethod::TEMPLATE_MATCHING:
                result = template_matching(target_image, threshold);
                break;
            case MatchMethod::FEATURE_MATCHING:
                result = feature_matching(target_image, threshold);
                break;
            case MatchMethod::MULTI_SCALE:
                result = multi_scale_matching(target_image, threshold);
                break;
        }

        update_debug_image(target_image, result);
    }
    catch (const cv::Exception& e) {
        blog(LOG_ERROR, "[ImageMatcher] OpenCV exception during matching: %s", e.what());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    last_processing_time_ = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    return result;
}

void ImageMatcher::set_match_method(MatchMethod method)
{
    if (match_method_ != method) {
        match_method_ = method;
        
        if (method == MatchMethod::FEATURE_MATCHING && is_template_loaded_ && sift_detector_) {
            template_keypoints_.clear();
            sift_detector_->detectAndCompute(template_gray_, cv::noArray(),
                                           template_keypoints_, template_descriptors_);
        }
    }
}

void ImageMatcher::set_scale_range(float min_scale, float max_scale)
{
    min_scale_ = std::max(0.1f, min_scale);
    max_scale_ = std::min(5.0f, max_scale);
    
    if (min_scale_ > max_scale_) {
        std::swap(min_scale_, max_scale_);
    }
}

void ImageMatcher::set_rotation_tolerance(float degrees)
{
    rotation_tolerance_ = std::clamp(degrees, 0.0f, 180.0f);
}

void ImageMatcher::set_max_matches(int max_matches)
{
    max_matches_ = std::max(1, max_matches);
}

void ImageMatcher::enable_grayscale_conversion(bool enable)
{
    use_grayscale_ = enable;
}

void ImageMatcher::enable_edge_detection(bool enable)
{
    if (use_edge_detection_ != enable) {
        use_edge_detection_ = enable;
        
        if (enable && is_template_loaded_) {
            cv::Canny(template_gray_, template_edges_, 50, 150);
        }
    }
}

void ImageMatcher::set_gaussian_blur(int kernel_size, double sigma_x)
{
    blur_kernel_size_ = (kernel_size > 0) ? (kernel_size | 1) : 0;
    blur_sigma_ = std::max(0.0, sigma_x);
}

cv::Mat ImageMatcher::get_debug_image() const
{
    return debug_image_.clone();
}

void ImageMatcher::save_debug_image(const std::string& path) const
{
    if (!debug_image_.empty()) {
        cv::imwrite(path, debug_image_);
    }
}

std::vector<ImageMatcher::MatchResult> ImageMatcher::get_all_matches() const
{
    return all_matches_;
}

double ImageMatcher::get_last_processing_time() const
{
    return last_processing_time_;
}

size_t ImageMatcher::get_template_size() const
{
    if (template_image_.empty()) return 0;
    return template_image_.rows * template_image_.cols * template_image_.channels();
}

ImageMatcher::MatchResult ImageMatcher::template_matching(const cv::Mat& target, float threshold)
{
    MatchResult result = {};
    
    cv::Mat target_processed = preprocess_image(target);
    cv::Mat template_processed = use_edge_detection_ ? template_edges_ : 
                                (use_grayscale_ ? template_gray_ : template_image_);

    cv::Mat match_result;
    cv::matchTemplate(target_processed, template_processed, match_result, cv::TM_CCOEFF_NORMED);

    double min_val, max_val;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(match_result, &min_val, &max_val, &min_loc, &max_loc);

    result.confidence = static_cast<float>(max_val);
    result.found = result.confidence >= threshold;

    if (result.found) {
        cv::Size template_size = template_processed.size();
        result.center = cv::Point2f(max_loc.x + template_size.width * 0.5f,
                                   max_loc.y + template_size.height * 0.5f);
        result.bounding_box = cv::Rect(max_loc, template_size);
        result.scale = 1.0f;
        result.rotation = 0.0f;
    }

    return result;
}

ImageMatcher::MatchResult ImageMatcher::feature_matching(const cv::Mat& target, float threshold)
{
    MatchResult result = {};
    
    if (!sift_detector_ || template_keypoints_.empty()) {
        return result;
    }

    cv::Mat target_gray = preprocess_image(target);
    
    std::vector<cv::KeyPoint> target_keypoints;
    cv::Mat target_descriptors;
    
    sift_detector_->detectAndCompute(target_gray, cv::noArray(), target_keypoints, target_descriptors);

    if (target_keypoints.empty() || target_descriptors.empty()) {
        return result;
    }

    std::vector<std::vector<cv::DMatch>> knn_matches;
    matcher_->knnMatch(template_descriptors_, target_descriptors, knn_matches, 2);

    std::vector<cv::DMatch> good_matches;
    const float ratio_threshold = 0.7f;
    
    for (const auto& match_pair : knn_matches) {
        if (match_pair.size() == 2 && 
            match_pair[0].distance < ratio_threshold * match_pair[1].distance) {
            good_matches.push_back(match_pair[0]);
        }
    }

    const int min_matches = 10;
    if (good_matches.size() < min_matches) {
        return result;
    }

    result.confidence = static_cast<float>(good_matches.size()) / 
                       static_cast<float>(template_keypoints_.size());
    result.found = result.confidence >= threshold;

    if (result.found) {
        cv::Point2f center(0, 0);
        for (const auto& match : good_matches) {
            center += target_keypoints[match.trainIdx].pt;
        }
        center /= static_cast<float>(good_matches.size());
        result.center = center;
        
        cv::Size template_size = template_image_.size();
        result.bounding_box = calculate_bounding_box(cv::Point(center), template_size);
        result.scale = 1.0f;
        result.rotation = 0.0f;
    }

    return result;
}

ImageMatcher::MatchResult ImageMatcher::multi_scale_matching(const cv::Mat& target, float threshold)
{
    MatchResult best_result = {};
    
    cv::Mat target_processed = preprocess_image(target);
    cv::Mat template_processed = use_grayscale_ ? template_gray_ : template_image_;

    const int scale_steps = 5;
    for (int i = 0; i < scale_steps; ++i) {
        float scale = min_scale_ + (max_scale_ - min_scale_) * i / (scale_steps - 1);
        
        cv::Mat scaled_template;
        cv::resize(template_processed, scaled_template, cv::Size(), scale, scale);

        if (scaled_template.cols > target_processed.cols || 
            scaled_template.rows > target_processed.rows) {
            continue;
        }

        cv::Mat match_result;
        cv::matchTemplate(target_processed, scaled_template, match_result, cv::TM_CCOEFF_NORMED);

        double min_val, max_val;
        cv::Point min_loc, max_loc;
        cv::minMaxLoc(match_result, &min_val, &max_val, &min_loc, &max_loc);

        if (max_val > best_result.confidence) {
            best_result.confidence = static_cast<float>(max_val);
            best_result.found = best_result.confidence >= threshold;
            
            if (best_result.found) {
                cv::Size template_size = scaled_template.size();
                best_result.center = cv::Point2f(max_loc.x + template_size.width * 0.5f,
                                               max_loc.y + template_size.height * 0.5f);
                best_result.bounding_box = cv::Rect(max_loc, template_size);
                best_result.scale = scale;
                best_result.rotation = 0.0f;
            }
        }
    }

    return best_result;
}

cv::Mat ImageMatcher::preprocess_image(const cv::Mat& image) const
{
    cv::Mat processed = image.clone();

    if (use_grayscale_ && processed.channels() > 1) {
        cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
    }

    apply_filters(processed);

    if (use_edge_detection_ && processed.channels() == 1) {
        cv::Canny(processed, processed, 50, 150);
    }

    return processed;
}

void ImageMatcher::apply_filters(cv::Mat& image) const
{
    if (blur_kernel_size_ > 0) {
        cv::GaussianBlur(image, image, cv::Size(blur_kernel_size_, blur_kernel_size_), 
                        blur_sigma_, blur_sigma_);
    }
}

float ImageMatcher::calculate_confidence(const cv::Mat& match_result, cv::Point max_loc) const
{
    double min_val, max_val;
    cv::minMaxLoc(match_result, &min_val, &max_val);
    return static_cast<float>((max_val - min_val) / (1.0 - min_val));
}

cv::Rect ImageMatcher::calculate_bounding_box(cv::Point center, cv::Size template_size, float scale) const
{
    cv::Size scaled_size(static_cast<int>(template_size.width * scale),
                        static_cast<int>(template_size.height * scale));
    
    cv::Point top_left(center.x - scaled_size.width / 2,
                      center.y - scaled_size.height / 2);
    
    return cv::Rect(top_left, scaled_size);
}

bool ImageMatcher::validate_images(const cv::Mat& target) const
{
    if (template_image_.empty() || target.empty()) {
        blog(LOG_WARNING, "[ImageMatcher] Empty template or target image");
        return false;
    }

    if (template_image_.cols > target.cols || template_image_.rows > target.rows) {
        blog(LOG_WARNING, "[ImageMatcher] Template larger than target image");
        return false;
    }

    const int min_size = 10;
    if (template_image_.cols < min_size || template_image_.rows < min_size ||
        target.cols < min_size || target.rows < min_size) {
        blog(LOG_WARNING, "[ImageMatcher] Image too small for reliable matching");
        return false;
    }

    return true;
}

void ImageMatcher::update_debug_image(const cv::Mat& target, const MatchResult& result)
{
    if (target.empty()) return;

    if (target.channels() == 1) {
        cv::cvtColor(target, debug_image_, cv::COLOR_GRAY2BGR);
    } else {
        debug_image_ = target.clone();
    }

    if (result.found) {
        cv::rectangle(debug_image_, result.bounding_box, cv::Scalar(0, 255, 0), 2);
        cv::circle(debug_image_, result.center, 5, cv::Scalar(0, 0, 255), -1);
        
        std::string confidence_text = "Confidence: " + std::to_string(result.confidence);
        cv::putText(debug_image_, confidence_text, 
                   cv::Point(result.bounding_box.x, result.bounding_box.y - 10),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    }

    all_matches_.clear();
    if (result.found) {
        all_matches_.push_back(result);
    }
}