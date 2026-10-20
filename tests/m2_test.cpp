#include "pdi/cli.hpp"
#include "pdi/components.hpp"
#include "pdi/contract.hpp"
#include "pdi/errors.hpp"
#include "pdi/image_io.hpp"
#include "pdi/morphology.hpp"
#include "pdi/operations.hpp"
#include "pdi/parameters.hpp"
#include "pdi/segmentation.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// Gera as duas imagens de estudo da M2 caso ainda nao existam em images/input/,
// garantindo reprodutibilidade deterministica dos testes.
void ensure_study_images_exist()
{
    fs::create_directories("images/input");

    // Imagem 1: Condicoes favoraveis (fundo escuro uniforme, 4 objetos claros bem separados,
    // com pequeno ruido isolado de 1px e 1 pequeno furo interno para testar a morfologia no M2.2).
    const fs::path fav_path = "images/input/m2_favorable_objects.png";
    if (!fs::exists(fav_path)) {
        cv::Mat fav(200, 260, CV_8UC3, cv::Scalar(25, 28, 30));
        // 4 discos claros (objetos de interesse)
        cv::circle(fav, cv::Point(60, 60), 28, cv::Scalar(215, 220, 225), cv::FILLED);
        cv::circle(fav, cv::Point(180, 60), 25, cv::Scalar(210, 215, 220), cv::FILLED);
        cv::circle(fav, cv::Point(65, 145), 24, cv::Scalar(205, 210, 215), cv::FILLED);
        cv::circle(fav, cv::Point(185, 145), 30, cv::Scalar(220, 225, 230), cv::FILLED);

        // Pequenos furos escuros internos (simulando reflexo/defeito na mascara inicial)
        cv::circle(fav, cv::Point(60, 60), 1, cv::Scalar(20, 20, 20), cv::FILLED);
        cv::circle(fav, cv::Point(185, 145), 1, cv::Scalar(20, 20, 20), cv::FILLED);

        // Pontos isolados de ruido claro (1x1) no fundo
        fav.at<cv::Vec3b>(15, 120) = cv::Vec3b(230, 230, 230);
        fav.at<cv::Vec3b>(105, 130) = cv::Vec3b(225, 225, 225);
        fav.at<cv::Vec3b>(185, 20) = cv::Vec3b(235, 235, 235);

        pdi::write_image(fav_path.string(), fav);
    }

    // Imagem 2: Condicoes desafiadoras (gradiente forte de iluminacao/sombra lateral,
    // ruido de fundo e dois objetos muito proximos / quase em contato).
    const fs::path chal_path = "images/input/m2_challenging_illumination.png";
    if (!fs::exists(chal_path)) {
        cv::Mat chal(200, 260, CV_8UC3);
        for (int r = 0; r < chal.rows; ++r) {
            for (int c = 0; c < chal.cols; ++c) {
                // Gradiente horizontal de iluminacao de 20 ate 165 no fundo
                const int bg = 20 + (145 * c) / chal.cols;
                chal.at<cv::Vec3b>(r, c) = cv::Vec3b(
                    static_cast<uchar>(bg),
                    static_cast<uchar>(bg),
                    static_cast<uchar>(bg)
                );
            }
        }

        // Objetos claros com contraste local +55 sobre o fundo local (na sombra valem ~95, no lado claro valem ~220)
        auto draw_local_disk = [&](cv::Point center, int radius, int delta) {
            for (int r = 0; r < chal.rows; ++r) {
                for (int c = 0; c < chal.cols; ++c) {
                    const int dx = c - center.x;
                    const int dy = r - center.y;
                    if (dx * dx + dy * dy <= radius * radius) {
                        for (int ch = 0; ch < 3; ++ch) {
                            const int val = std::min(255, static_cast<int>(chal.at<cv::Vec3b>(r, c)[ch]) + delta);
                            chal.at<cv::Vec3b>(r, c)[ch] = static_cast<uchar>(val);
                        }
                    }
                }
            }
        };

        draw_local_disk(cv::Point(50, 55), 22, 65);   // Objeto na regiao escura (sombra)
        draw_local_disk(cv::Point(55, 145), 22, 65);  // Outro objeto na regiao escura
        draw_local_disk(cv::Point(175, 70), 20, 65);  // Par de objetos muito proximos na regiao iluminada
        draw_local_disk(cv::Point(217, 70), 20, 65);  // (separados por apenas 2px -> fechamento pode fundi-los!)
        draw_local_disk(cv::Point(195, 150), 24, 65); // Quinto objeto

        pdi::write_image(chal_path.string(), chal);
    }
}

