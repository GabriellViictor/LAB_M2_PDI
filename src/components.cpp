#include "pdi/components.hpp"
#include "pdi/errors.hpp"
#include "pdi/result_io.hpp"
#include "pdi/segmentation.hpp"

#include <array>
#include <fstream>
#include <iomanip>
#include <opencv2/imgproc.hpp>
#include <string>

namespace pdi {
namespace {

cv::Vec3b colorForLabel(int label)
{
    static const std::array<cv::Vec3b, 12> palette {{
        {66, 133, 244},   // Azul
        {52, 168, 83},    // Verde
        {251, 188, 5},    // Amarelo/Ouro
        {234, 67, 53},    // Vermelho/Coral
        {171, 71, 188},   // Roxo
        {0, 172, 193},    // Ciano
        {255, 112, 67},   // Laranja
        {158, 157, 36},   // Oliva
        {92, 107, 192},   // Indigo
        {38, 166, 154},   // Teal
        {236, 64, 122},   // Rosa
        {120, 144, 156}   // Cinza-azulado
    }};
    return palette[static_cast<std::size_t>((label - 1) % static_cast<int>(palette.size()))];
}

} // namespace

ConnectedComponentsReport analyzeConnectedComponents(
    const cv::Mat& binary_mask,
    int connectivity,
    int min_area)
{
    if (connectivity != 4 && connectivity != 8) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "Valor invalido para --connectivity: " + std::to_string(connectivity) +
                ". Use 4 ou 8."
        );
    }
    if (min_area < 0) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "--min-area deve ser um inteiro nao negativo."
        );
    }

    cv::Mat mask = ensureBinaryMask(binary_mask);

    ConnectedComponentsReport report;
    report.connectivity = connectivity;
    report.min_area = min_area;

    cv::Mat stats;
    cv::Mat centroids;
    const int num_labels = cv::connectedComponentsWithStats(
        mask,
        report.labels_32s,
        stats,
        centroids,
        connectivity,
        CV_32S
    );

    // O rotulo 0 representa o fundo e nao entra na lista de objetos
    report.raw_count = (num_labels > 1) ? (num_labels - 1) : 0;
    report.components.reserve(static_cast<std::size_t>(report.raw_count));

    for (int lbl = 1; lbl < num_labels; ++lbl) {
        const int area = stats.at<int>(lbl, cv::CC_STAT_AREA);
        const int x = stats.at<int>(lbl, cv::CC_STAT_LEFT);
        const int y = stats.at<int>(lbl, cv::CC_STAT_TOP);
        const int width = stats.at<int>(lbl, cv::CC_STAT_WIDTH);
        const int height = stats.at<int>(lbl, cv::CC_STAT_HEIGHT);
        const double cx = centroids.at<double>(lbl, 0);
        const double cy = centroids.at<double>(lbl, 1);
        const bool kept = (area >= min_area);

        report.components.push_back(ComponentInfo{
            lbl, area, x, y, width, height, cx, cy, kept
        });

        report.raw_foreground_area += static_cast<std::uint64_t>(area);
        if (kept) {
            ++report.filtered_count;
            report.filtered_foreground_area += static_cast<std::uint64_t>(area);
        }
    }

    // Construcao da mascara filtrada e da visualizacao colorida
    report.filtered_mask = cv::Mat::zeros(mask.size(), CV_8UC1);
    report.visualization = cv::Mat::zeros(mask.size(), CV_8UC3);

    for (int r = 0; r < report.labels_32s.rows; ++r) {
        const auto* lbl_ptr = report.labels_32s.ptr<int>(r);
        auto* fmask_ptr = report.filtered_mask.ptr<uchar>(r);
        auto* vis_ptr = report.visualization.ptr<cv::Vec3b>(r);

        for (int c = 0; c < report.labels_32s.cols; ++c) {
            const int lbl = lbl_ptr[c];
            if (lbl <= 0) {
                continue;
            }
            const auto& comp = report.components[static_cast<std::size_t>(lbl - 1)];
            if (comp.kept) {
                fmask_ptr[c] = 255;
                vis_ptr[c] = colorForLabel(lbl);
            } else {
                // Componentes descartados pelo filtro de area aparecem em cinza escuro na visualizacao
                vis_ptr[c] = cv::Vec3b(55, 55, 55);
            }
        }
    }

    // Desenha caixas delimitadoras, centroides e identificadores nos componentes mantidos
    for (const auto& comp : report.components) {
        if (!comp.kept) {
            continue;
        }
        const cv::Rect bbox(comp.x, comp.y, comp.width, comp.height);
        cv::rectangle(report.visualization, bbox, cv::Scalar(255, 255, 255), 1, cv::LINE_8);

        const cv::Point center(
            static_cast<int>(std::round(comp.cx)),
            static_cast<int>(std::round(comp.cy))
        );
        cv::circle(report.visualization, center, 2, cv::Scalar(0, 0, 255), cv::FILLED, cv::LINE_AA);

        const std::string text = "#" + std::to_string(comp.label);
        const int text_x = std::max(0, comp.x);
        const int text_y = std::max(10, comp.y - 3);
        cv::putText(
            report.visualization,
            text,
            cv::Point(text_x, text_y),
            cv::FONT_HERSHEY_SIMPLEX,
            0.35,
            cv::Scalar(0, 255, 255),
            1,
            cv::LINE_AA
        );
    }

    return report;
}

void writeComponentsCsv(
    const std::filesystem::path& path,
    const std::vector<ComponentInfo>& components)
{
    try {
        ensure_parent_directory(path);
    } catch (const std::exception& error) {
        throw PdiError(
            ExitCode::write_error,
            "Nao foi possivel preparar o diretorio para o CSV de componentes: " + std::string(error.what())
        );
    }

    std::ofstream output(path);
    if (!output) {
        throw PdiError(
            ExitCode::write_error,
            "Nao foi possivel criar o CSV de componentes: " + path.string()
        );
    }

    output << "label,area,x,y,width,height,cx,cy,kept\n";
    output << std::fixed << std::setprecision(1);
    for (const auto& comp : components) {
        output << comp.label << ','
               << comp.area << ','
               << comp.x << ','
               << comp.y << ','
               << comp.width << ','
               << comp.height << ','
               << comp.cx << ','
               << comp.cy << ','
               << (comp.kept ? "true" : "false") << '\n';
    }
}

} // namespace pdi
