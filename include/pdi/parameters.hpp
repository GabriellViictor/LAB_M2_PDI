#pragma once

#include "pdi/cli.hpp"
#include "pdi/morphology.hpp"
#include "pdi/segmentation.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace pdi {

// Estratégias de borda previstas no Laboratório M1.3.
enum class BorderStrategy {
    copy,
    replicate
};

// Funções de infraestrutura para obter parâmetros já convertidos.
int parameter_as_int(const CliOptions& options, const std::string& name);
double parameter_as_double(const CliOptions& options, const std::string& name);
bool parameter_as_bool(const CliOptions& options, const std::string& name);
std::filesystem::path parameter_as_path(const CliOptions& options, const std::string& name);
BorderStrategy parameter_as_border(const CliOptions& options, const std::string& name = "border");

bool has_parameter(const CliOptions& options, const std::string& name);

// M2.1 — Converte e valida parametros da operacao segment
SegmentationParams parse_segmentation_params(const CliOptions& options);

// M2.2 — Converte e valida a sequencia morfologica via argumentos de CLI ou --morph-config
std::vector<MorphStageConfig> parse_morph_sequence(const CliOptions& options);

} // namespace pdi
