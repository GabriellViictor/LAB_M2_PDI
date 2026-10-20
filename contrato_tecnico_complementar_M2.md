# Processamento de Imagens — 2026-02



Este complemento adapta o contrato técnico da M1 a **dois laboratórios individuais, M2.1 e M2.2, desenvolvidos no mesmo projeto de código**. Onde houver diferença, prevalecem as regras abaixo e os enunciados específicos da M2.

## Regras preservadas

Continuam valendo C++, Java ou Python; execução local por linha de comando; portabilidade sem caminhos absolutos, IDE, Google Drive ou notebook obrigatório; dependências declaradas; entradas e resultados reproduzíveis; códigos de saída 0 no sucesso e diferente de 0 no erro; `README.md`, `REPORT.md`, `AI_USAGE.md`, `lab.json`, fontes, testes e imagens; transparência no uso de IA; exclusão de binários, caches e ambientes virtuais. O projeto pode aproveitar a estrutura técnica da M1, mas não precisa incluir as operações da M1.

## Diferenças da M2

- **OpenCV liberado:** funções prontas de conversão, filtragem, limiarização, morfologia e componentes conexos são permitidas. Não existe exigência de implementar essas operações manualmente.
- **Um projeto, duas entregas:** M2.1 entrega `segment`; M2.2 amplia o mesmo código com `analyze`, mantendo `segment` funcional. O relatório e os testes evoluem junto com o código.
- **Entradas e saídas:** `segment` recebe imagem e escreve máscara binária no caminho exato de `--output`; `analyze` recebe máscara e usa `--output` como prefixo dos arquivos de análise. Imagens intermediárias em PNG, tabelas em CSV, parâmetros e resumo em JSON ou formato textual documentado.
- **Dados observáveis:** conectividade, limiar, intervalos de cor, elemento estruturante, iterações e filtragem por área devem ser registrados; uma nova execução com mesmos dados e parâmetros deve repetir os resultados.

## Estrutura sugerida

```text
lab_m2_nome_sobrenome/
├── README.md
├── REPORT.md
├── AI_USAGE.md
├── lab.json
├── images/
│   ├── input/
│   └── output/
├── results/
├── src/
├── tests/
└── arquivos específicos da linguagem
```

O `lab.json` mantém `schema_version: 1`, `language` (`cpp`, `java` ou `python`) e `student.name`; use `"lab": "m2.1"` na primeira entrega e `"lab": "m2.2"` na versão consolidada. Inclua `"stages": ["m2.1", "m2.2"]` na versão final para explicitar os dois marcos. Os outros arquivos e comandos da M1 permanecem como modelo de organização, com nomes adaptados à M2.

## Interface comum

```bash
pdi_lab --input <imagem> --output <arquivo.png> --operation segment \
  --method fixed --threshold 120
pdi_lab --input <mascara.png> --output <prefixo> --operation analyze \
  --connectivity 8 --min-area 20
```

Em Windows, Linux e macOS, documente como o comando `pdi_lab` é invocado na linguagem escolhida (binário, `java -jar` ou `python -m pdi_lab`). A aplicação não pode exigir interface gráfica ou interação durante a execução. Argumentos adicionais podem existir; publique `--help` e exemplos completos para as duas configurações de segmentação e para as duas conectividades. O processamento de uma máscara não deve depender de estado oculto de execução anterior.

## Convenções verificáveis

- Máscara: 1 canal, mesmo tamanho da entrada, valores 0/255, objeto em branco, PNG sem perdas.
- Componentes: fundo excluído; distinguir contagem bruta e contagem após filtro; CSV com cabeçalho estável e campos definidos no M2.2.
- Erros: arquivo ausente ou ilegível, imagem incompatível, método/parâmetro inválido, máscara não binária e falha de gravação devem produzir mensagem clara e código diferente de 0.
- Dependências: versão de OpenCV e pacote de ligação da linguagem identificados no `README.md`; compilação/instalação e teste reprodutíveis sem pastas geradas incluídas na entrega.
- Relatório: manter seções M2.1 e M2.2, resultados intermediários legíveis, parâmetros completos, limitações e declaração de IA em `AI_USAGE.md`.

Os critérios gerais da rubrica vigente — execução, requisitos, corretude, guardas e análise — continuam aplicáveis. Na M2, a corretude examina a seleção e aplicação apropriada dos métodos, a interpretação dos resultados, a consistência de máscaras, contagens e medidas, e não uma implementação manual do algoritmo do OpenCV. Datas e pesos específicos de cada entrega serão informados no ambiente da disciplina.