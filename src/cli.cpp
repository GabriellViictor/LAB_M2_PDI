#include "pdi/cli.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace pdi {
namespace {

constexpr std::array<std::string_view, 28> parameter_names {
    // M1
    "value", "levels", "threshold", "alpha", "kernel", "border", "size",
    // M2.1 (segment)
    "method", "invert", "blur", "block-size", "c",
    "h-min", "h-max", "s-min", "s-max", "v-min", "v-max",
    "hsv-low", "hsv-high", "overlay",
    // M2.2 (analyze)
    "connectivity", "min-area", "morph-config",
    "morph-seq", "morph-shapes", "morph-sizes", "morph-iters"
};

bool is_known_parameter(std::string_view name)
{
    for (const auto candidate : parameter_names) {
        if (candidate == name) {
            return true;
        }
    }
    return false;
}

} // namespace

CliOptions parse_cli(int argc, char** argv)
{
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "--help" || argument == "-h") {
            options.help = true;
            continue;
        }
        if (argument == "--version") {
            options.version = true;
            continue;
        }
        if (!argument.starts_with("--")) {
            throw std::invalid_argument("Argumento inesperado: " + argument);
        }
        if (i + 1 >= argc) {
            throw std::invalid_argument("Falta valor para: " + argument);
        }

        const std::string value = argv[++i];
        const std::string name = argument.substr(2);

        if (name == "operation") {
            options.operation = value;
        } else if (name == "input") {
            options.input = value;
        } else if (name == "output") {
            options.output = value;
        } else if (is_known_parameter(name)) {
            options.parameters[name] = value;
        } else {
            throw std::invalid_argument("Opcao desconhecida: --" + name);
        }
    }

    return options;
}

void print_help()
{
    std::cout
        << "pdi_lab - Laboratorios de Processamento Digital de Imagens (M1 + M2)\n\n"
        << "Uso geral:\n"
        << "  pdi_lab --operation <operacao> --input <arquivo> --output <arquivo-ou-prefixo> [parametros]\n"
        << "  pdi_lab --help\n"
        << "  pdi_lab --version\n\n"
        << "Operacoes M2:\n"
        << "  segment  (M2.1 — Mascara binaria e segmentacao)\n"
        << "    --method <fixed|otsu|adaptive|hsv>   Metodo de segmentacao (obrigatorio)\n"
        << "    --threshold <0..255>                 Limiar para --method fixed\n"
        << "    --invert <true|false>                Inverte mascara para objetos escuros\n"
        << "    --blur <0|3|5|7...>                  Filtro Gaussiano previo (opcional)\n"
        << "    --block-size <impar>=3> --c <real>   Parametros para --method adaptive\n"
        << "    --hsv-low <H,S,V> --hsv-high <H,S,V> Limites HSV (ou --h-min/--h-max etc.)\n"
        << "    --overlay <arquivo.png>              Salva sobreposicao da mascara na original\n\n"
        << "  analyze  (M2.2 — Componentes conexos e sequencia morfologica)\n"
        << "    --connectivity <4|8>                 Conectividade (padrao: 8)\n"
        << "    --min-area <int>=0>                  Filtro de area minima (padrao: 0)\n"
        << "    --morph-seq <op1,op2,...>            Sequencia (erode,dilate,open,close; padrao: open,close)\n"
        << "    --morph-shapes <s1,s2,...>           Elemento estruturante (rect,ellipse,cross)\n"
        << "    --morph-sizes <k1,k2,...>            Tamanhos impares >= 3 (padrao: 3,5)\n"
        << "    --morph-iters <i1,i2,...>            Iteracoes por estagio (padrao: 1,1)\n"
        << "    --morph-config <arquivo>             Arquivo de configuracao da sequencia\n\n"
        << "Operacoes M1 (preservadas):\n"
        << "  inspect, copy, channel_b, channel_g, channel_r, grayscale_average,\n"
        << "  grayscale_weighted, quantize, brightness, contrast, negative,\n"
        << "  threshold, histogram, convolution, mean_filter, weighted_mean, laplacian, sobel\n\n"
        << "Exemplos M2:\n"
        << "  pdi_lab --input images/input/cena.png --output images/output/cena_mask.png \\\n"
        << "    --operation segment --method fixed --threshold 120\n"
        << "  pdi_lab --input images/output/cena_mask.png --output results/cena \\\n"
        << "    --operation analyze --connectivity 8 --min-area 20\n";
}

void print_version()
{
    std::cout << "pdi_lab 0.3.0 contract=2 lab=m2.2\n";
}

} // namespace pdi
