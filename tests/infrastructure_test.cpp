#include "pdi/contract.hpp"
#include "pdi/image_io.hpp"
#include "pdi/kernel.hpp"
#include "pdi/parameters.hpp"
#include "pdi/result_io.hpp"

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main()
{
    namespace fs = std::filesystem;

    // Validação de parâmetros tipados e regras do contrato.
    pdi::CliOptions quantize;
    quantize.operation = "quantize";
    quantize.input = "images/input/m1_gray_ramp_256.png";
    quantize.output = "images/output/out.png";
    quantize.parameters["levels"] = "8";
    assert(pdi::validate_contract(quantize));
    assert(pdi::parameter_as_int(quantize, "levels") == 8);

    quantize.parameters["levels"] = "3";
    assert(!pdi::validate_contract(quantize));

    pdi::CliOptions missing_input;
    missing_input.operation = "negative";
    missing_input.input = "images/input/nao_existe.png";
    missing_input.output = "images/output/out.png";
    const auto missing_validation = pdi::validate_contract(missing_input);
    assert(!missing_validation);
    assert(missing_validation.code == pdi::ExitCode::read_error);

    pdi::CliOptions convolution;
    convolution.operation = "convolution";
    convolution.input = "images/input/m1_gray_5x5.png";
    convolution.output = "images/output/out.png";
    convolution.parameters["kernel"] = "kernels/identity_3x3.txt";
    convolution.parameters["border"] = "replicate";
    assert(pdi::validate_contract(convolution));
    assert(pdi::parameter_as_border(convolution) == pdi::BorderStrategy::replicate);

    // O teste é executado com a raiz do projeto C++ como working directory.
    // Assim ele pode usar uma das imagens públicas sem depender de um caminho
    // absoluto específico da máquina do estudante.
    const auto image = pdi::read_image("images/input/m1_color_2x2.png");
    assert(!image.empty());
    assert(image.rows == 2);
    assert(image.cols == 2);

    const fs::path root = fs::current_path() / "pdi_test_tmp";
    fs::remove_all(root);
    fs::create_directories(root);

    // A gravação aqui apenas testa a infraestrutura de I/O. Ela não substitui
    // a implementação da cópia manual exigida no M1.1.
    const fs::path image_path = root / "nested" / "image.png";
    pdi::write_image(image_path.string(), image);
    assert(fs::exists(image_path));

    const fs::path kernel_path = root / "identity.txt";
    {
        std::ofstream kernel_file(kernel_path);
        kernel_file << "0 0 0\n0 1 0\n0 0 0\n";
    }

    const auto kernel = pdi::read_kernel(kernel_path);
    assert(kernel.size() == 3);
    assert(kernel.radius() == 1);
    assert(kernel.at(1, 1) == 1.0);

    std::array<std::uint64_t, 256> histogram{};
    histogram[0] = 2;
    histogram[255] = 1;
    const fs::path csv_path = root / "nested" / "histogram.csv";
    pdi::write_histogram_csv(csv_path, histogram);
    assert(fs::exists(csv_path));

    const fs::path json_path = root / "nested" / "metadata.json";
    pdi::write_json_object(json_path, {{"operation", "test"}, {"status", "ok"}});
    assert(fs::exists(json_path));

    fs::remove_all(root);
    return 0;
}
