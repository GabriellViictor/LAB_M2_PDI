#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace pdi {

// Representa a linha de comando depois do parsing sintático inicial.
//
// operation/input/output aparecem separadamente porque formam o contrato
// básico de execução. Os parâmetros específicos das operações ficam no mapa
// parameters e são convertidos para tipos adequados pela camada parameters.
// O estudante normalmente não precisa alterar esta estrutura.
struct CliOptions {
    bool help = false;
    bool version = false;
    std::string operation;
    std::optional<std::string> input;
    std::optional<std::string> output;
    std::unordered_map<std::string, std::string> parameters;
};

// Interpreta os argumentos do terminal e rejeita opções desconhecidas.
// A validação semântica de valores acontece posteriormente em contract.cpp.
CliOptions parse_cli(int argc, char** argv);

// Exibem documentação de uso e identificação da versão sem executar operação.
void print_help();
void print_version();

} // namespace pdi
