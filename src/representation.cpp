#include "pdi/representation.hpp"

#include <iostream>
#include <ostream>
#include <opencv2/core/core.hpp>
#include <iomanip>

using namespace std;
namespace pdi {

    static string getMatTypeStr(const cv::Mat& img) {
        int imgType = img.type();

        uchar depth = imgType & CV_MAT_DEPTH_MASK;
        int chns = 1 + (imgType >> CV_CN_SHIFT);

        string r;

        switch (depth) {
            case CV_8U:  r = "8U"; break;
            case CV_8S:  r = "8S"; break;
            case CV_16U: r = "16U"; break;
            case CV_16S: r = "16S"; break;
            case CV_32S: r = "32S"; break;
            case CV_32F: r = "32F"; break;
            case CV_64F: r = "64F"; break;
            default:     r = "User"; break;
        }

        return "CV_" + r + "C" + to_string(chns);
    }

    void inspectImage(const cv::Mat& img) {
        if (img.empty()) {
            cerr << "Erro: Imagem vazia ou invalida para inspecao. \n" << endl;
            return;
        }

        int width = img.cols;
        int height = img.rows;
        int channels = img.channels();
        size_t total_pixels = static_cast<size_t>(width) * static_cast<size_t>(height);

        cout<< "largura: " << width << "px\n";
        cout<< "altura: " << height << "px\n";
        cout<< "canais: " << channels << "\n";
        cout<< "tipo: " << getMatTypeStr(img) << endl;
        cout<< "total pixels: " << total_pixels << "\n";

        if (channels == 1) {
            int min_val = 255;
            int max_val = 0;

            uint64_t sum = 0;

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    uchar pixel = img.at<uchar>(y, x);

                    if (pixel > max_val) max_val = pixel;
                    if (pixel < min_val) min_val = pixel;
                    sum += pixel;
                }
            }

