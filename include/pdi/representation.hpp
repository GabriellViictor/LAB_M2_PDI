#pragma once
#include <opencv2/core/mat.hpp>

namespace pdi {
    void inspectImage(const cv::Mat& img);
    cv::Mat copyImage(const cv::Mat& src);
    cv::Mat extractChannel(const cv::Mat& src, int channelIndex);
    cv::Mat grayScaleAvg(const cv::Mat& src);
    cv::Mat grayscaleWeighted(const cv::Mat& src);
    cv::Mat quantImg(const cv::Mat& src, int level);

}
