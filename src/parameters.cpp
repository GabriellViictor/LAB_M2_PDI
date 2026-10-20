#include "pdi/parameters.hpp"
#include "pdi/errors.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

namespace pdi {
namespace {

const std::string& require_parameter(const CliOptions& options, const std::string& name)
{
    const auto it = options.parameters.find(name);
    if (it == options.parameters.end() || it->second.empty()) {
        throw std::invalid_argument("Parametro ausente: --" + name);
    }
    return it->second;
}

std::vector<std::string> split_csv_tokens(const std::string& text)
{
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) {
            throw PdiError(ExitCode::invalid_parameter, "Lista separada por virgulas contem item vazio: " + text);
        }
        tokens.push_back(item);
    }
    if (tokens.empty()) {
        throw PdiError(ExitCode::invalid_parameter, "Lista de parametros vazia.");
    }
    return tokens;
}

int parse_strict_int(const std::string& text, const std::string& context_name)
{
    std::size_t consumed = 0;
    int value = 0;
    try {
        value = std::stoi(text, &consumed);
    } catch (const std::exception&) {
        throw PdiError(ExitCode::invalid_parameter, "Valor inteiro invalido para " + context_name + ": " + text);
    }
    if (consumed != text.size()) {
        throw PdiError(ExitCode::invalid_parameter, "Valor inteiro invalido para " + context_name + ": " + text);
    }
    return value;
}

} // namespace

bool has_parameter(const CliOptions& options, const std::string& name)
{
    const auto it = options.parameters.find(name);
    return it != options.parameters.end() && !it->second.empty();
}

int parameter_as_int(const CliOptions& options, const std::string& name)
{
    const auto& text = require_parameter(options, name);
    std::size_t consumed = 0;
    int value = 0;
    try {
        value = std::stoi(text, &consumed);
    } catch (const std::exception&) {
        throw std::invalid_argument("Valor invalido para --" + name + ": " + text);
    }
    if (consumed != text.size()) {
        throw std::invalid_argument("Valor invalido para --" + name + ": " + text);
    }
    return value;
}

double parameter_as_double(const CliOptions& options, const std::string& name)
{
    const auto& text = require_parameter(options, name);
    std::size_t consumed = 0;
    double value = 0.0;
    try {
        value = std::stod(text, &consumed);
    } catch (const std::exception&) {
        throw std::invalid_argument("Valor invalido para --" + name + ": " + text);
    }
    if (consumed != text.size()) {
        throw std::invalid_argument("Valor invalido para --" + name + ": " + text);
    }
    return value;
}

bool parameter_as_bool(const CliOptions& options, const std::string& name)
{
    const auto& text = require_parameter(options, name);
    if (text == "true" || text == "1" || text == "yes") {
        return true;
    }
    if (text == "false" || text == "0" || text == "no") {
        return false;
    }
    throw std::invalid_argument(
        "Valor booleano invalido para --" + name + ": " + text + ". Use true ou false."
    );
}

std::filesystem::path parameter_as_path(const CliOptions& options, const std::string& name)
{
    return std::filesystem::path(require_parameter(options, name));
}

BorderStrategy parameter_as_border(const CliOptions& options, const std::string& name)
{
    const auto& text = require_parameter(options, name);
    if (text == "copy") {
        return BorderStrategy::copy;
    }
    if (text == "replicate") {
        return BorderStrategy::replicate;
    }
    throw std::invalid_argument(
        "Valor invalido para --" + name + ": " + text +
        ". Use copy ou replicate."
    );
}

