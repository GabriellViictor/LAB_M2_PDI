#include "pdi/kernel.hpp"
#include "pdi/errors.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace pdi {
namespace {

[[noreturn]] void invalid_kernel(const std::string& message)
{
    throw PdiError(ExitCode::invalid_parameter, message);
}

} // namespace

Kernel::Kernel(std::size_t size, std::vector<double> values)
    : size_(size), values_(std::move(values))
{
    if (size_ == 0 || size_ % 2 == 0) {
        invalid_kernel("O kernel deve ter dimensao impar e maior que zero.");
    }
    if (values_.size() != size_ * size_) {
        invalid_kernel("A quantidade de coeficientes nao corresponde ao tamanho do kernel.");
    }
}

std::size_t Kernel::size() const noexcept
{
    return size_;
}

std::size_t Kernel::radius() const noexcept
{
    return size_ / 2;
}

double Kernel::at(std::size_t row, std::size_t col) const
{
    if (row >= size_ || col >= size_) {
        throw std::out_of_range("Indice fora dos limites do kernel.");
    }
    return values_.at(row * size_ + col);
}

Kernel read_kernel(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) {
        throw PdiError(ExitCode::read_error, "Nao foi possivel abrir o kernel: " + path.string());
    }

    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        std::istringstream tokens(line);
        std::vector<double> row;
        std::string token;
        while (tokens >> token) {
            std::size_t consumed = 0;
            double value = 0.0;
            try {
                value = std::stod(token, &consumed);
            } catch (const std::exception&) {
                invalid_kernel("Coeficiente invalido no kernel: " + token);
            }
            if (consumed != token.size()) {
                invalid_kernel("Coeficiente invalido no kernel: " + token);
            }
            row.push_back(value);
        }

        if (!row.empty()) {
            rows.push_back(std::move(row));
        }
    }

    if (rows.empty()) {
        invalid_kernel("O arquivo de kernel esta vazio.");
    }

    if (rows.size() > 1 && rows.front().size() == 2) {
        std::size_t h = static_cast<std::size_t>(rows[0][0]);
        std::size_t w = static_cast<std::size_t>(rows[0][1]);
        if (h != w) {
            invalid_kernel("O kernel deve ser quadrado.");
        }
        rows.erase(rows.begin());
    }

    const auto size = rows.front().size();
    if (size == 0 || rows.size() != size) {
        invalid_kernel("O kernel deve ser quadrado.");
    }
    if (size % 2 == 0) {
        invalid_kernel("O kernel deve possuir dimensao impar.");
    }

    std::vector<double> flattened;
    flattened.reserve(size * size);
    for (const auto& row : rows) {
        if (row.size() != size) {
            invalid_kernel("Todas as linhas do kernel devem ter o mesmo tamanho.");
        }
        flattened.insert(flattened.end(), row.begin(), row.end());
    }

    return Kernel(size, std::move(flattened));
}

} // namespace pdi
