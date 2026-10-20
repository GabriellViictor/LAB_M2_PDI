#pragma once

#include "pdi/exit_code.hpp"

#include <stdexcept>
#include <string>

namespace pdi {

// Exceção da infraestrutura com código de saída padronizado.
// Permite que C++, Java e Python sinalizem categorias equivalentes de erro.
class PdiError : public std::runtime_error {
public:
    PdiError(ExitCode code, const std::string& message)
        : std::runtime_error(message), code_(code)
    {
    }

    [[nodiscard]] ExitCode code() const noexcept
    {
        return code_;
    }

private:
    ExitCode code_;
};

} // namespace pdi
