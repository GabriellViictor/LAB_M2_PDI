#pragma once

#include "pdi/kernel.hpp"
#include "pdi/parameters.hpp"
#include <opencv2/core/mat.hpp>

namespace pdi {

// 1. Convolucao generica
cv::Mat applyConvolution(const cv::Mat& src, const Kernel& kernel, BorderStrategy border);

// 2. Filtro de media (size 3 ou 5)
cv::Mat meanFilter(const cv::Mat& src, int size, BorderStrategy border);

// 3. Filtro de media ponderada (3x3 gaussiano)
cv::Mat weightedMeanFilter(const cv::Mat& src, BorderStrategy border);

// 4. Laplaciano e realce
cv::Mat laplacianFilter(const cv::Mat& src, BorderStrategy border);

// 5. Filtro de Sobel (magnitude de bordas)
cv::Mat sobelFilter(const cv::Mat& src, BorderStrategy border);

} // namespace pdi
