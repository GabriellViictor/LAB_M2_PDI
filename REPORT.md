# Relatório Técnico Consolidado — Laboratórios M2.1 e M2.2

**Estudante:** Gabriel Victor Garcia Teixeira (`7701594`)  
**Disciplina:** Processamento Digital de Imagens (2026-02)

---

## Parte 1 — Laboratório M2.1: Máscara Binária e Segmentação

### 1.1. Definição do Alvo e Conjunto de Imagens
- **Objeto de interesse:** Discos circulares claros (`255` na máscara binária) sobre fundo escuro (`0`).
- **Imagem 1 — Condições favoráveis (`images/input/m2_favorable_objects.png`, $260 \times 200$):** 4 objetos circulares claros bem contrastados sobre fundo escuro uniforme, contendo 3 pontos isolados de ruído de alta intensidade ($1 \times 1$ px) no fundo e 2 pequenos furos escuros no interior dos discos.
- **Imagem 2 — Condições desafiadoras (`images/input/m2_challenging_illumination.png`, $260 \times 200$):** 5 objetos circulares distribuídos sobre um fundo com forte gradiente horizontal de iluminação (intensidade do fundo variando de $20$ à esquerda até $165$ à direita), além de dois objetos separados por apenas $2\text{ px}$ na região mais clara.

### 1.2. Configurações Comparadas e Pré-processamento
1. **Configuração 1 — Limiar Fixo (`--method fixed --threshold 120`):** Conversão para níveis de cinza de 8 bits e binarização direta ($g(x,y) = 255$ se $f(x,y) \ge 120$, caso contrário $0$), sem pré-processamento.
2. **Configuração 2 — Limiarização Adaptativa Gaussiana (`--method adaptive`):**
   - Na Imagem 1: `--block-size 31 --c -15 --blur 0`.
   - Na Imagem 2: `--block-size 35 --c -15 --blur 3` (pré-processamento com filtro Gaussiano $3 \times 3$ para atenuar variações pontuais antes do limiar local).

### 1.3. Proporção de Primeiro Plano por Imagem e Configuração

| Imagem de Entrada | Configuração | Parâmetros | Pixels de 1º Plano (`255`) | Total de Pixels | Proporção de 1º Plano |
|---|---|---|---:|---:|---:|
| `m2_favorable_objects.png` | `fixed` | `--threshold 120` | $9\,021$ | $52\,000$ | $17,35\%$ |
| `m2_favorable_objects.png` | `adaptive` | `--block-size 31 --c -15` | $4\,319$ | $52\,000$ | $8,31\%$ |
| `m2_challenging_illumination.png` | `fixed` | `--threshold 120` | $17\,693$ | $52\,000$ | $34,02\%$ |
| `m2_challenging_illumination.png` | `adaptive` | `--block-size 35 --c -15 --blur 3` | $2\,559$ | $52\,000$ | $4,92\%$ |

### 1.4. Discussão Comparativa e Respostas às Questões (M2.1)
- **(a) Em que casos o limiar fixo funcionou ou falhou?**
  O limiar fixo (`T = 120`) funcionou muito bem na **Imagem 1 (`m2_favorable_objects.png`)**, preservando integralmente a área dos 4 discos porque o histograma é bimodal e a iluminação de fundo é constante ($\approx 28$). Em contrapartida, falhou severamente na **Imagem 2 (`m2_challenging_illumination.png`)**: como o fundo à direita atinge intensidade $165 > 120$ e os objetos à esquerda possuem intensidade $\approx 95 < 120$, o limiar fixo produziu falsos negativos totais nos 2 objetos da região sombreada e um enorme bloco de falso positivo no fundo iluminado à direita.
- **(b) Qual parâmetro foi mais sensível?**
  No método `fixed`, o parâmetro `--threshold` é crítico na presença de rampa de iluminação (qualquer valor único perde objetos na sombra ou satura o fundo claro). No método `adaptive`, o tamanho da janela `--block-size` mostrou-se o parâmetro mais sensível: quando menor que o diâmetro do objeto, o interior homogêneo dos discos tem média próxima aos próprios pixels do disco, gerando furos internos (efeito de anel/subsegmentação central observado quando `block-size` é pequeno frente ao raio do disco).
