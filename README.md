# Laboratórios de Processamento Digital de Imagens - M2 (M2.1 + M2.2)

- **Disciplina:** Processamento Digital de Imagens (2026-02)
- **Estudante:** Gabriel Victor Garcia Teixeira (`7701594`)
- **Linguagem:** C++20 (`CMake` + `OpenCV`)
- **Identificador do Laboratório:** `m2.2` (entrega consolidada contemplando M1, M2.1 e M2.2 no executável `pdi_lab`)

---

## 1. Requisitos e Ambiente

- Compilador C++20 (`GCC/MinGW UCRT64` ou `Clang`/`MSVC`)
- `CMake >= 3.24` e `Ninja`
- `OpenCV 4.x` (módulos `core`, `imgcodecs`, `imgproc`)

### Compilação e Testes

```bash
cmake --preset ucrt64-debug
cmake --build --preset ucrt64-debug
ctest --preset ucrt64-debug --output-on-failure
```

Caso utilize uma distribuição portátil do OpenCV no Windows:
```bash
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPDI_OPENCV_DIR="C:/OpenCV-MinGW-Build-OpenCV-4.5.5-x64"
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

---

## 2. Operações da M2

### 2.1. Laboratório M2.1 - Segmentação e Máscara Binária (`--operation segment`)

Gera uma máscara binária de 1 canal (`CV_8UC1`) contendo exclusivamente `{0, 255}` (`0` = fundo, `255` = objeto) em formato sem perdas (`PNG`).

#### Parâmetros aceitos:
- `--method <fixed|otsu|adaptive|hsv>` *(obrigatório)*
- `--threshold <0..255>` *(obrigatório para `--method fixed`; regra `pixel >= T ? 255 : 0`)*
- `--invert <true|false>` *(opcional; inverte a máscara quando o objeto de interesse é escuro)*
- `--blur <0|3|5|7...>` *(opcional; aplica suavização Gaussiana prévia de tamanho ímpar)*
- `--block-size <impar>=3>` e `--c <real>` *(parâmetros da limiarização adaptativa Gaussiana)*
- `--hsv-low <H,S,V>` e `--hsv-high <H,S,V>` *(ou `--h-min`, `--h-max`, `--s-min`, `--s-max`, `--v-min`, `--v-max`; em OpenCV 8-bit: $H \in [0, 179]$, $S,V \in [0, 255]$. Se `h-min > h-max`, trata automaticamente a descontinuidade do matiz combinando $[H_{\min}, 179] \cup [0, H_{\max}]$)*
- `--overlay <arquivo.png>` *(opcional; salva sobreposição visual da máscara e contornos sobre a imagem de entrada)*

#### Comandos concretos para reproduzir o estudo M2.1:

```bash
# Imagem 1 (Condicao favoravel) - Configuracao 1: Limiar fixo
./cmake-build-debug/pdi_lab --input images/input/m2_favorable_objects.png \
  --output images/output/fav_fixed_mask.png \
  --overlay images/output/fav_fixed_overlay.png \
  --operation segment --method fixed --threshold 120

# Imagem 1 (Condicao favoravel) - Configuracao 2: Limiarizacao adaptativa
./cmake-build-debug/pdi_lab --input images/input/m2_favorable_objects.png \
  --output images/output/fav_adaptive_mask.png \
  --overlay images/output/fav_adaptive_overlay.png \
  --operation segment --method adaptive --block-size 31 --c -15

# Imagem 2 (Condicao desafiadora: gradiente de iluminacao) - Configuracao 1: Limiar fixo
./cmake-build-debug/pdi_lab --input images/input/m2_challenging_illumination.png \
  --output images/output/chal_fixed_mask.png \
  --overlay images/output/chal_fixed_overlay.png \
  --operation segment --method fixed --threshold 120

# Imagem 2 (Condicao desafiadora: gradiente de iluminacao) - Configuracao 2: Limiarizacao adaptativa + Gaussiano 3x3
./cmake-build-debug/pdi_lab --input images/input/m2_challenging_illumination.png \
  --output images/output/chal_adaptive_mask.png \
  --overlay images/output/chal_adaptive_overlay.png \
  --operation segment --method adaptive --block-size 35 --c -15 --blur 3
```

---

### 2.2. Laboratório M2.2 - Componentes Conexos e Sequência Morfológica (`--operation analyze`)

Recebe uma máscara binária (`--input`) e um **prefixo de saída** (`--output`), executando:
1. Rotulação inicial em conectividade `4` e `8` (excluindo o fundo `0`);
2. Aplicação da sequência morfológica configurada (mínimo de 2 operações, incluindo abertura ou fechamento);
3. Recontagem na máscara final, filtragem por área mínima (`--min-area`) e geração de artefatos rastreáveis.

#### Parâmetros aceitos:
- `--connectivity <4|8>` *(padrão: `8`)*
- `--min-area <int>=0>` *(padrão: `0`; marca a coluna `kept` como `true` para `area >= min-area`)*
- `--morph-seq <op1,op2,...>` *(operações separadas por vírgula: `erode`, `dilate`, `open`, `close`; padrão: `open,close`)*
- `--morph-shapes <s1,s2,...>` *(formas do elemento estruturante: `rect`, `ellipse`, `cross`; padrão: `ellipse,ellipse`)*
- `--morph-sizes <k1,k2,...>` *(tamanhos ímpares $\ge 3$; padrão: `3,5`)*
- `--morph-iters <i1,i2,...>` *(iterações por estágio $\ge 1$; padrão: `1,1`)*
- `--morph-config <arquivo>` *(alternativa por arquivo de configuração, ex.: `config/morph_default.txt`)*

#### Arquivos gerados a partir do prefixo `--output <prefixo>`:
- `<prefixo>_initial_mask.png`: cópia preservada da máscara inicial;
- `<prefixo>_stage1_<op>.png`, `<prefixo>_stage2_<op>.png`, ...: máscara binária em PNG após cada estágio morfológico;
- `<prefixo>_final_mask.png`: máscara binária final após filtragem por `--min-area`;
- `<prefixo>_labels.png`: visualização colorida dos componentes conexos com *bounding boxes*, centroides e rótulos;
- `<prefixo>_initial.csv` e `<prefixo>_final.csv`: tabelas de componentes no formato `label,area,x,y,width,height,cx,cy,kept`;
- `<prefixo>_summary.json`: resumo estruturado com conectividade, contagens em conectividade 4 e 8, sequência morfológica e áreas antes/depois.

#### Comandos concretos para reproduzir o estudo M2.2:

```bash
# Caso 1: Limpeza de ruido isolado e fechamento de furos internos
./cmake-build-debug/pdi_lab --input images/output/fav_fixed_mask.png \
  --output results/fav \
  --operation analyze --connectivity 8 --min-area 20 \
  --morph-seq open,close --morph-shapes ellipse,ellipse --morph-sizes 3,5 --morph-iters 1,1

# Caso 2: Analise sob iluminacao irregular / objetos muito proximos
./cmake-build-debug/pdi_lab --input images/output/chal_adaptive_mask.png \
  --output results/chal \
  --operation analyze --connectivity 8 --min-area 20 \
  --morph-config config/morph_default.txt
```