void test_m2_1_synthetic_fixed_threshold()
{
    // Imagem 3x3 sintetica com valores < 120, == 120 e > 120
    cv::Mat synth = (cv::Mat_<uchar>(3, 3) <<
        0,   50,  119,
        120, 121, 180,
        200, 254, 255
    );

    pdi::SegmentationParams params;
    params.method = pdi::SegmentationMethod::fixed;
    params.threshold = 120;
    params.invert = false;

    const auto res = pdi::segmentImage(synth, params);
    assert(pdi::isValidBinaryMask(res.mask, 3, 3));

    // < 120 -> 0
    assert(res.mask.at<uchar>(0, 0) == 0);
    assert(res.mask.at<uchar>(0, 1) == 0);
    assert(res.mask.at<uchar>(0, 2) == 0);
    // >= 120 -> 255
    assert(res.mask.at<uchar>(1, 0) == 255);
    assert(res.mask.at<uchar>(1, 1) == 255);
    assert(res.mask.at<uchar>(1, 2) == 255);
    assert(res.mask.at<uchar>(2, 0) == 255);
    assert(res.mask.at<uchar>(2, 1) == 255);
    assert(res.mask.at<uchar>(2, 2) == 255);
    assert(res.foreground_pixels == 6);
    assert(res.total_pixels == 9);

    // Teste com inversao (--invert true)
    params.invert = true;
    const auto res_inv = pdi::segmentImage(synth, params);
    assert(pdi::isValidBinaryMask(res_inv.mask, 3, 3));
    assert(res_inv.foreground_pixels == 3);
    assert(res_inv.mask.at<uchar>(0, 2) == 255);
    assert(res_inv.mask.at<uchar>(1, 0) == 0);
}

void test_m2_1_other_methods_and_hsv_wraparound()
{
    // Teste Otsu e Adaptive
    cv::Mat bimodal(40, 40, CV_8UC1, cv::Scalar(30));
    cv::rectangle(bimodal, cv::Rect(10, 10, 20, 20), cv::Scalar(220), cv::FILLED);

    pdi::SegmentationParams otsu_params;
    otsu_params.method = pdi::SegmentationMethod::otsu;
    const auto otsu_res = pdi::segmentImage(bimodal, otsu_params);
    assert(pdi::isValidBinaryMask(otsu_res.mask, 40, 40));
    assert(otsu_res.foreground_pixels == 400);

    pdi::SegmentationParams adap_params;
    adap_params.method = pdi::SegmentationMethod::adaptive;
    adap_params.block_size = 15;
    adap_params.c_constant = -5.0;
    const auto adap_res = pdi::segmentImage(bimodal, adap_params);
    assert(pdi::isValidBinaryMask(adap_res.mask, 40, 40));

    // Teste HSV incluindo cruzamento da descontinuidade do matiz (vermelho H=175 e H=5)
    cv::Mat hsv_synth(10, 10, CV_8UC3, cv::Scalar(60, 200, 200)); // Verde em HSV
    hsv_synth.at<cv::Vec3b>(2, 2) = cv::Vec3b(175, 220, 220);     // Vermelho faixa alta
    hsv_synth.at<cv::Vec3b>(7, 7) = cv::Vec3b(5, 220, 220);       // Vermelho faixa baixa
    cv::Mat bgr_synth;
    cv::cvtColor(hsv_synth, bgr_synth, cv::COLOR_HSV2BGR);

    pdi::SegmentationParams hsv_params;
    hsv_params.method = pdi::SegmentationMethod::hsv;
    hsv_params.h_min = 170;
    hsv_params.h_max = 10; // h_min > h_max -> wrap-around!
    hsv_params.s_min = 100;
    hsv_params.s_max = 255;
    hsv_params.v_min = 100;
    hsv_params.v_max = 255;

    const auto hsv_res = pdi::segmentImage(bgr_synth, hsv_params);
    assert(pdi::isValidBinaryMask(hsv_res.mask, 10, 10));
    assert(hsv_res.mask.at<uchar>(2, 2) == 255);
    assert(hsv_res.mask.at<uchar>(7, 7) == 255);
    assert(hsv_res.mask.at<uchar>(0, 0) == 0);
    assert(hsv_res.foreground_pixels == 2);
}