- **(c) Quais erros podem atrapalhar a contagem de objetos no M2.2?**
  1. **Falsos positivos isolados (ruído de sal):** Inflam artificialmente a contagem bruta de componentes conexos (na Imagem 1 com limiar fixo, 3 pixels isolados elevam a contagem de 4 para 7 componentes).
  2. **Fragmentação / oco interno:** Na limiarização adaptativa com janela menor que o objeto, o centro do objeto vira fundo ou o contorno pode se fragmentar.
  3. **Fusão por proximidade ou iluminação:** Objetos muito próximos (como o par superior direito da Imagem 2) podem se fundir em um único componente conexo durante o pós-processamento.

---

## Parte 2 — Laboratório M2.2: Componentes Conexos e Sequência Morfológica

### 2.1. Comparação entre Conectividade 4 e Conectividade 8
A rotulação inicial foi executada com `cv::connectedComponentsWithStats` para conectividade `4` e `8` na mesma máscara:
- Em configurações com pixels que se tocam apenas pelos vértices diagonais (validado no teste sintético diagonal e em contornos serrilhados), a **conectividade 4** separa pixels diagonais em componentes distintos ($2$ componentes no caso sintético diagonal), enquanto a **conectividade 8** os agrupa no mesmo componente ($1$ componente).
- Adotou-se **conectividade 8** (`--connectivity 8`) para a análise final por preservar melhor a integridade de bordas curvas discretizadas na grade raster.

### 2.2. Sequência Morfológica Justificada
Foi aplicada a sequência de **duas operações** (`config/morph_default.txt`):
1. **Estágio 1 — Abertura (`open`, elemento estruturante `ellipse` $3 \times 3$, `1` iteração):**
   - **Problema visado:** Eliminar pequenos falsos positivos isolados (ruído impulsivo de $1 \times 1$ a $2 \times 2$ pixels no fundo) sem deslocar os contornos principais.
2. **Estágio 2 — Fechamento (`close`, elemento estruturante `ellipse` $5 \times 5$, `1` iteração):**
   - **Problema visado:** Preencher pequenos furos escuros no interior dos objetos segmentados (falsos negativos internos) e reconectar pequenas falhas de borda.

### 2.3. Tabela Comparativa Antes e Depois da Sequência Morfológica

| Caso / Máscara | Conectividade | Componentes Brutos (Antes) | Componentes Filtrados (`area >= 20`, Antes) | Área Total Bruta (Antes) | Componentes Brutos (Depois) | Componentes Filtrados (`area >= 20`, Depois) | Área Total (Depois) |
|---|---:|---:|---:|---:|---:|---:|---:|
| **Caso 1 (`fav_fixed_mask.png`)** | 8 | $7$ | $4$ | $9\,021\text{ px}$ | $4$ | $4$ | $9\,028\text{ px}$ |
| **Caso 2 (`chal_adaptive_mask.png`)** | 8 | $5$ | $5$ | $2\,559\text{ px}$ | $5$ | $5$ | $2\,557\text{ px}$ |

### 2.4. Análise de Mudanças e Limitações (Onde Ajuda vs. Efeito Indesejado)
1. **Caso em que a sequência resolve o problema (Caso 1 — `results/fav`):**
   - Na máscara inicial `fav_fixed_mask.png`, havia **7 componentes brutos** ($4$ discos reais + $3$ ruídos isolados de $1\text{ px}$ cada) e **2 furos internos** nos discos ($9\,021\text{ px}$ no total).
   - O **Estágio 1 (`results/fav_stage1_open.png`)** removeu completamente os $3$ pontos de ruído isolado, reduzindo a contagem bruta de $7$ para $4$ componentes (mesmo sem depender do filtro de área).
   - O **Estágio 2 (`results/fav_stage2_close.png`)** preencheu os furos internos dos discos, elevando a área total de primeiro plano para $9\,028\text{ px}$ e produzindo 4 componentes conexos limpos e coesos em `results/fav_labels.png`.
2. **Efeito indesejado / Limitação observada (Fusão de objetos próximos e alteração de contorno):**
   - Quando dois objetos estão separados por uma distância inferior ao diâmetro do elemento estruturante do **fechamento** (por exemplo, objetos separados por $2\text{ px}$ submetidos a um fechamento com kernel $\ge 5 \times 5$ ou múltiplas iterações), a dilatação do fechamento conecta as duas regiões e a erosão subsequente não desfaz a ponte criada, fundindo **2 objetos reais em 1 único componente conexo** (subcontagem).
   - Por isso, a quantidade de componentes conexos rotulados não equivale necessariamente ao número real de objetos físicos na cena quando ocorre contato, sobreposição ou fragmentação severa.