            double mean = static_cast<double>(sum) / static_cast<double>(total_pixels);
            cout << fixed << std::setprecision(2);
            cout << "Minimo: " << min_val << "\n";
            cout << "Maximo: " << max_val << "\n";
            cout << "Media: " << mean << "\n";

        }else if (img.channels() == 3) {
            int min_b = 255, max_b = 0;
            int min_g = 255, max_g = 0;
            int min_r = 255, max_r = 0;

            uint64_t sum_b = 0, sum_g = 0, sum_r = 0;

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    cv::Vec3b pixel = img.at<cv::Vec3b>(y, x);
                    uchar b = pixel[0];
                    uchar g = pixel[1];
                    uchar r = pixel[2];

                    //pega o azul
                    if (b > max_b) max_b = b;
                    if (b < min_b) min_b = b;
                    sum_b += b;

                    //pega o verde
                    if (g > max_g) max_g = g;
                    if (g < min_g) min_g = g;
                    sum_g += g;

                    //pega o vermelho
                    if (r > max_r) max_r = r;
                    if (r < min_r) min_r = r;
                    sum_r += r;
                }
            }

            double mean_b = static_cast<double>(sum_b) / static_cast<double>(total_pixels);
            double mean_g = static_cast<double>(sum_g) / static_cast<double>(total_pixels);
            double mean_r = static_cast<double>(sum_r) / static_cast<double>(total_pixels);

            cout << fixed << setprecision(2);
            cout << "Canal Azul (B)     - Min: " << min_b << " | Max: " << max_b << " | Media: " << mean_b << "\n";
            cout << "Canal Verde (G)    - Min: " << min_g << " | Max: " << max_g << " | Media: " << mean_g << "\n";
            cout << "Canal Vermelho (R) - Min: " << min_r << " | Max: " << max_r << " | Media: " << mean_r << "\n";

        }else {
            cerr<< "aviso: imagem não aceita (" << channels << "). \n";
        }
    }

    cv::Mat copyImage(const cv::Mat& src) {
        if (src.empty()) {
            cerr << "Erro: Imagem vazia ou invalida para copia.\n" << endl;
            return cv::Mat();
        }

        int cols = src.cols;
        int rows = src.rows;
        int channels = src.channels();

        cv::Mat dst = cv::Mat::zeros(src.size(), src.type());

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (channels == 1) {
                    dst.at<uchar>(y, x) = src.at<uchar>(y, x);
                }
                else if (channels == 3) {
                    cv::Vec3b pixel = src.at<cv::Vec3b>(y, x);
                    dst.at<cv::Vec3b>(y, x) = pixel;
                }
            }
        }

        return dst;
    }

    cv::Mat extractChannel(const cv::Mat& src, int channelIndex) {
        if (src.empty()) {
            cerr << "Erro: Imagem vazia ou invalida para separacao de canal.\n";
            return cv::Mat();
        }

        if (src.channels() != 3) {
            cerr << "Erro: A imagem deve ser colorida (3 canais).\n";
            return cv::Mat();
        }

        if (channelIndex < 0 || channelIndex > 2) {
            cerr << "Erro: Indice de canal invalido. Use 0 (B), 1 (G) ou 2 (R).\n";
            return cv::Mat();
        }

        int rows = src.rows;
        int cols = src.cols;

        // cira matriz toda zerada
        cv::Mat dst = cv::Mat::zeros(src.size(), src.type());

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                // Lê o pixel colorido da imagem de origem
                cv::Vec3b pixel = src.at<cv::Vec3b>(y, x);

                // Grava apenas o canal escolhido no destino
                dst.at<cv::Vec3b>(y, x)[channelIndex] = pixel[channelIndex];
            }
        }

        return dst;
    }

    cv::Mat grayScaleAvg(const cv::Mat& src) {
        if (src.empty()) {
            cerr << "Erro: Imagem vazia ou invalida para conversao.\n" << endl;
            return cv::Mat();
        }

        if (src.channels() != 3) {
            cerr << "Erro: A imagem deve ser colorida (3 canais).\n" << endl;
            return cv::Mat();
        }

        cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1); //sai só com um canal

        for (int y = 0; y < src.rows; y++) {
            for (int x = 0; x < src.cols; x++) {
                cv::Vec3b pixel = src.at<cv::Vec3b>(y, x);

                int b = pixel[0];
                int g = pixel[1];
                int r = pixel[2];

                int gray = (b + g + r) / 3;

                dst.at<uchar>(y, x) = static_cast<uchar>(gray);
            }
        }
        return dst;
    }

    cv::Mat grayscaleWeighted(const cv::Mat& src) {
        if (src.empty()) {
            cerr << "Erro: Imagem vazia ou invalida para conversao.\n" << endl;
            return cv::Mat();
        }

        if (src.channels() != 3) {
            cerr << "Erro: A imagem deve ser colorida (3 canais).\n" << endl;
            return cv::Mat();
        }

        cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1);
        for (int y = 0; y < src.rows; y++) {
            for (int x = 0; x < src.cols; x++) {
                cv::Vec3b pixel = src.at<cv::Vec3b>(y, x);

                double b = pixel[0];
                double g = pixel[1];
                double r = pixel[2];

                double gray = 0.299 * r + 0.587 * g + 0.114 * b;
                dst.at<uchar>(y, x) = static_cast<uchar>(std::round(gray));
            }
        }
        return dst;
    }

    cv::Mat quantImg(const cv::Mat& src, int levels) {
        if (src.empty()) {
            cerr << "Erro: Imagem vazia ou invalida para quantizacao.\n" << endl;
            return cv::Mat();
        }

        if (src.channels() != 1) {
            cerr<< "A quantização deve ser feita em imagem em escala de cinza (1 canal)\n";
            return cv::Mat();
        }

        if (levels != 2 && levels != 4 && levels != 8 && levels != 16) {
            std::cerr << "Erro: Quantidade de niveis invalida (use 2, 4, 8 ou 16).\n";
            return cv::Mat();
        }

        int binSize = 256 / levels;
        double scale = 255.0 / (levels - 1);

        cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1);

        for (int y = 0; y < src.rows; y++) {
            for (int x = 0; x < src.cols; x++) {
                uchar pixel = src.at<uchar>(y, x);

                int binIndex = pixel / binSize;
                if (binIndex >= levels) {
                    binIndex = levels - 1;
                }

                int quantizedVal = static_cast<int>(std::round(binIndex * scale));
                dst.at<uchar>(y, x) = static_cast<uchar>(quantizedVal);
            }
        }
        return dst;
    }
}
