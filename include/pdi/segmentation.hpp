#pragma once

#include <cstdint>
#include <opencv2/core/mat.hpp>
#include <string>
#include <string_view>

namespace pdi {

enum class SegmentationMethod {
    fixed,
    otsu,
    adaptive,
    hsv
};

struct SegmentationParams {
    SegmentationMethod method = SegmentationMethod::fixed;
    int threshold = 128;       // Limiar fixo [0..255]
    bool invert = false;       // Inverte mascara quando objeto e escuro
    int blur_size = 0;         // Tamanho do filtro Gaussiano (0 = sem filtro, ou impar >= 3)
    int block_size = 11;       // Vizinhanca da limiarizacao adaptativa (impar >= 3)
    double c_constant = 2.0;   // Constante subtraida na limiarizacao adaptativa
    int h_min = 0;             // HSV Hue minimo [0..179] (suporta wrap-around quando h_min > h_max)
    int h_max = 179;           // HSV Hue maximo [0..179]
    int s_min = 0;             // HSV Saturation minimo [0..255]
    int s_max = 255;           // HSV Saturation maximo [0..255]
    int v_min = 0;             // HSV Value minimo [0..255]
    int v_max = 255;           // HSV Value maximo [0..255]
};

struct SegmentationResult {
    cv::Mat mask;                         // Mascara binaria 1 canal (CV_8UC1) com valores em {0, 255}
    cv::Mat overlay;                      // Visualizacao sobreposta (CV_8UC3) para inspecao de erros
    std::uint64_t foreground_pixels = 0;  // Contagem de pixels 255 (objeto)
    std::uint64_t total_pixels = 0;       // Total de pixels (rows * cols)
    double foreground_ratio = 0.0;        // Proporcao de primeiro plano [0.0..1.0]
    double effective_threshold = 0.0;     // Limiar efetivo (fixo ou calculado por Otsu)
};

SegmentationMethod parseSegmentationMethod(std::string_view text);
std::string segmentationMethodToString(SegmentationMethod method);

// Valida programaticamente que a mascara mantem dimensoes esperadas, 1 canal (CV_8UC1)
// e possui exclusivamente valores no conjunto {0, 255}.
bool isValidBinaryMask(const cv::Mat& mask, int expected_rows = -1, int expected_cols = -1);

// Garante/converte uma imagem binaria lida do disco para CV_8UC1 estrita em {0, 255},
// lancando PdiError(ExitCode::invalid_parameter) caso contenha valores fora de {0, 255}.
cv::Mat ensureBinaryMask(const cv::Mat& input);

// Cria visualizacao sobrepondo a mascara e seu contorno sobre a imagem original.
cv::Mat createSegmentationOverlay(const cv::Mat& src, const cv::Mat& mask);

// Executa a segmentacao conforme o metodo e parametros especificados (M2.1).
SegmentationResult segmentImage(const cv::Mat& src, const SegmentationParams& params);

} // namespace pdi
