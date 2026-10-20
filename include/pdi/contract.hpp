#pragma once
#include "pdi/cli.hpp"
#include "pdi/exit_code.hpp"
namespace pdi {
struct ValidationResult {
    ExitCode code = ExitCode::success;
    std::string message;
    explicit operator bool() const { return code == ExitCode::success; }
};
ValidationResult validate_contract(const CliOptions& options);
}