void test_m2_2_diagonal_connectivity_4_vs_8()
{
    // Mascara sintetica com dois pixels apenas na diagonal:
    // Deve contar 2 componentes em conectividade 4 e 1 componente em conectividade 8.
    cv::Mat diag = cv::Mat::zeros(5, 5, CV_8UC1);
    diag.at<uchar>(1, 1) = 255;
    diag.at<uchar>(2, 2) = 255;

    const auto rep4 = pdi::analyzeConnectedComponents(diag, 4, 0);
    const auto rep8 = pdi::analyzeConnectedComponents(diag, 8, 0);

    assert(rep4.raw_count == 2);
    assert(rep4.raw_foreground_area == 2);
    assert(rep8.raw_count == 1);
    assert(rep8.raw_foreground_area == 2);
}

void test_m2_2_morphology_opening_and_closing()
{
    // Mascara sintetica 30x30 com:
    // - Regiao principal 13x13 em (8..20, 8..20)
    // - Furo interno 1x1 em (14, 14)
    // - Ruido isolado 1x1 em (2, 2)
    cv::Mat mask = cv::Mat::zeros(30, 30, CV_8UC1);
    cv::rectangle(mask, cv::Rect(8, 8, 13, 13), cv::Scalar(255), cv::FILLED);
    mask.at<uchar>(14, 14) = 0; // Furo pequeno interno
    mask.at<uchar>(2, 2) = 255; // Ruido isolado externo

    const auto initial_rep = pdi::analyzeConnectedComponents(mask, 8, 0);
    assert(initial_rep.raw_count == 2); // Objeto principal + ruido isolado
    assert(initial_rep.raw_foreground_area == (13 * 13 - 1 + 1));

    // Estagio 1: Abertura 3x3 remove o ruido isolado em (2,2)
    const pdi::MorphStageConfig open_stage{pdi::MorphOp::open, pdi::StructuringShape::rect, 3, 1};
    const cv::Mat after_open = pdi::applyMorphStage(mask, open_stage);
    assert(after_open.at<uchar>(2, 2) == 0);
    const auto open_rep = pdi::analyzeConnectedComponents(after_open, 8, 0);
    assert(open_rep.raw_count == 1);

    // Estagio 2: Fechamento 3x3 fecha o furo interno em (14,14)
    const pdi::MorphStageConfig close_stage{pdi::MorphOp::close, pdi::StructuringShape::rect, 3, 1};
    const cv::Mat after_close = pdi::applyMorphStage(after_open, close_stage);
    assert(after_close.at<uchar>(14, 14) == 255);
    const auto close_rep = pdi::analyzeConnectedComponents(after_close, 8, 0);
    assert(close_rep.raw_count == 1);
    assert(close_rep.raw_foreground_area == 13 * 13);
}

void test_m2_2_empty_mask_and_border_touching()
{
    // Mascara vazia -> contagem 0
    cv::Mat empty_mask = cv::Mat::zeros(16, 16, CV_8UC1);
    const auto empty_rep = pdi::analyzeConnectedComponents(empty_mask, 8, 0);
    assert(empty_rep.raw_count == 0);
    assert(empty_rep.filtered_count == 0);
    assert(empty_rep.raw_foreground_area == 0);
    assert(empty_rep.components.empty());

    // Objeto tocando as bordas superior e esquerda (0,0)
    cv::Mat border_mask = cv::Mat::zeros(16, 16, CV_8UC1);
    cv::rectangle(border_mask, cv::Rect(0, 0, 5, 4), cv::Scalar(255), cv::FILLED);
    const auto border_rep = pdi::analyzeConnectedComponents(border_mask, 8, 0);
    assert(border_rep.raw_count == 1);
    assert(border_rep.raw_foreground_area == 20);
    assert(border_rep.components[0].x == 0);
    assert(border_rep.components[0].y == 0);
    assert(border_rep.components[0].width == 5);
    assert(border_rep.components[0].height == 4);
}

