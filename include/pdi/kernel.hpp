#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace pdi {

// Representação simples de um kernel quadrado.
// A leitura e a validação estrutural são fornecidas pelo projeto-base;
// aplicar o kernel sobre a vizinhança continua sendo responsabilidade
// do estudante no Laboratório M1.3.
class Kernel {
public:
    Kernel(std::size_t size, std::vector<double> values);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t radius() const noexcept;
    [[nodiscard]] double at(std::size_t row, std::size_t col) const;

private:
    std::size_t size_;
    std::vector<double> values_;
};

Kernel read_kernel(const std::filesystem::path& path);

} // namespace pdi
