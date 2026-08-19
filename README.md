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

Para executar Branch-Cut-and-Price com cover cuts de CPU:

```powershell
.\bin\x64\Release\vne_branch_price.exe `
  bcp `
  instances\sub-20.txt `
  instances\r-250-0-50-20-10-5-25 `
  1 `
  saida-bcp.txt
```

O primeiro argumento seleciona o metodo de resolucao: `bp` para
Branch-and-Price, `bcp` para Branch-Cut-and-Price com cover cuts de CPU ou `ilp`
para a formulacao inteira. Os demais argumentos sao o arquivo do substrato, a
pasta das requisicoes, a quantidade de requisicoes e, nos modos `bp` e `bcp`,
o arquivo de saida opcional.

## Cover cuts de CPU

No modo `bcp`, depois de cada resolucao do mestre relaxado, o separador procura
para cada no fisico um conjunto de nos virtuais cuja demanda total de CPU
ultrapassa a capacidade. Para cada cover violado `C`, adiciona a desigualdade
`sum(z[v,k,i] para (v,k) em C) <= |C| - 1` e resolve novamente o mestre antes
de executar o pricing. Os cortes sao mantidos nos descendentes da arvore e o
arquivo de saida informa quantos cortes foram herdados/gerados e o tempo gasto
na separacao.

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

O perfil de depuracao da solucao ja esta configurado com esse teste de uma
requisicao. A Community Edition do CPLEX limita o tamanho dos modelos; testes
com muitas requisicoes podem exceder esse limite.
