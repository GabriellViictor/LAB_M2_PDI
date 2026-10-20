#include "pdi/image_io.hpp"
#include "pdi/errors.hpp"
#include "pdi/result_io.hpp"

#include <opencv2/imgcodecs.hpp>
#include <stdexcept>

namespace pdi {

cv::Mat read_image(const std::string& path)
{
    // IMREAD_UNCHANGED preserva o número de canais e a profundidade do arquivo.
    // Isso é importante no M1.1, pois o estudante precisa observar como a
    // representação da imagem influencia o acesso aos pixels.
    cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

    if (image.empty()) {
        throw PdiError(ExitCode::read_error, "Nao foi possivel abrir a imagem: " + path);
    }

    return image;
}

void write_image(const std::string& path, const cv::Mat& image)
{
    if (image.empty()) {
        throw PdiError(ExitCode::write_error, "A imagem de saida esta vazia.");
    }

    try {
        // A infraestrutura cria o diretório-pai quando necessário para que o
        // estudante possa concentrar-se no algoritmo e não em detalhes de I/O.
        ensure_parent_directory(path);
    } catch (const std::exception& error) {
        throw PdiError(
            ExitCode::write_error,
            "Nao foi possivel preparar o caminho de saida: " + std::string(error.what())
        );
    }

    if (!cv::imwrite(path, image)) {
        throw PdiError(ExitCode::write_error, "Nao foi possivel salvar a imagem: " + path);
    }
}

} // namespace pdi
