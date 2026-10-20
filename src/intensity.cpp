//
// Created by gabri on 25/08/2026.
//
#include "pdi/intensity.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace pdi {

bool isGrayscaleImage(const cv::Mat& img) {
    return !img.empty() && img.channels() == 1 && img.depth() == CV_8U;
}

// 1. Ajuste de brilho: g(x,y) = f(x,y) + b
cv::Mat adjustBrightness(const cv::Mat& src, int value) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza.");
    }

    cv::Mat dst = cv::Mat::zeros(src.size(), src.type());

    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            int soma = src.at<uchar>(i, j) + value;
            dst.at<uchar>(i, j) = static_cast<uchar>(std::clamp(soma, 0, 255));
        }
    }

    return dst;
}

// 2. Ajuste de contraste: g(x,y) = alpha * (f(x,y) - 128) + 128
cv::Mat adjustContrast(const cv::Mat& src, double alpha) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza.");
    }

    cv::Mat dst = cv::Mat::zeros(src.size(), src.type());

    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            double valor = std::round(alpha * (static_cast<double>(src.at<uchar>(i, j)) - 128.0) + 128.0);
            dst.at<uchar>(i, j) = static_cast<uchar>(std::clamp(valor, 0.0, 255.0));
        }
    }

    return dst;
}

// 3. Negativo: g(x,y) = 255 - f(x,y)
cv::Mat invertNegative(const cv::Mat& src) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza.");
    }

    cv::Mat dst = cv::Mat::zeros(src.size(), src.type());

    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            int valor = 255 - src.at<uchar>(i, j);
            dst.at<uchar>(i, j) = static_cast<uchar>(valor);
        }
    }
    return dst;
}

// 4. Limiarização binária: 0 se < T, 255 se >= T
cv::Mat applyThreshold(const cv::Mat& src, int threshold) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza.");
    }

    cv::Mat dst = cv::Mat::zeros(src.size(), src.type());

    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            if (src.at<uchar>(i, j) >= threshold) {
                dst.at<uchar>(i, j) = 255;
            } else {
                dst.at<uchar>(i, j) = 0;
            }
        }
    }
    return dst;
}

// 5. Histograma manual: vetor de 256 posições
std::array<std::uint64_t, 256> computeHistogram(const cv::Mat& src) {
    if (!isGrayscaleImage(src)) {
        throw std::invalid_argument("A imagem deve ser 8-bit em escala de cinza.");
    }
    std::array<std::uint64_t, 256> hist {}; // inicializa tudo com 0
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            uchar intensity = src.at<uchar>(i, j);
            hist[intensity]++;
        }
    }
    return hist;
}

}
