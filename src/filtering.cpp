#include "pdi/filtering.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace pdi {

namespace {

bool isGrayscaleImage(const cv::Mat& img) {
    return !img.empty() && img.channels() == 1;
}

// Convolucao pixel a pixel com tratamento explicito de bordas
double convolvePixel(const cv::Mat& src, const Kernel& kernel, BorderStrategy border, int y, int x, bool& isBorderPixel) {
    int rows = src.rows;
    int cols = src.cols;
    int kSize = static_cast<int>(kernel.size());
    int radius = static_cast<int>(kernel.radius());

    isBorderPixel = (y - radius < 0 || y + radius >= rows || x - radius < 0 || x + radius >= cols);

    if (isBorderPixel && border == BorderStrategy::copy) {
        return static_cast<double>(src.at<uchar>(y, x));
    }

    double sum = 0.0;
    for (int ky = 0; ky < kSize; ++ky) {
        for (int kx = 0; kx < kSize; ++kx) {
            int ny = y + ky - radius;
            int nx = x + kx - radius;

            if (border == BorderStrategy::replicate) {
                ny = std::clamp(ny, 0, rows - 1);
                nx = std::clamp(nx, 0, cols - 1);
            }

            double pixelVal = static_cast<double>(src.at<uchar>(ny, nx));
            double weight = kernel.at(static_cast<std::size_t>(ky), static_cast<std::size_t>(kx));
            sum += pixelVal * weight;
        }
    }
    return sum;
}

} // namespace

// 1. Convolucao generica
cv::Mat applyConvolution(const cv::Mat& src, const Kernel& kernel, BorderStrategy border) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza (1 canal).");
    }

    cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1);

    for (int y = 0; y < src.rows; ++y) {
        for (int x = 0; x < src.cols; ++x) {
            bool isBorder = false;
            double val = convolvePixel(src, kernel, border, y, x, isBorder);

            if (isBorder && border == BorderStrategy::copy) {
                dst.at<uchar>(y, x) = src.at<uchar>(y, x);
            } else {
                dst.at<uchar>(y, x) = static_cast<uchar>(std::clamp(std::round(val), 0.0, 255.0));
            }
        }
    }

    return dst;
}

// 2. Filtro de media (3x3 ou 5x5)
cv::Mat meanFilter(const cv::Mat& src, int size, BorderStrategy border) {
    if (size != 3 && size != 5) {
        throw std::invalid_argument("Tamanho invalido para mean_filter (use 3 ou 5).");
    }

    double weight = 1.0 / static_cast<double>(size * size);
    std::vector<double> values(static_cast<std::size_t>(size * size), weight);
    Kernel meanKernel(static_cast<std::size_t>(size), values);

    return applyConvolution(src, meanKernel, border);
}

// 3. Filtro de media ponderada (3x3)
cv::Mat weightedMeanFilter(const cv::Mat& src, BorderStrategy border) {
    std::vector<double> values = {
        1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
        2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
        1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
    };
    Kernel weightedKernel(3, values);

    return applyConvolution(src, weightedKernel, border);
}

// 4. Laplaciano e realce de bordas
cv::Mat laplacianFilter(const cv::Mat& src, BorderStrategy border) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza (1 canal).");
    }

    // Kernel Laplaciano com centro -4
    std::vector<double> values = {
         0.0,  1.0,  0.0,
         1.0, -4.0,  1.0,
         0.0,  1.0,  0.0
    };
    Kernel lapKernel(3, values);

    cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1);

    for (int y = 0; y < src.rows; ++y) {
        for (int x = 0; x < src.cols; ++x) {
            bool isBorder = false;
            double lap = convolvePixel(src, lapKernel, border, y, x, isBorder);

            if (isBorder && border == BorderStrategy::copy) {
                dst.at<uchar>(y, x) = src.at<uchar>(y, x);
            } else {
                // Realce subtraindo o Laplaciano (pois o centro eh negativo -4)
                double orig = static_cast<double>(src.at<uchar>(y, x));
                double sharpened = orig - lap;
                dst.at<uchar>(y, x) = static_cast<uchar>(std::clamp(std::round(sharpened), 0.0, 255.0));
            }
        }
    }

    return dst;
}

// 5. Filtro de Sobel (magnitude de bordas)
cv::Mat sobelFilter(const cv::Mat& src, BorderStrategy border) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza (1 canal).");
    }

    // Gradiente horizontal (Gx)
    std::vector<double> gxValues = {
        -1.0, 0.0, 1.0,
        -2.0, 0.0, 2.0,
        -1.0, 0.0, 1.0
    };
    Kernel gxKernel(3, gxValues);

    // Gradiente vertical (Gy)
    std::vector<double> gyValues = {
        -1.0, -2.0, -1.0,
         0.0,  0.0,  0.0,
         1.0,  2.0,  1.0
    };
    Kernel gyKernel(3, gyValues);

    cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1);

    for (int y = 0; y < src.rows; ++y) {
        for (int x = 0; x < src.cols; ++x) {
            bool isBorder = false;
            double gx = convolvePixel(src, gxKernel, border, y, x, isBorder);
            double gy = convolvePixel(src, gyKernel, border, y, x, isBorder);

            if (isBorder && border == BorderStrategy::copy) {
                dst.at<uchar>(y, x) = src.at<uchar>(y, x);
            } else {
                // Magnitude euclidiana do gradiente
                double mag = std::sqrt(gx * gx + gy * gy);
                dst.at<uchar>(y, x) = static_cast<uchar>(std::clamp(std::round(mag), 0.0, 255.0));
            }
        }
    }

    return dst;
}

} // namespace pdi
