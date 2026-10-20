#include "pdi/cli.hpp"
#include "pdi/contract.hpp"
#include "pdi/errors.hpp"
#include "pdi/exit_code.hpp"
#include "pdi/operations.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    try {
        const auto options = pdi::parse_cli(argc, argv);

        if (options.help) {
            pdi::print_help();
            return static_cast<int>(pdi::ExitCode::success);
        }
        if (options.version) {
            pdi::print_version();
            return static_cast<int>(pdi::ExitCode::success);
        }

        const auto validation = pdi::validate_contract(options);
        if (!validation) {
            std::cerr << "Erro: " << validation.message << '\n';
            return static_cast<int>(validation.code);
        }

        return static_cast<int>(pdi::run_operation(options));
    } catch (const pdi::PdiError& error) {
        std::cerr << "Erro: " << error.what() << '\n';
        return static_cast<int>(error.code());
    } catch (const std::invalid_argument& error) {
        std::cerr << "Erro de argumentos: " << error.what() << '\n';
        return static_cast<int>(pdi::ExitCode::invalid_arguments);
    } catch (const std::exception& error) {
        std::cerr << "Erro: " << error.what() << '\n';
        return static_cast<int>(pdi::ExitCode::general_error);
    }
}
