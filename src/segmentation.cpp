#include "pdi/segmentation.hpp"
#include "pdi/errors.hpp"

#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <vector>

namespace pdi {
namespace {

cv::Mat toGrayscale8U(const cv::Mat& src)
{
    if (src.empty()) {
        throw PdiError(ExitCode::read_error, "Imagem de entrada vazia na segmentacao.");
    }

    cv::Mat src8u;
    if (src.depth() != CV_8U) {
        src.convertTo(src8u, CV_8U);
    } else {
        src8u = src;
    }

    if (src8u.channels() == 1) {
        return src8u.clone();
    }
    if (src8u.channels() == 3) {
        cv::Mat gray;
        cv::cvtColor(src8u, gray, cv::COLOR_BGR2GRAY);
        return gray;
    }
    if (src8u.channels() == 4) {
        cv::Mat gray;
        cv::cvtColor(src8u, gray, cv::COLOR_BGRA2GRAY);
        return gray;
    }

    throw PdiError(
        ExitCode::invalid_parameter,
        "Numero de canais nao suportado para conversao em escala de cinza: " +
            std::to_string(src8u.channels())
    );
}

cv::Mat toBgr8U(const cv::Mat& src)
{
    if (src.empty()) {
        throw PdiError(ExitCode::read_error, "Imagem de entrada vazia.");
    }

    cv::Mat src8u;
    if (src.depth() != CV_8U) {
        src.convertTo(src8u, CV_8U);
    } else {
        src8u = src;
    }

    if (src8u.channels() == 3) {
        return src8u.clone();
    }
    if (src8u.channels() == 1) {
        cv::Mat bgr;
        cv::cvtColor(src8u, bgr, cv::COLOR_GRAY2BGR);
        return bgr;
    }
    if (src8u.channels() == 4) {
        cv::Mat bgr;
        cv::cvtColor(src8u, bgr, cv::COLOR_BGRA2BGR);
        return bgr;
    }

    throw PdiError(
        ExitCode::invalid_parameter,
        "Numero de canais nao suportado: " + std::to_string(src8u.channels())
    );
}

} // namespace

SegmentationMethod parseSegmentationMethod(std::string_view text)
{
    if (text == "fixed") return SegmentationMethod::fixed;
    if (text == "otsu") return SegmentationMethod::otsu;
    if (text == "adaptive") return SegmentationMethod::adaptive;
    if (text == "hsv") return SegmentationMethod::hsv;

    throw PdiError(
        ExitCode::invalid_parameter,
        "Metodo de segmentacao desconhecido: " + std::string(text) +
            ". Use fixed, otsu, adaptive ou hsv."
    );
}

std::string segmentationMethodToString(SegmentationMethod method)
{
    switch (method) {
    case SegmentationMethod::fixed: return "fixed";
    case SegmentationMethod::otsu: return "otsu";
    case SegmentationMethod::adaptive: return "adaptive";
    case SegmentationMethod::hsv: return "hsv";
    }
    return "fixed";
}

bool isValidBinaryMask(const cv::Mat& mask, int expected_rows, int expected_cols)
{
    if (mask.empty()) {
        return false;
    }
    if (mask.type() != CV_8UC1 || mask.channels() != 1) {
        return false;
    }
    if (expected_rows > 0 && mask.rows != expected_rows) {
        return false;
    }
    if (expected_cols > 0 && mask.cols != expected_cols) {
        return false;
    }

    for (int r = 0; r < mask.rows; ++r) {
        const auto* row_ptr = mask.ptr<uchar>(r);
        for (int c = 0; c < mask.cols; ++c) {
            const uchar val = row_ptr[c];
            if (val != 0 && val != 255) {
                return false;
            }
        }
    }
    return true;
}

cv::Mat ensureBinaryMask(const cv::Mat& input)
{
    if (input.empty()) {
        throw PdiError(ExitCode::read_error, "Mascara de entrada vazia.");
    }

    cv::Mat gray;
    if (input.channels() == 1 && input.type() == CV_8UC1) {
        gray = input.clone();
    } else if (input.channels() == 3 && input.depth() == CV_8U) {
        // Verifica se os 3 canais sao identicos (PNG salvo em RGB mas binario)
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        throw PdiError(
            ExitCode::invalid_parameter,
            "A mascara de entrada deve ter 1 canal de 8 bits (CV_8UC1)."
        );
    }

    if (!isValidBinaryMask(gray, input.rows, input.cols)) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "A mascara de entrada nao e estritamente binaria {0, 255}."
        );
    }

    return gray;
}

cv::Mat createSegmentationOverlay(const cv::Mat& src, const cv::Mat& mask)
{
    cv::Mat base = toBgr8U(src);
    cv::Mat overlay = base.clone();

    // Realca regioes detectadas como primeiro plano (255) com tingimento verde suave
    for (int r = 0; r < mask.rows; ++r) {
        const auto* m_ptr = mask.ptr<uchar>(r);
        auto* o_ptr = overlay.ptr<cv::Vec3b>(r);
        for (int c = 0; c < mask.cols; ++c) {
            if (m_ptr[c] == 255) {
                o_ptr[c][0] = static_cast<uchar>(o_ptr[c][0] * 0.6);
                o_ptr[c][1] = static_cast<uchar>( o_ptr[c][1] * 0.6 + 255 * 0.4 );
                o_ptr[c][2] = static_cast<uchar>(o_ptr[c][2] * 0.6);
            }
        }
    }

    // Desenha contorno vermelho das bordas da mascara para evidenciar FP/FN
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::drawContours(overlay, contours, -1, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);

    return overlay;
}

