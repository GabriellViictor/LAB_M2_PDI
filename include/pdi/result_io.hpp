#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

namespace pdi {

// Cria os diretórios-pai necessários para um arquivo de saída.
void ensure_parent_directory(const std::filesystem::path& path);

// Salva um histograma já calculado pelo estudante.
// Esta função não calcula o histograma; apenas serializa os 256 contadores.
void write_histogram_csv(
    const std::filesystem::path& path,
    const std::array<std::uint64_t, 256>& histogram
);

// Metadados simples para registrar parâmetros e resultados de execução.
// O projeto-base cuida apenas da serialização; o conteúdo continua sendo
// definido pelo código do estudante.
using StringMap = std::map<std::string, std::string>;

void write_json_object(
    const std::filesystem::path& path,
    const StringMap& values
);

} // namespace pdi
