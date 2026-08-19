# VNE Branch-and-Price

Implementacao em C++ de Virtual Network Embedding usando Branch-and-Price e
IBM ILOG CPLEX Concert.

## Requisitos no Windows

- Visual Studio 2026 com a carga **Desenvolvimento para Desktop com C++**;
- IBM ILOG CPLEX Optimization Studio 22.2.0 x64.

O projeto procura o CPLEX pela variavel `CPLEX_STUDIO_DIR222`. Quando ela nao
estiver definida, usa por padrao:

`C:\Program Files\IBM\ILOG\CPLEX_Studio_Community222`

## Compilar no Visual Studio

1. Abra `VNE_BranchPrice.sln`.
2. Selecione `Release` e `x64`.
3. Use **Compilar > Compilar Solucao**.

O executavel sera criado em `bin\x64\Release\vne_branch_price.exe`. A DLL do
CPLEX e copiada automaticamente para a mesma pasta.

## Executar

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  bp `
  instances\sub-20.txt `
  instances\r-250-0-50-20-10-5-25 `
  1 `
  saida.txt
```

Para executar o modelo ILP:

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  ilp `
  instances\sub-20.txt `
  instances\r-250-0-50-20-10-5-25 `
  1
```

Para executar Branch-Cut-and-Price com cover cuts de CPU, banda e aceitacao:

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  bcp `
  instances\sub-20.txt `
  instances\r-250-0-50-20-10-5-25 `
  1 `
  saida-bcp.txt
```

O primeiro argumento seleciona o metodo de resolucao: `bp` para
Branch-and-Price, `bcp` para Branch-Cut-and-Price com cover cuts de CPU, banda
e aceitacao `y`, ou `ilp` para a formulacao inteira. Os demais argumentos sao o
arquivo do substrato, a pasta das requisicoes, a quantidade de requisicoes e,
por fim, o arquivo de saida opcional.

### Limites e diagnostico

Todos os metodos aceitam `--time-limit SEGUNDOS`. BP e BCP tambem aceitam:

- `--heuristic-time-limit SEGUNDOS`: limite da heuristica primal por no;
- `--restricted-mip-time-limit SEGUNDOS`: limite do MIP restrito;
- `--root-only`: resolve somente a geracao de colunas do no raiz.

Exemplo de diagnostico da raiz por 120 segundos:

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  bcp instances\sub-20.txt instances\r-250-0-50-20-10-5-25 5 `
  saida-raiz.txt `
  --time-limit 120 `
  --heuristic-time-limit 2 `
  --restricted-mip-time-limit 2 `
  --root-only
```

O MIP restrito e executado apenas na raiz ou quando a heuristica primal nao
encontra incumbente. O branching escolhe a variavel mais fracionaria da
primeira familia disponivel (`y`, depois `z`, depois uso de aresta), com
desempate deterministico.

Cada arquivo de saida registra iteracoes de GC, colunas novas e duplicadas e o
motivo de parada. O bloco entre `BEGIN_SUMMARY` e `END_SUMMARY` e estavel e
pode ser consumido por ferramentas externas.

## Benchmark automatizado

Os scripts requerem Python 3.9 ou superior e usam apenas a biblioteca padrao.

O script abaixo executa ILP, BP e BCP com 1 a 5 requisicoes e limite de 120
segundos. Cada execucao recebe uma pasta propria contendo comando, stdout,
stderr, resultado e metadados JSON:

```powershell
python .\scripts\run_benchmark.py
```

Uma campanha customizada pode usar outros tamanhos, repeticoes ou limites:

```powershell
python .\scripts\run_benchmark.py `
  --request-counts 1 2 3 4 5 10 `
  --time-limits 60 300 900 3600 `
  --repetitions 3 `
  --campaign-name servidor-cplex
```

Para estudar apenas o no raiz de BP e BCP, acrescente `--root-only`. ILP
continua sendo resolvido normalmente nessa campanha. Por padrao os resultados
ficam em `benchmark_results\<campanha>`; use `--output-root` para escolher
outro local.

O relatorio HTML autocontido e gerado a partir da pasta da campanha:

```powershell
python .\scripts\generate_report.py `
  .\benchmark_results\servidor-cplex