SegmentationResult segmentImage(const cv::Mat& src, const SegmentationParams& params)
{
    if (src.empty()) {
        throw PdiError(ExitCode::read_error, "Imagem de entrada vazia para segmentacao.");
    }

    if (params.blur_size < 0 || (params.blur_size > 0 && (params.blur_size < 3 || params.blur_size % 2 == 0))) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "--blur deve ser 0 (desativado) ou inteiro impar >= 3."
        );
    }

    SegmentationResult result;
    result.total_pixels = static_cast<std::uint64_t>(src.rows) * static_cast<std::uint64_t>(src.cols);

    if (params.method == SegmentationMethod::hsv) {
        if (src.channels() < 3) {
            throw PdiError(
                ExitCode::invalid_parameter,
                "O metodo hsv exige imagem colorida de entrada (3 canais)."
            );
        }
        if (params.h_min < 0 || params.h_min > 179 || params.h_max < 0 || params.h_max > 179 ||
            params.s_min < 0 || params.s_min > 255 || params.s_max < 0 || params.s_max > 255 ||
            params.v_min < 0 || params.v_min > 255 || params.v_max < 0 || params.v_max > 255 ||
            params.s_min > params.s_max || params.v_min > params.v_max) {
            throw PdiError(
                ExitCode::invalid_parameter,
                "Limites HSV invalidos. Use H em [0..179] e S,V em [0..255] com min <= max."
            );
        }

        cv::Mat bgr = toBgr8U(src);
        if (params.blur_size >= 3) {
            cv::GaussianBlur(bgr, bgr, cv::Size(params.blur_size, params.blur_size), 0.0);
        }

        cv::Mat hsv;
        cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);

        if (params.h_min <= params.h_max) {
            cv::inRange(
                hsv,
                cv::Scalar(params.h_min, params.s_min, params.v_min),
                cv::Scalar(params.h_max, params.s_max, params.v_max),
                result.mask
            );
        } else {
            // Tratamento explicito de faixas que cruzam a descontinuidade do matiz (ex.: vermelho 170..10)
            cv::Mat upper_hue;
            cv::Mat lower_hue;
            cv::inRange(
                hsv,
                cv::Scalar(params.h_min, params.s_min, params.v_min),
                cv::Scalar(179, params.s_max, params.v_max),
                upper_hue
            );
            cv::inRange(
                hsv,
                cv::Scalar(0, params.s_min, params.v_min),
                cv::Scalar(params.h_max, params.s_max, params.v_max),
                lower_hue
            );
            cv::bitwise_or(upper_hue, lower_hue, result.mask);
        }

        if (params.invert) {
            cv::bitwise_not(result.mask, result.mask);
        }
    } else {
        cv::Mat gray = toGrayscale8U(src);
        if (params.blur_size >= 3) {
            cv::GaussianBlur(gray, gray, cv::Size(params.blur_size, params.blur_size), 0.0);
        }

        if (params.method == SegmentationMethod::fixed) {
            if (params.threshold < 0 || params.threshold > 255) {
                throw PdiError(
                    ExitCode::invalid_parameter,
                    "--threshold deve estar entre 0 e 255."
                );
            }
            // Mantem a mesma convencao do contrato M1 (>= threshold -> 255, < threshold -> 0)
            // usando LUT/comparacao ou cv::threshold:
            result.mask = cv::Mat::zeros(gray.size(), CV_8UC1);
            for (int r = 0; r < gray.rows; ++r) {
                const auto* g_ptr = gray.ptr<uchar>(r);
                auto* m_ptr = result.mask.ptr<uchar>(r);
                for (int c = 0; c < gray.cols; ++c) {
                    const bool is_fg = (g_ptr[c] >= params.threshold);
                    const bool final_fg = params.invert ? !is_fg : is_fg;
                    m_ptr[c] = final_fg ? static_cast<uchar>(255) : static_cast<uchar>(0);
                }
            }
            result.effective_threshold = static_cast<double>(params.threshold);
        } else if (params.method == SegmentationMethod::otsu) {
            const int thresh_type = params.invert
                ? (cv::THRESH_BINARY_INV | cv::THRESH_OTSU)
                : (cv::THRESH_BINARY | cv::THRESH_OTSU);
            result.effective_threshold = cv::threshold(gray, result.mask, 0, 255, thresh_type);
        } else if (params.method == SegmentationMethod::adaptive) {
            if (params.block_size < 3 || params.block_size % 2 == 0) {
                throw PdiError(
                    ExitCode::invalid_parameter,
                    "--block-size deve ser inteiro impar >= 3."
                );
            }
            const int thresh_type = params.invert ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
            cv::adaptiveThreshold(
                gray,
                result.mask,
                255,
                cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                thresh_type,
                params.block_size,
                params.c_constant
            );
            result.effective_threshold = params.c_constant;
        }
    }

    if (!isValidBinaryMask(result.mask, src.rows, src.cols)) {
        throw PdiError(
            ExitCode::general_error,
            "Falha de validacao interna: mascara produzida nao e binaria {0, 255} de 1 canal."
        );
    }

    result.foreground_pixels = static_cast<std::uint64_t>(cv::countNonZero(result.mask));
    result.foreground_ratio = (result.total_pixels > 0)
        ? (static_cast<double>(result.foreground_pixels) / static_cast<double>(result.total_pixels))
        : 0.0;
    result.overlay = createSegmentationOverlay(src, result.mask);

    return result;
}

} // namespace pdi