void test_error_guards_and_exit_codes()
{
    // 1. Falha de leitura (arquivo inexistente)
    pdi::CliOptions missing_in;
    missing_in.operation = "segment";
    missing_in.input = "images/input/arquivo_inexistente_123.png";
    missing_in.output = "images/output/tmp.png";
    missing_in.parameters["method"] = "fixed";
    missing_in.parameters["threshold"] = "120";
    const auto v1 = pdi::validate_contract(missing_in);
    assert(!v1);
    assert(v1.code == pdi::ExitCode::read_error);

    // 2. Metodo desconhecido em segment
    pdi::CliOptions bad_method;
    bad_method.operation = "segment";
    bad_method.input = "images/input/m1_gray_5x5.png";
    bad_method.output = "images/output/tmp.png";
    bad_method.parameters["method"] = "watershed_invalido";
    const auto v2 = pdi::validate_contract(bad_method);
    assert(!v2);
    assert(v2.code == pdi::ExitCode::invalid_parameter);

    // 3. Limiar fora do intervalo [0..255]
    pdi::CliOptions bad_thresh;
    bad_thresh.operation = "segment";
    bad_thresh.input = "images/input/m1_gray_5x5.png";
    bad_thresh.output = "images/output/tmp.png";
    bad_thresh.parameters["method"] = "fixed";
    bad_thresh.parameters["threshold"] = "300";
    const auto v3 = pdi::validate_contract(bad_thresh);
    assert(!v3);
    assert(v3.code == pdi::ExitCode::invalid_parameter);

    // 4. Proibicao de JPEG como formato de saida da mascara binaria
    pdi::CliOptions bad_jpeg;
    bad_jpeg.operation = "segment";
    bad_jpeg.input = "images/input/m1_gray_5x5.png";
    bad_jpeg.output = "images/output/tmp.jpg";
    bad_jpeg.parameters["method"] = "otsu";
    const auto v4 = pdi::validate_contract(bad_jpeg);
    assert(!v4);
    assert(v4.code == pdi::ExitCode::invalid_parameter);

    // 5. Conectividade invalida em analyze (--connectivity 6)
    pdi::CliOptions bad_conn;
    bad_conn.operation = "analyze";
    bad_conn.input = "images/input/m1_gray_5x5.png";
    bad_conn.output = "results/tmp";
    bad_conn.parameters["connectivity"] = "6";
    const auto v5 = pdi::validate_contract(bad_conn);
    assert(!v5);
    assert(v5.code == pdi::ExitCode::invalid_parameter);

    // 6. Sequencia morfologica invalida (repeticao sem finalidade distinta)
    pdi::CliOptions bad_morph;
    bad_morph.operation = "analyze";
    bad_morph.input = "images/input/m1_gray_5x5.png";
    bad_morph.output = "results/tmp";
    bad_morph.parameters["morph-seq"] = "open,open";
    bad_morph.parameters["morph-sizes"] = "3,3";
    const auto v6 = pdi::validate_contract(bad_morph);
    assert(!v6);
    assert(v6.code == pdi::ExitCode::invalid_parameter);
}

