#pragma once
#include <opencv2/core/mat.hpp>
#include <string>
namespace pdi { cv::Mat read_image(const std::string& path); void write_image(const std::string& path,const cv::Mat& image); }
