#include "pdi/operations.hpp"

#include "pdi/components.hpp"
#include "pdi/filtering.hpp"
#include "pdi/image_io.hpp"
#include "pdi/intensity.hpp"
#include "pdi/kernel.hpp"
#include "pdi/morphology.hpp"
#include "pdi/parameters.hpp"
#include "pdi/representation.hpp"
#include "pdi/result_io.hpp"
#include "pdi/segmentation.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

namespace pdi {
namespace {

constexpr std::array<std::string_view, 20> operations {
    // M1.1 — representação, canais e níveis de cinza
    "inspect",
    "copy",
    "channel_b",
    "channel_g",
    "channel_r",
    "grayscale_average",
    "grayscale_weighted",
    "quantize",

    // M1.2 — transformações de intensidade
    "brightness",
    "contrast",
    "negative",
    "threshold",
    "histogram",

    // M1.3 — convolução e filtragem espacial
    "convolution",
    "mean_filter",
    "weighted_mean",
    "laplacian",
    "sobel",

    // M2.1 — máscara binária e segmentação
    "segment",

    // M2.2 — componentes conexos e sequência morfológica
    "analyze"
};

} // namespace

bool is_known_operation(std::string_view operation)
{
    for (const auto candidate : operations) {
        if (candidate == operation) {
            return true;
        }
    }
    return false;
}

ExitCode run_operation(const CliOptions& options)
{
    // M1.1
    if (options.operation == "inspect") {
        cv::Mat input = read_image(*options.input);
        inspectImage(input);
        return ExitCode::success;
    }

    if (options.operation == "copy") {
        cv::Mat input = read_image(*options.input);
        cv::Mat result = copyImage(input);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "channel_b") {
        cv::Mat input = read_image(*options.input);
        cv::Mat result = extractChannel(input, 0);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "channel_g") {
        cv::Mat input = read_image(*options.input);
        cv::Mat result = extractChannel(input, 1);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "channel_r") {
        cv::Mat input = read_image(*options.input);
        cv::Mat result = extractChannel(input, 2);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "grayscale_average") {
        cv::Mat input = read_image(*options.input);
        cv::Mat result = grayScaleAvg(input);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "grayscale_weighted") {
        cv::Mat input = read_image(*options.input);
        cv::Mat result = grayscaleWeighted(input);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "quantize") {
        cv::Mat input = read_image(*options.input);
        int levels = parameter_as_int(options, "levels");
        cv::Mat result = quantImg(input, levels);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    // M1.2
    if (options.operation == "brightness") {
        cv::Mat input = read_image(*options.input);
        int value = parameter_as_int(options, "value");
        cv::Mat result = adjustBrightness(input, value);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "contrast") {
        cv::Mat input = read_image(*options.input);
        double alpha = parameter_as_double(options, "alpha");
        cv::Mat result = adjustContrast(input, alpha);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "negative") {
        cv::Mat input = read_image(*options.input);
        cv::Mat result = invertNegative(input);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "threshold") {
        cv::Mat input = read_image(*options.input);
        int thresh = parameter_as_int(options, "threshold");
        cv::Mat result = applyThreshold(input, thresh);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "histogram") {
        cv::Mat input = read_image(*options.input);
        auto hist = computeHistogram(input);
        write_histogram_csv(*options.output, hist);
        return ExitCode::success;
    }

    // M1.3
    if (options.operation == "convolution") {
        cv::Mat input = read_image(*options.input);
        auto kernel_path = parameter_as_path(options, "kernel");
        Kernel kernel = read_kernel(kernel_path);
        BorderStrategy border = parameter_as_border(options);
        cv::Mat result = applyConvolution(input, kernel, border);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "mean_filter") {
        cv::Mat input = read_image(*options.input);
        int size = parameter_as_int(options, "size");
        BorderStrategy border = parameter_as_border(options);
        cv::Mat result = meanFilter(input, size, border);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "weighted_mean") {
        cv::Mat input = read_image(*options.input);
        BorderStrategy border = parameter_as_border(options);
        cv::Mat result = weightedMeanFilter(input, border);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "laplacian") {
        cv::Mat input = read_image(*options.input);
        BorderStrategy border = parameter_as_border(options);
        cv::Mat result = laplacianFilter(input, border);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    if (options.operation == "sobel") {
        cv::Mat input = read_image(*options.input);
        BorderStrategy border = parameter_as_border(options);
        cv::Mat result = sobelFilter(input, border);
        write_image(*options.output, result);
        return ExitCode::success;
    }

    // M2.1 — Segmentação e geração de máscara binária
    if (options.operation == "segment") {
        cv::Mat input = read_image(*options.input);
        SegmentationParams params = parse_segmentation_params(options);
        SegmentationResult seg = segmentImage(input, params);

        write_image(*options.output, seg.mask);

        if (has_parameter(options, "overlay")) {
            write_image(options.parameters.at("overlay"), seg.overlay);
        }

        std::ostringstream ratio_str;
        ratio_str << std::fixed << std::setprecision(4) << seg.foreground_ratio;

        std::cout
            << "method=" << segmentationMethodToString(params.method) << '\n'
            << "width=" << seg.mask.cols << '\n'
            << "height=" << seg.mask.rows << '\n'
            << "channels=" << seg.mask.channels() << '\n'
            << "foreground_pixels=" << seg.foreground_pixels << '\n'
            << "total_pixels=" << seg.total_pixels << '\n'
            << "foreground_ratio=" << ratio_str.str() << '\n'
            << "binary_valid=" << (isValidBinaryMask(seg.mask, input.rows, input.cols) ? "true" : "false") << '\n';

        return ExitCode::success;
    }

    // M2.2 — Rotulação de componentes conexos e sequência morfológica
    if (options.operation == "analyze") {
        cv::Mat raw_input = read_image(*options.input);
        cv::Mat initial_mask = ensureBinaryMask(raw_input);

        const int connectivity = has_parameter(options, "connectivity")
            ? parameter_as_int(options, "connectivity")
            : 8;
        const int min_area = has_parameter(options, "min-area")
            ? parameter_as_int(options, "min-area")
            : 0;

        // 1. Rotula máscara inicial em conectividade 4 e 8 para comparação exigida (M2.2 Atividade 2)
        const auto init_conn4 = analyzeConnectedComponents(initial_mask, 4, min_area);
        const auto init_conn8 = analyzeConnectedComponents(initial_mask, 8, min_area);
        const auto& initial_report = (connectivity == 4) ? init_conn4 : init_conn8;

        // 2. Executa sequência morfológica justificada
        const auto morph_sequence = parse_morph_sequence(options);
        const auto stages = applyMorphSequence(initial_mask, morph_sequence);
        const cv::Mat& final_mask = stages.back().mask;

        // 3. Rotula máscara final com a conectividade e filtro --min-area selecionados
        const auto final_report = analyzeConnectedComponents(final_mask, connectivity, min_area);

        // 4. Salva todas as saídas usando --output como prefixo
        const std::string prefix = *options.output;

        write_image(prefix + "_initial_mask.png", initial_mask);
        for (const auto& st : stages) {
            write_image(prefix + "_" + st.stage_tag + ".png", st.mask);
        }
        write_image(prefix + "_final_mask.png", final_report.filtered_mask);
        write_image(prefix + "_labels.png", final_report.visualization);

        writeComponentsCsv(prefix + "_initial.csv", initial_report.components);
        writeComponentsCsv(prefix + "_final.csv", final_report.components);

        StringMap summary {
            {"connectivity", std::to_string(connectivity)},
            {"min_area", std::to_string(min_area)},
            {"morph_sequence", formatMorphSequenceSummary(morph_sequence)},
            {"morph_stages_count", std::to_string(stages.size())},
            {"initial_count_conn4_raw", std::to_string(init_conn4.raw_count)},
            {"initial_count_conn8_raw", std::to_string(init_conn8.raw_count)},
            {"initial_count_raw", std::to_string(initial_report.raw_count)},
            {"initial_count_filtered", std::to_string(initial_report.filtered_count)},
            {"initial_foreground_area_raw", std::to_string(initial_report.raw_foreground_area)},
            {"initial_foreground_area_filtered", std::to_string(initial_report.filtered_foreground_area)},
            {"final_count_raw", std::to_string(final_report.raw_count)},
            {"final_count_filtered", std::to_string(final_report.filtered_count)},
            {"final_foreground_area_raw", std::to_string(final_report.raw_foreground_area)},
            {"final_foreground_area_filtered", std::to_string(final_report.filtered_foreground_area)}
        };
        write_json_object(prefix + "_summary.json", summary);

        std::cout
            << "connectivity=" << connectivity << '\n'
            << "min_area=" << min_area << '\n'
            << "initial_count_conn4=" << init_conn4.raw_count << '\n'
            << "initial_count_conn8=" << init_conn8.raw_count << '\n'
            << "initial_count_raw=" << initial_report.raw_count << '\n'
            << "initial_count_filtered=" << initial_report.filtered_count << '\n'
            << "initial_foreground_area=" << initial_report.raw_foreground_area << '\n'
            << "final_count_raw=" << final_report.raw_count << '\n'
            << "final_count_filtered=" << final_report.filtered_count << '\n'
            << "final_foreground_area_raw=" << final_report.raw_foreground_area << '\n'
            << "final_foreground_area_filtered=" << final_report.filtered_foreground_area << '\n';

        return ExitCode::success;
    }

    std::cerr
        << "Operacao '" << options.operation
        << "' reconhecida, mas ainda nao implementada.\n";

    return ExitCode::general_error;
}

} // namespace pdi
