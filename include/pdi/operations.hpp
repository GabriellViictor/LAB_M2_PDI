#pragma once
#include "pdi/cli.hpp"
#include "pdi/exit_code.hpp"
#include <string_view>
namespace pdi { bool is_known_operation(std::string_view operation); ExitCode run_operation(const CliOptions& options); }