SegmentationParams parse_segmentation_params(const CliOptions& options)
{
    if (!has_parameter(options, "method")) {
        throw PdiError(ExitCode::invalid_arguments, "A operacao 'segment' exige --method.");
    }

    SegmentationParams params;
    params.method = parseSegmentationMethod(options.parameters.at("method"));

    if (has_parameter(options, "invert")) {
        params.invert = parameter_as_bool(options, "invert");
    }
    if (has_parameter(options, "blur")) {
        params.blur_size = parameter_as_int(options, "blur");
        if (params.blur_size < 0 || (params.blur_size > 0 && (params.blur_size < 3 || params.blur_size % 2 == 0))) {
            throw PdiError(ExitCode::invalid_parameter, "--blur deve ser 0 ou inteiro impar >= 3.");
        }
    }

    if (params.method == SegmentationMethod::fixed) {
        if (!has_parameter(options, "threshold")) {
            throw PdiError(ExitCode::invalid_arguments, "O metodo 'fixed' exige --threshold.");
        }
        params.threshold = parameter_as_int(options, "threshold");
        if (params.threshold < 0 || params.threshold > 255) {
            throw PdiError(ExitCode::invalid_parameter, "--threshold deve estar entre 0 e 255.");
        }
    } else if (params.method == SegmentationMethod::adaptive) {
        if (has_parameter(options, "block-size")) {
            params.block_size = parameter_as_int(options, "block-size");
        }
        if (params.block_size < 3 || params.block_size % 2 == 0) {
            throw PdiError(ExitCode::invalid_parameter, "--block-size deve ser inteiro impar >= 3.");
        }
        if (has_parameter(options, "c")) {
            params.c_constant = parameter_as_double(options, "c");
        }
    } else if (params.method == SegmentationMethod::hsv) {
        if (has_parameter(options, "hsv-low")) {
            const auto low_tokens = split_csv_tokens(options.parameters.at("hsv-low"));
            if (low_tokens.size() != 3) {
                throw PdiError(ExitCode::invalid_parameter, "--hsv-low exige formato H,S,V.");
            }
            params.h_min = parse_strict_int(low_tokens[0], "--hsv-low(H)");
            params.s_min = parse_strict_int(low_tokens[1], "--hsv-low(S)");
            params.v_min = parse_strict_int(low_tokens[2], "--hsv-low(V)");
        }
        if (has_parameter(options, "hsv-high")) {
            const auto high_tokens = split_csv_tokens(options.parameters.at("hsv-high"));
            if (high_tokens.size() != 3) {
                throw PdiError(ExitCode::invalid_parameter, "--hsv-high exige formato H,S,V.");
            }
            params.h_max = parse_strict_int(high_tokens[0], "--hsv-high(H)");
            params.s_max = parse_strict_int(high_tokens[1], "--hsv-high(S)");
            params.v_max = parse_strict_int(high_tokens[2], "--hsv-high(V)");
        }
        if (has_parameter(options, "h-min")) params.h_min = parameter_as_int(options, "h-min");
        if (has_parameter(options, "h-max")) params.h_max = parameter_as_int(options, "h-max");
        if (has_parameter(options, "s-min")) params.s_min = parameter_as_int(options, "s-min");
        if (has_parameter(options, "s-max")) params.s_max = parameter_as_int(options, "s-max");
        if (has_parameter(options, "v-min")) params.v_min = parameter_as_int(options, "v-min");
        if (has_parameter(options, "v-max")) params.v_max = parameter_as_int(options, "v-max");

        if (params.h_min < 0 || params.h_min > 179 || params.h_max < 0 || params.h_max > 179 ||
            params.s_min < 0 || params.s_min > 255 || params.s_max < 0 || params.s_max > 255 ||
            params.v_min < 0 || params.v_min > 255 || params.v_max < 0 || params.v_max > 255 ||
            params.s_min > params.s_max || params.v_min > params.v_max) {
            throw PdiError(
                ExitCode::invalid_parameter,
                "Limites HSV invalidos: H em [0..179], S e V em [0..255] com min <= max."
            );
        }
    }

    return params;
}

std::vector<MorphStageConfig> parse_morph_sequence(const CliOptions& options)
{
    if (has_parameter(options, "morph-config")) {
        const auto cfg_path = parameter_as_path(options, "morph-config");
        return loadMorphConfigFile(cfg_path);
    }

    if (!has_parameter(options, "morph-seq")) {
        return defaultMorphSequence();
    }

    const auto op_tokens = split_csv_tokens(options.parameters.at("morph-seq"));
    const std::size_t n = op_tokens.size();

    std::vector<std::string> shape_tokens(n, "ellipse");
    if (has_parameter(options, "morph-shapes")) {
        const auto parsed = split_csv_tokens(options.parameters.at("morph-shapes"));
        if (parsed.size() == 1) {
            shape_tokens.assign(n, parsed[0]);
        } else if (parsed.size() == n) {
            shape_tokens = parsed;
        } else {
            throw PdiError(
                ExitCode::invalid_parameter,
                "--morph-shapes deve ter 1 valor ou a mesma quantidade de estagios que --morph-seq."
            );
        }
    }

    std::vector<int> ksizes(n, 3);
    if (n >= 2 && !has_parameter(options, "morph-sizes")) {
        ksizes[1] = 5; // Default distinto quando usuario informa apenas op1,op2 iguais
    }
    if (has_parameter(options, "morph-sizes")) {
        const auto parsed = split_csv_tokens(options.parameters.at("morph-sizes"));
        if (parsed.size() == 1) {
            ksizes.assign(n, parse_strict_int(parsed[0], "--morph-sizes"));
        } else if (parsed.size() == n) {
            for (std::size_t i = 0; i < n; ++i) {
                ksizes[i] = parse_strict_int(parsed[i], "--morph-sizes");
            }
        } else {
            throw PdiError(
                ExitCode::invalid_parameter,
                "--morph-sizes deve ter 1 valor ou a mesma quantidade de estagios que --morph-seq."
            );
        }
    }

    std::vector<int> iters(n, 1);
    if (has_parameter(options, "morph-iters")) {
        const auto parsed = split_csv_tokens(options.parameters.at("morph-iters"));
        if (parsed.size() == 1) {
            iters.assign(n, parse_strict_int(parsed[0], "--morph-iters"));
        } else if (parsed.size() == n) {
            for (std::size_t i = 0; i < n; ++i) {
                iters[i] = parse_strict_int(parsed[i], "--morph-iters");
            }
        } else {
            throw PdiError(
                ExitCode::invalid_parameter,
                "--morph-iters deve ter 1 valor ou a mesma quantidade de estagios que --morph-seq."
            );
        }
    }

    std::vector<MorphStageConfig> sequence;
    sequence.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        sequence.push_back(MorphStageConfig{
            parseMorphOp(op_tokens[i]),
            parseStructuringShape(shape_tokens[i]),
            ksizes[i],
            iters[i]
        });
    }

    validateMorphSequence(sequence);
    return sequence;
}

} // namespace pdi
