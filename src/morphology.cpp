#include "pdi/morphology.hpp"
#include "pdi/errors.hpp"
#include "pdi/segmentation.hpp"

#include <fstream>
#include <opencv2/imgproc.hpp>
#include <sstream>

namespace pdi {
namespace {

int toOpenCvMorphShape(StructuringShape shape)
{
    switch (shape) {
    case StructuringShape::rect: return cv::MORPH_RECT;
    case StructuringShape::ellipse: return cv::MORPH_ELLIPSE;
    case StructuringShape::cross: return cv::MORPH_CROSS;
    }
    return cv::MORPH_ELLIPSE;
}

int toOpenCvMorphOp(MorphOp op)
{
    switch (op) {
    case MorphOp::erode: return cv::MORPH_ERODE;
    case MorphOp::dilate: return cv::MORPH_DILATE;
    case MorphOp::open: return cv::MORPH_OPEN;
    case MorphOp::close: return cv::MORPH_CLOSE;
    }
    return cv::MORPH_OPEN;
}

} // namespace

MorphOp parseMorphOp(std::string_view text)
{
    if (text == "erode") return MorphOp::erode;
    if (text == "dilate") return MorphOp::dilate;
    if (text == "open") return MorphOp::open;
    if (text == "close") return MorphOp::close;

    throw PdiError(
        ExitCode::invalid_parameter,
        "Operacao morfologica desconhecida: " + std::string(text) +
            ". Use erode, dilate, open ou close."
    );
}

std::string morphOpToString(MorphOp op)
{
    switch (op) {
    case MorphOp::erode: return "erode";
    case MorphOp::dilate: return "dilate";
    case MorphOp::open: return "open";
    case MorphOp::close: return "close";
    }
    return "open";
}

StructuringShape parseStructuringShape(std::string_view text)
{
    if (text == "rect") return StructuringShape::rect;
    if (text == "ellipse") return StructuringShape::ellipse;
    if (text == "cross") return StructuringShape::cross;

    throw PdiError(
        ExitCode::invalid_parameter,
        "Forma de elemento estruturante desconhecida: " + std::string(text) +
            ". Use rect, ellipse ou cross."
    );
}

std::string structuringShapeToString(StructuringShape shape)
{
    switch (shape) {
    case StructuringShape::rect: return "rect";
    case StructuringShape::ellipse: return "ellipse";
    case StructuringShape::cross: return "cross";
    }
    return "ellipse";
}

void validateMorphSequence(const std::vector<MorphStageConfig>& sequence)
{
    if (sequence.size() < 2) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "A sequencia morfologica deve conter pelo menos 2 operacoes."
        );
    }

    bool has_open_or_close = false;
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        const auto& st = sequence[i];
        if (st.ksize < 3 || st.ksize % 2 == 0) {
            throw PdiError(
                ExitCode::invalid_parameter,
                "O tamanho do elemento estruturante deve ser inteiro impar >= 3."
            );
        }
        if (st.iterations < 1) {
            throw PdiError(
                ExitCode::invalid_parameter,
                "O numero de iteracoes morfologicas deve ser >= 1."
            );
        }
        if (st.op == MorphOp::open || st.op == MorphOp::close) {
            has_open_or_close = true;
        }
        if (i > 0) {
            const auto& prev = sequence[i - 1];
            if (prev.op == st.op && prev.shape == st.shape &&
                prev.ksize == st.ksize && prev.iterations == st.iterations) {
                throw PdiError(
                    ExitCode::invalid_parameter,
                    "Operacoes morfologicas consecutivas identicas com os mesmos parametros nao sao permitidas."
                );
            }
        }
    }

    if (!has_open_or_close) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "A sequencia morfologica deve incluir obrigatoriamente open (abertura) ou close (fechamento)."
        );
    }
}

std::vector<MorphStageConfig> defaultMorphSequence()
{
    return {
        MorphStageConfig{MorphOp::open, StructuringShape::ellipse, 3, 1},
        MorphStageConfig{MorphOp::close, StructuringShape::ellipse, 5, 1}
    };
}

std::vector<MorphStageConfig> loadMorphConfigFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) {
        throw PdiError(
            ExitCode::read_error,
            "Nao foi possivel abrir o arquivo de configuracao morfologica: " + path.string()
        );
    }

    std::vector<MorphStageConfig> sequence;
    std::string line;
    int line_num = 0;
    while (std::getline(input, line)) {
        ++line_num;
        const auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        std::istringstream iss(line);
        std::string op_str;
        if (!(iss >> op_str)) {
            continue; // Linha em branco ou comentario
        }

        std::string shape_str;
        int ksize = 0;
        int iterations = 0;
        if (!(iss >> shape_str >> ksize >> iterations)) {
            throw PdiError(
                ExitCode::invalid_parameter,
                "Linha invalida na configuracao morfologica (" + std::to_string(line_num) +
                    "): esperado '<op> <shape> <ksize> <iterations>'."
            );
        }

        sequence.push_back(MorphStageConfig{
            parseMorphOp(op_str),
            parseStructuringShape(shape_str),
            ksize,
            iterations
        });
    }

    validateMorphSequence(sequence);
    return sequence;
}

cv::Mat applyMorphStage(const cv::Mat& mask, const MorphStageConfig& stage)
{
    cv::Mat binary = ensureBinaryMask(mask);

    if (stage.ksize < 3 || stage.ksize % 2 == 0) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "Tamanho do elemento estruturante invalido: " + std::to_string(stage.ksize)
        );
    }
    if (stage.iterations < 1) {
        throw PdiError(
            ExitCode::invalid_parameter,
            "Numero de iteracoes invalido: " + std::to_string(stage.iterations)
        );
    }

    cv::Mat kernel = cv::getStructuringElement(
        toOpenCvMorphShape(stage.shape),
        cv::Size(stage.ksize, stage.ksize)
    );

    cv::Mat dst;
    cv::morphologyEx(
        binary,
        dst,
        toOpenCvMorphOp(stage.op),
        kernel,
        cv::Point(-1, -1),
        stage.iterations
    );

    return dst;
}

std::vector<MorphStageResult> applyMorphSequence(
    const cv::Mat& initial_mask,
    const std::vector<MorphStageConfig>& sequence)
{
    validateMorphSequence(sequence);

    cv::Mat current = ensureBinaryMask(initial_mask);
    std::vector<MorphStageResult> results;
    results.reserve(sequence.size());

    for (std::size_t i = 0; i < sequence.size(); ++i) {
        const auto& cfg = sequence[i];
        current = applyMorphStage(current, cfg);

        const int idx = static_cast<int>(i + 1);
        std::ostringstream tag;
        tag << "stage" << idx << "_" << morphOpToString(cfg.op);

        results.push_back(MorphStageResult{
            idx,
            cfg,
            tag.str(),
            current.clone()
        });
    }

    return results;
}

std::string formatMorphSequenceSummary(const std::vector<MorphStageConfig>& sequence)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        if (i > 0) {
            oss << " -> ";
        }
        const auto& st = sequence[i];
        oss << morphOpToString(st.op) << "("
            << structuringShapeToString(st.shape) << ","
            << st.ksize << "x" << st.ksize << ",it="
            << st.iterations << ")";
    }
    return oss.str();
}

} // namespace pdi
