#pragma once

namespace pdi {

// Códigos de saída padronizados entre C++, Java e Python.
enum class ExitCode : int {
    success = 0,
    general_error = 1,
    invalid_arguments = 2,
    read_error = 3,
    write_error = 4,
    invalid_parameter = 5,
    unknown_operation = 6
};

} // namespace pdi