void test_end_to_end_study_images_and_area_conservation()
{
    ensure_study_images_exist();

    const fs::path tmp_dir = fs::current_path() / "pdi_m2_test_tmp";
    fs::remove_all(tmp_dir);
    fs::create_directories(tmp_dir);

    const std::vector<std::string> inputs {
        "images/input/m2_favorable_objects.png",
        "images/input/m2_challenging_illumination.png"
    };

    for (std::size_t idx = 0; idx < inputs.size(); ++idx) {
        const std::string tag = "case" + std::to_string(idx + 1);
        const fs::path mask_fixed = tmp_dir / (tag + "_fixed.png");
        const fs::path mask_adapt = tmp_dir / (tag + "_adaptive.png");

        // Configuracao 1: Limiar fixo
        pdi::CliOptions seg_fixed;
        seg_fixed.operation = "segment";
        seg_fixed.input = inputs[idx];
        seg_fixed.output = mask_fixed.string();
        seg_fixed.parameters["method"] = "fixed";
        seg_fixed.parameters["threshold"] = "130";
        assert(pdi::validate_contract(seg_fixed));
        assert(pdi::run_operation(seg_fixed) == pdi::ExitCode::success);
        assert(fs::exists(mask_fixed));

        const cv::Mat m1 = pdi::read_image(mask_fixed.string());
        assert(pdi::isValidBinaryMask(m1));

        // Configuracao 2: Limiarizacao adaptativa
        pdi::CliOptions seg_adapt;
        seg_adapt.operation = "segment";
        seg_adapt.input = inputs[idx];
        seg_adapt.output = mask_adapt.string();
        seg_adapt.parameters["method"] = "adaptive";
        seg_adapt.parameters["block-size"] = "31";
        seg_adapt.parameters["c"] = "-15";
        seg_adapt.parameters["blur"] = "3";
        assert(pdi::validate_contract(seg_adapt));
        assert(pdi::run_operation(seg_adapt) == pdi::ExitCode::success);
        assert(fs::exists(mask_adapt));

        const cv::Mat m2 = pdi::read_image(mask_adapt.string());
        assert(pdi::isValidBinaryMask(m2));

        // Executa analyze (M2.2) sobre a mascara produzida
        const fs::path prefix = tmp_dir / (tag + "_analysis");
        pdi::CliOptions analyze_opts;
        analyze_opts.operation = "analyze";
        analyze_opts.input = mask_adapt.string();
        analyze_opts.output = prefix.string();
        analyze_opts.parameters["connectivity"] = "8";
        analyze_opts.parameters["min-area"] = "20";
        assert(pdi::validate_contract(analyze_opts));
        assert(pdi::run_operation(analyze_opts) == pdi::ExitCode::success);

        assert(fs::exists(prefix.string() + "_initial.csv"));
        assert(fs::exists(prefix.string() + "_final.csv"));
        assert(fs::exists(prefix.string() + "_labels.png"));
        assert(fs::exists(prefix.string() + "_summary.json"));
        assert(fs::exists(prefix.string() + "_stage1_open.png"));
        assert(fs::exists(prefix.string() + "_stage2_close.png"));

        // Verifica que a soma das areas dos componentes brutos corresponde exatamente
        // a quantidade de pixels de primeiro plano (255) da mascara respectiva, antes do filtro.
        const cv::Mat init_mask = pdi::read_image(prefix.string() + "_initial_mask.png");
        const auto init_rep = pdi::analyzeConnectedComponents(init_mask, 8, 20);
        const auto init_nonzero = static_cast<std::uint64_t>(cv::countNonZero(init_mask));
        assert(init_rep.raw_foreground_area == init_nonzero);

        const cv::Mat stage2_mask = pdi::read_image(prefix.string() + "_stage2_close.png");
        const auto final_rep = pdi::analyzeConnectedComponents(stage2_mask, 8, 20);
        const auto final_nonzero = static_cast<std::uint64_t>(cv::countNonZero(stage2_mask));
        assert(final_rep.raw_foreground_area == final_nonzero);
    }

    fs::remove_all(tmp_dir);
}

} // namespace

int main()
{
    test_m2_1_synthetic_fixed_threshold();
    test_m2_1_other_methods_and_hsv_wraparound();
    test_m2_2_diagonal_connectivity_4_vs_8();
    test_m2_2_morphology_opening_and_closing();
    test_m2_2_empty_mask_and_border_touching();
    test_error_guards_and_exit_codes();
    test_end_to_end_study_images_and_area_conservation();

    std::cout << "Todos os testes do Laboratorio M2 (M2.1 + M2.2) passaram com sucesso.\n";
    return 0;
}
