#pragma once
#include <opencv2/core/mat.hpp>
#include <array>
#include <cstdint>

namespace pdi {

    bool isGrayscaleImage(const cv::Mat& img);

    cv::Mat adjustBrightness(const cv::Mat& src, int value);

    cv::Mat adjustContrast(const cv::Mat& src, double alpha);

    cv::Mat invertNegative(const cv::Mat& src);

    cv::Mat applyThreshold(const cv::Mat& src, int threshold);

    std::array<std::uint64_t, 256> computeHistogram(const cv::Mat& src);
}
