#pragma once

#include <cstdint>
#include <filesystem>
#include <opencv2/core/mat.hpp>
#include <vector>

namespace pdi {

struct ComponentInfo {
    int label = 0;      // Rotulo do componente (1..N; 0 = fundo excluido)
    int area = 0;       // Area em pixels
    int x = 0;          // Bounding box: coluna inicial (left)
    int y = 0;          // Bounding box: linha inicial (top)
    int width = 0;      // Bounding box: largura
    int height = 0;     // Bounding box: altura
    double cx = 0.0;    // Centroide X
    double cy = 0.0;    // Centroide Y
    bool kept = true;   // Indica se o componente passou pelo filtro --min-area
};

struct ConnectedComponentsReport {
    int connectivity = 8;                    // Conectividade usada (4 ou 8)
    int min_area = 0;                        // Area minima para filtragem
    int raw_count = 0;                       // Quantidade bruta de componentes (sem fundo)
    int filtered_count = 0;                  // Quantidade de componentes com kept == true
    std::uint64_t raw_foreground_area = 0;   // Soma das areas brutas (== pixels 255 da mascara)
    std::uint64_t filtered_foreground_area = 0; // Soma das areas dos componentes mantidos
    std::vector<ComponentInfo> components;   // Lista completa de componentes (1..N)
    cv::Mat labels_32s;                      // Matriz de rotulos (CV_32S)
    cv::Mat filtered_mask;                   // Mascara binaria {0, 255} apenas com componentes mantidos
    cv::Mat visualization;                   // Imagem colorida com rotulos, caixas delimitadoras e centroides
};

// Rotula componentes conexos de primeiro plano (255) na mascara binaria usando conectividade 4 ou 8.
ConnectedComponentsReport analyzeConnectedComponents(
    const cv::Mat& binary_mask,
    int connectivity,
    int min_area = 0
);

// Exporta a tabela de componentes conexos em formato CSV padronizado (M2.2):
// label,area,x,y,width,height,cx,cy,kept
void writeComponentsCsv(
    const std::filesystem::path& path,
    const std::vector<ComponentInfo>& components
);

} // namespace pdi
