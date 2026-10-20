#pragma once

#include <filesystem>
#include <opencv2/core/mat.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pdi {

enum class MorphOp {
    erode,
    dilate,
    open,
    close
};

enum class StructuringShape {
    rect,
    ellipse,
    cross
};

struct MorphStageConfig {
    MorphOp op = MorphOp::open;
    StructuringShape shape = StructuringShape::ellipse;
    int ksize = 3;       // Tamanho impar >= 3
    int iterations = 1;  // Numero de iteracoes >= 1
};

struct MorphStageResult {
    int stage_index = 1;
    MorphStageConfig config;
    std::string stage_tag; // Ex.: "stage1_open_ellipse3x3_it1"
    cv::Mat mask;
};

MorphOp parseMorphOp(std::string_view text);
std::string morphOpToString(MorphOp op);

StructuringShape parseStructuringShape(std::string_view text);
std::string structuringShapeToString(StructuringShape shape);

// Valida que a sequencia morfologica possui ao menos 2 operacoes, inclui obrigatoriamente
// abertura ou fechamento, e nao repete a mesma operacao com os mesmos parametros consecutivamente.
void validateMorphSequence(const std::vector<MorphStageConfig>& sequence);

// Sequencia morfologica padrao (abertura 3x3 seguida de fechamento 5x5 elipticos).
std::vector<MorphStageConfig> defaultMorphSequence();

// Carrega sequencia morfologica de arquivo texto (uma linha por estagio: <op> <shape> <ksize> <iterations>).
std::vector<MorphStageConfig> loadMorphConfigFile(const std::filesystem::path& path);

// Aplica um unico estagio morfologico sobre uma mascara binaria {0, 255}.
cv::Mat applyMorphStage(const cv::Mat& mask, const MorphStageConfig& stage);

// Executa toda a sequencia morfologica retornando a mascara intermediaria de cada estagio.
std::vector<MorphStageResult> applyMorphSequence(
    const cv::Mat& initial_mask,
    const std::vector<MorphStageConfig>& sequence
);

// Formata a descricao textual compacta da sequencia para o JSON de resumo.
std::string formatMorphSequenceSummary(const std::vector<MorphStageConfig>& sequence);

} // namespace pdi