```

O arquivo `report.html` compara UB, LB, gap, tempo, nos, iteracoes de GC e
colunas, alem de preservar os resultados brutos para auditoria.

## Cover cuts de CPU e banda

No modo `bcp`, depois de cada resolucao do mestre relaxado, o separador procura
para cada no fisico um conjunto de nos virtuais cuja demanda total de CPU
ultrapassa a capacidade. Para cada cover violado `C`, adiciona a desigualdade
`sum(z[v,k,i] para (v,k) em C) <= |C| - 1` e resolve novamente o mestre antes
de executar o pricing.

O mesmo processo e aplicado a cada aresta fisica. Se um conjunto `C` de
colunas que utilizam a aresta ultrapassa sua capacidade de banda, o mestre
recebe `sum(lambda[p] para p em C) <= |C| - 1`. Cada coluna possui um
identificador persistente para que esse corte continue valido quando o mestre
e reconstruido em um no filho.

As familias de CPU e banda inspecionam a mesma solucao LP antes de alterar o modelo. Os
cortes sao mantidos nos descendentes da arvore e o arquivo de saida separa as
metricas `# CPU Cuts`, `# BW Cuts`, `# Gen. CPU` e `# Gen. BW`, alem do total e
do tempo de separacao.

### Covers de recursos sobre aceitacao `y`

O separador tambem constroi desigualdades de recursos que nao dependem das
rotas geradas. Para CPU e banda sao usados tanto o consumo total minimo de cada
requisicao quanto perfis por limiar. Em um limiar `t`, por exemplo, conta-se
quantos nos virtuais de cada requisicao exigem pelo menos `t` unidades de CPU e
quantos desses itens cabem nos nos fisicos. O mesmo raciocinio e aplicado aos
enlaces e suas capacidades de banda.

Cada perfil define uma mochila `sum(w[v] * y[v]) <= capacidade`. Quando um
conjunto `C` forma um cover violado, o mestre recebe
`sum(y[v] para v em C) <= |C| - 1`. Esses cortes permanecem validos para todas
as colunas futuras e, portanto, nao exigem alteracoes no pricing. O relatorio
usa `# Y Cuts` e `# Gen. Y` para essa familia. No teste `instances\cover-test`,
um cover sobre `y` fecha o gap da raiz.

### No-good cuts de factibilidade

Quando os separadores de recursos nao encontram novos cortes, o modo `bcp`
pode chamar um oraculo compacto para um conjunto violado de variaveis `y`. O
oraculo fixa essas requisicoes como aceitas e verifica posicionamento, CPU,
banda e conectividade. As rotas usam uma variavel binaria por aresta fisica e
cutsets de conectividade separados iterativamente, mantendo o modelo auxiliar
dentro do limite de tamanho configurado para a Community Edition.

Se o CPLEX prova que o conjunto `C` e inviavel, o mestre recebe o no-good
`sum(y[v] para v em C) <= |C| - 1`. O conjunto e reduzido enquanto a
inviabilidade continuar provada. Timeouts, limites do CPLEX e outros resultados
inconclusivos nunca geram cortes. As colunas `# NG Cuts`, `# Gen. NG`,
`# Feas. Checks` e `# Feas. Unk` registram essa etapa.

O caso `instances\nogood-test` possui recursos totais suficientes, mas uma
topologia fisica desconectada que torna a requisicao inviavel:

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  bcp `
  instances\nogood-test\substrate.txt `
  instances\nogood-test\requests `
  1 `
  saida-nogood-test.txt
```

### Heuristica primal guiada pela relaxacao

Ao terminar a geracao de colunas de cada no, o algoritmo usa os valores LP de
`y` para ordenar as requisicoes e chama o modelo auxiliar para construir um
embedding inteiro completo. Diferentemente do MIP restrito executado em
seguida, essa heuristica pode produzir rotas que ainda nao pertencem ao pool de
`lambda`.

As decisoes anteriores de branching sobre `y` e `z`, assim como arestas
proibidas, sao aplicadas no modelo auxiliar. Se o conjunto inicial nao puder
ser construido no limite de tempo, a requisicao opcional com menor valor de
`y` e removida e a tentativa e repetida. Nos com uma decisao que obriga o uso
de uma aresta especifica sao ignorados conservadoramente pela heuristica atual.

O melhor valor entre a heuristica e o MIP restrito atualiza o incumbente do no.
O relatorio inclui `Heur. UB`, `Heur. Acc` e `HeurTime`. Na instancia de cinco
requisicoes usada nos exemplos, a heuristica encontrou um embedding aceitando
as cinco redes e reduziu o incumbente da raiz de aproximadamente `30107.7` para
`1275.98`.

O caso pequeno em `instances\cover-test` existe para validar a geracao efetiva
dos cortes:

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  bcp `
  instances\cover-test\substrate.txt `
  instances\cover-test\requests `
  2 `
  saida-cover-test.txt
```

O caso `instances\bandwidth-cover-test` exercita uma capacidade de banda
congestionada e os covers associados:

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  bcp `
  instances\bandwidth-cover-test\substrate.txt `
  instances\bandwidth-cover-test\requests `
  2 `
  saida-bandwidth-cover-test.txt
```

O perfil de depuracao da solucao ja esta configurado com esse teste de uma
requisicao. A Community Edition do CPLEX limita o tamanho dos modelos; testes
com muitas requisicoes podem exceder esse limite.
