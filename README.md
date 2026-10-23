
# MiniHV — Simulador de Hypervisor

<p align="center">
  <strong>Simulação educacional de virtualização, escalonamento de CPU, gerenciamento de memória e operações de entrada e saída.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Linguagem-C-blue?style=flat-square&logo=c" alt="Linguagem C">
  <img src="https://img.shields.io/badge/Padr%C3%A3o-C11-blue?style=flat-square" alt="Padrão C11">
  <img src="https://img.shields.io/badge/Escopo-02-purple?style=flat-square" alt="Escopo 02">
  <img src="https://img.shields.io/badge/Licen%C3%A7a-MIT-green?style=flat-square" alt="Licença MIT">
</p>

---

## Sumário

- [1. Sobre o projeto](#1-sobre-o-projeto)
- [2. Objetivos](#2-objetivos)
- [3. Escopo do projeto](#3-escopo-do-projeto)
- [4. Funcionalidades](#4-funcionalidades)
- [5. Tecnologias utilizadas](#5-tecnologias-utilizadas)
- [6. Arquitetura](#6-arquitetura)
- [7. Políticas e mecanismos](#7-políticas-e-mecanismos)
- [8. Estrutura do repositório](#8-estrutura-do-repositório)
- [9. Requisitos do sistema](#9-requisitos-do-sistema)
- [10. Instalação](#10-instalação)
- [11. Compilação](#11-compilação)
- [12. Execução](#12-execução)
- [13. Interface de linha de comando](#13-interface-de-linha-de-comando)
- [14. Configuração](#14-configuração)
- [15. Programas simulados](#15-programas-simulados)
- [16. Testes automatizados](#16-testes-automatizados)
- [17. Métricas e resultados](#17-métricas-e-resultados)
- [18. Solução de problemas](#18-solução-de-problemas)
- [19. Limitações conhecidas](#19-limitações-conhecidas)
- [20. Contribuições](#20-contribuições)
- [21. Licença](#21-licença)
- [22. Autor](#22-autor)

---

## 1. Sobre o projeto

O **MiniHV — Simulador de Hypervisor** é um projeto acadêmico desenvolvido em linguagem C para representar conceitos fundamentais de sistemas operacionais e virtualização.

Um hypervisor é uma camada de software responsável por gerenciar máquinas virtuais e distribuir recursos computacionais entre elas. Em ambientes reais, esses recursos incluem processamento, memória e dispositivos de entrada e saída.

O MiniHV reproduz parte desses mecanismos por meio de uma simulação. Cada máquina virtual possui um estado de execução e pode executar programas simulados, utilizar recursos computacionais, solicitar operações de E/S e apresentar situações de término ou falha.

O projeto permite estudar como diferentes políticas de gerenciamento influenciam o comportamento do sistema, além de observar métricas produzidas durante a execução.

### Informações do projeto

| Propriedade | Descrição |
|---|---|
| Nome | MiniHV — Simulador de Hypervisor |
| Número do escopo | 02 |
| Categoria | Projeto acadêmico de sistemas operacionais |
| Linguagem | C |
| Padrão da linguagem | C11 |
| Compilador de referência | GCC |
| Sistema de build | Make |
| Licença | MIT |
| Objetivo principal | Simular mecanismos de gerenciamento de máquinas virtuais |

> **Importante:** o MiniHV é um simulador educacional. Ele não inicializa sistemas operacionais convidados, não executa virtualização real de hardware e não substitui hypervisors de produção, como KVM, Xen ou VMware ESXi.

## 2. Objetivos

O projeto tem como objetivos:

- Compreender os fundamentos de virtualização e gerenciamento de máquinas virtuais.
- Simular a execução de múltiplas máquinas virtuais.
- Representar o escalonamento de unidades virtuais de processamento (vCPUs).
- Implementar mecanismos de gerenciamento e substituição de páginas de memória.
- Simular operações de entrada e saída.
- Organizar solicitações de disco por meio de uma política de escalonamento.
- Identificar condições de execução excessiva.
- Representar falhas de proteção e instruções inválidas.
- Coletar métricas para analisar o comportamento da simulação.
- Aplicar conceitos de programação modular, compilação e testes em C.

## 3. Escopo do projeto

O MiniHV segue o **Escopo 02** do projeto acadêmico.

O sistema é organizado como uma aplicação monolítica modular: seus componentes são implementados em módulos de código separados, mas integrados em um único executável.

Essa organização permite separar responsabilidades sem exigir que cada componente seja executado como um processo independente.

O escopo contempla mecanismos relacionados a:

1. Gerenciamento do ciclo de vida das máquinas virtuais.
2. Escalonamento de CPU.
3. Gerenciamento de memória virtual simulada.
4. Operações de entrada e saída.
5. Escalonamento de solicitações de disco.
6. Tratamento de erros e condições anormais.
7. Interface de linha de comando.
8. Coleta de métricas e testes.

As políticas, os parâmetros e as regras de funcionamento devem ser consultados no código-fonte e no arquivo de configuração da versão disponibilizada neste repositório.

## 4. Funcionalidades

| Componente | Responsabilidade |
|---|---|
| Gerenciamento de VMs | Controlar os estados das máquinas virtuais simuladas. |
| Escalonador de CPU | Determinar a ordem de utilização do processamento. |
| Memória virtual | Representar o acesso a páginas e as faltas de página. |
| Substituição de páginas | Selecionar páginas candidatas à substituição utilizando LRU. |
| Subsistema de E/S | Simular solicitações de console, disco e rede. |
| Escalonador de disco | Organizar solicitações utilizando C-LOOK. |
| Watchdog | Detectar execuções que ultrapassam limites definidos. |
| Proteção | Representar operações que violam as regras de proteção. |
| Validação de instruções | Identificar instruções inválidas nos programas simulados. |
| Interface CLI | Permitir consultas e interação com a simulação pelo terminal. |
| Métricas | Registrar dados de execução, processamento, memória e E/S. |
| Testes | Verificar o comportamento do sistema em cenários definidos. |

A disponibilidade de cada operação e os critérios de tratamento dependem da implementação presente no código-fonte.

## 5. Tecnologias utilizadas

| Tecnologia | Utilização |
|---|---|
| C11 | Implementação dos componentes do simulador. |
| GCC | Compilação do código-fonte. |
| GNU Make | Automação da compilação e das tarefas do projeto. |
| Git | Controle de versão e colaboração. |
| PowerShell | Compilação nativa no Windows, quando configurada. |
| WSL2 / Ubuntu | Ambiente recomendado no Windows para executar scripts Unix. |
| Valgrind | Análise de gerenciamento de memória em ambientes compatíveis. |

### Opções de compilação

Na compilação de referência com GCC, são utilizadas as seguintes opções:

| Opção | Finalidade |
|---|---|
| `-std=c11` | Seleciona o padrão C11. |
| `-Wall` | Habilita avisos comuns do compilador. |
| `-Wextra` | Habilita avisos adicionais. |
| `-Werror` | Trata avisos como erros. |
| `-pedantic` | Solicita verificações adicionais de conformidade com o padrão. |
| `-O2` | Ativa otimizações de compilação. |
| `-Isrc` | Inclui `src/` no caminho de busca por cabeçalhos. |

## 6. Arquitetura

O MiniHV utiliza uma arquitetura monolítica modular. Os componentes cooperam dentro do mesmo programa, compartilhando as estruturas e os mecanismos necessários à simulação.

### Visão conceitual

```text
                       +----------------------+
                       |    Interface CLI     |
                       +----------+-----------+
                                  |
                                  v
                       +----------------------+
                       |   Núcleo do MiniHV   |
                       +----------+-----------+
                                  |
              +-------------------+-------------------+
              |                   |                   |
              v                   v                   v
      +---------------+   +---------------+   +---------------+
      | Escalonador   |   | Gerenciamento |   | Subsistema    |
      | de CPU        |   | de memória    |   | de E/S        |
      +---------------+   +---------------+   +---------------+
              |                   |                   |
              +-------------------+-------------------+
                                  |
                                  v
                       +----------------------+
                       | Tratamento de erros |
                       | e estados das VMs    |
                       +----------+-----------+
                                  |
                                  v
                       +----------------------+
                       | Métricas e relatórios|
                       +----------------------+
```

O diagrama apresenta uma visão conceitual, não um mapa exato das dependências do código. Para identificar os módulos concretos e suas interfaces, consulte os arquivos em `src/`.

### Responsabilidades dos componentes

**Núcleo do simulador**

Coordena a execução da simulação e a interação entre os subsistemas.

**Gerenciamento de VMs**

Mantém as informações e os estados necessários à execução das máquinas virtuais simuladas.

**Escalonador de CPU**

Escolhe a próxima unidade de trabalho conforme a política implementada e as condições da simulação.

**Gerenciamento de memória**

Representa o acesso à memória, o tratamento de faltas de página e a substituição de páginas.

**Subsistema de E/S**

Representa as solicitações de dispositivos e o processamento das operações simuladas.

**Interface de linha de comando**

Disponibiliza comandos para consultar informações e interagir com a execução.

**Métricas e relatórios**

Reúnem dados que ajudam a compreender o comportamento do sistema e avaliar os resultados.

## 7. Políticas e mecanismos

### 7.1. Escalonamento de CPU — SJF

O **Shortest Job First (SJF)** prioriza o trabalho com menor duração estimada, conforme os critérios definidos na implementação.

Essa política permite estudar como a escolha da próxima tarefa influencia o tempo de espera e o tempo de conclusão dos trabalhos.

Os resultados dependem das regras utilizadas para estimar ou determinar a duração dos trabalhos, bem como do comportamento das VMs.

### 7.2. Gerenciamento de memória — LRU

O **Least Recently Used (LRU)** utiliza informações sobre a utilização recente das páginas para selecionar uma página candidata à substituição.

Quando uma página precisa ser substituída, o mecanismo considera o histórico de acessos mantido pela implementação.

A quantidade de faltas de página e substituições depende dos programas executados, da configuração de memória e da sequência de acessos.

### 7.3. Escalonamento de disco — C-LOOK

O **C-LOOK** organiza solicitações de disco de acordo com uma direção de movimentação dos cilindros.

Ao chegar à última solicitação pendente naquela direção, o escalonador retorna à solicitação mais distante no sentido oposto, sem precisar percorrer toda a extremidade física do disco.

A política permite estudar como a ordenação dos pedidos influencia o deslocamento total do mecanismo de disco simulado.

### 7.4. Watchdog

O watchdog monitora a execução e identifica situações em que os limites configurados são ultrapassados.

O resultado pode incluir a sinalização ou o encerramento de uma execução, conforme as regras implementadas no simulador.

### 7.5. Proteção e instruções inválidas

O sistema representa situações em que um programa tenta realizar uma operação proibida ou executar uma instrução que não é aceita pelo interpretador.

Esses cenários permitem observar o tratamento de falhas e diferenciar a conclusão normal da execução de situações anormais.

## 8. Estrutura do repositório

A organização esperada do projeto é apresentada a seguir. Os nomes e a quantidade de arquivos internos devem corresponder à versão efetivamente publicada.

```text
MiniHV-Escopo02/
├── README.md
├── LICENSE
├── Makefile
├── src/
│   └── ...
├── conf/
│   └── hv.conf
├── progs/
│   └── ...
├── tests/
│   └── ...
├── docs/
│   ├── relatorio.md
│   └── relatorio.pdf
├── build/
└── out/
```

| Caminho | Conteúdo |
|---|---|
| `README.md` | Documentação principal do projeto. |
| `LICENSE` | Termos da licença MIT. |
| `Makefile` | Instruções de compilação e tarefas automatizadas. |
| `src/` | Código-fonte e cabeçalhos do simulador. |
| `conf/` | Arquivos de configuração. |
| `progs/` | Programas utilizados na simulação. |
| `tests/` | Testes e scripts de validação. |
| `docs/` | Documentação técnica e relatório acadêmico. |
| `build/` | Executáveis e artefatos de compilação. |
| `out/` | Saídas geradas durante a execução, quando utilizadas. |

Os diretórios `build/` e `out/` podem ser gerados automaticamente. Não é necessário versionar executáveis se eles puderem ser reproduzidos a partir do código-fonte.

## 9. Requisitos do sistema

### Linux

- Distribuição Linux compatível.
- GCC compatível com C11.
- GNU Make.
- Git, para clonar o repositório.
- Bash, caso os scripts de teste dependam dele.

### Windows

É possível utilizar GCC diretamente no PowerShell para compilar os arquivos C.

Entretanto, para executar o Makefile e os testes que dependem de ferramentas Unix, recomenda-se o Ubuntu pelo WSL2.

### macOS

- Ferramentas de linha de comando do Xcode ou ambiente equivalente.
- Compilador compatível com C11.
- GNU Make ou Make compatível com as instruções do projeto.

Os requisitos exatos dependem das dependências presentes no código e nos scripts da versão publicada.

## 10. Instalação

### 10.1. Linux e Ubuntu

Instale as ferramentas necessárias:

```bash
sudo apt update
sudo apt install build-essential gcc make git
```

Para utilizar Valgrind:

```bash
sudo apt install valgrind
```

### 10.2. Windows com WSL2

Abra o PowerShell como administrador e execute:

```powershell
wsl --install
```

Após instalar e configurar uma distribuição Linux, abra o Ubuntu e instale as ferramentas:

```bash
sudo apt update
sudo apt install build-essential gcc make git
```

Consulte a documentação oficial do [WSL](https://learn.microsoft.com/windows/wsl/install) se precisar de ajuda com a instalação.

### 10.3. macOS

Instale as ferramentas de linha de comando:

```bash
xcode-select --install
```

Confira se os comandos estão disponíveis:

```bash
git --version
gcc --version
make --version
```

O comando `gcc` pode corresponder ao Clang disponibilizado pelas ferramentas da Apple. Se o Makefile utilizar opções específicas do GCC, poderá ser necessário instalar e configurar o GCC correspondente.

## 11. Compilação

### 11.1. Obter o código-fonte

Clone o repositório:

```bash
git clone URL_DO_REPOSITORIO
```

Entre na pasta do projeto:

```bash
cd MiniHV-Escopo02
```

Substitua `URL_DO_REPOSITORIO` pelo endereço real do repositório no GitHub.

### 11.2. Compilar pelo Makefile

Na raiz do projeto, execute:

```bash
make
```

O Makefile define as tarefas de compilação e o local de saída do executável.

Para consultar os alvos disponíveis, se o projeto oferecer esse recurso:

```bash
make help
```

Caso o alvo `help` não exista, consulte o arquivo `Makefile`.

### 11.3. Compilar manualmente no Linux

Caso seja necessário compilar sem utilizar o Makefile:

```bash
mkdir -p build out

gcc -Isrc -std=c11 -Wall -Wextra -Werror -pedantic -O2 \
    $(find src -type f -name '*.c') \
    -o build/minihv
```

Esse comando pressupõe que os arquivos necessários estejam em `src/` e seus subdiretórios, sem dependências externas adicionais que exijam opções específicas de compilação.

Se o projeto utilizar outras bibliotecas, arquivos ou flags, utilize o Makefile como referência.

### 11.4. Compilar no Windows com PowerShell

Abra o PowerShell na raiz do projeto.

Crie os diretórios de saída:

```powershell
New-Item -ItemType Directory -Force build, out | Out-Null
```

Localize os arquivos C:

```powershell
$sources = @(
    Get-ChildItem .\src -Recurse -Filter *.c |
    ForEach-Object { $_.FullName }
)
```

Compile:

```powershell
gcc -Isrc -std=c11 -Wall -Wextra -Werror -pedantic -O2 `
    $sources -o .\build\minihv.exe
```

Verifique o resultado:

```powershell
if ($LASTEXITCODE -ne 0) {
    throw "Falha na compilação do MiniHV."
}
```

Esse procedimento compila os arquivos C diretamente. Ele não garante que os scripts Unix do projeto possam ser executados no PowerShell.

## 12. Execução

### 12.1. Linux e macOS

Se o executável estiver em `build/minihv`, execute:

```bash
./build/minihv
```

Para iniciar a interface interativa:

```bash
./build/minihv -i
```

### 12.2. Windows

Se a compilação nativa tiver gerado `build/minihv.exe`, execute:

```powershell
.\build\minihv.exe
```

Para iniciar a interface interativa:

```powershell
.\build\minihv.exe -i
```

Os caminhos acima pressupõem que o executável tenha sido gerado nesses locais. Se o Makefile utilizar outro destino, ajuste o comando.

### 12.3. Diretório de execução

Execute o programa a partir de um diretório compatível com os caminhos utilizados pela aplicação.

O MiniHV pode depender de arquivos externos de configuração e programas simulados. Por isso, preserve a estrutura do repositório e confira se os arquivos necessários estão presentes.

## 13. Interface de linha de comando

A interface de linha de comando permite consultar informações e interagir com a simulação.

Os comandos abaixo foram utilizados durante os testes da versão documentada. A sintaxe exata pode depender da implementação.

| Comando | Finalidade |
|---|---|
| `help` | Exibe informações sobre os comandos disponíveis. |
| `ps` | Consulta informações sobre processos ou VMs. |
| `mem` | Consulta informações de memória. |
| `io` | Consulta informações de entrada e saída. |
| `stats` | Exibe estatísticas da simulação. |
| `run` | Executa a simulação. |
| `step` | Avança a simulação em etapas. |
| `pause` | Solicita a pausa de uma VM, quando permitida. |
| `resume` | Solicita a retomada de uma VM pausada, quando permitida. |
| `kill` | Solicita o encerramento de uma VM, conforme as regras implementadas. |
| `quit` | Encerra a interface interativa. |

Alguns comandos podem exigir argumentos, como um identificador de VM. As operações também podem depender do estado atual da máquina virtual.

Para conhecer a sintaxe aceita, inicie a interface e consulte o comando `help`.

### Exemplo de sessão

```text
help
ps
mem
io
stats
run
quit
```

Digite um comando por vez.

## 14. Configuração

O arquivo principal de configuração é:

```text
conf/hv.conf
```

Os programas simulados ficam no diretório:

```text
progs/
```

A configuração determina parâmetros utilizados durante a simulação, enquanto os programas descrevem as instruções executadas pelas VMs.

### Recomendações para alterar a configuração

1. Faça uma cópia dos arquivos originais.
2. Consulte o formato aceito pelo simulador.
3. Modifique apenas os parâmetros necessários.
4. Execute novamente a simulação.
5. Rode os testes disponíveis.
6. Compare as métricas antes e depois das alterações.

Arquivos com sintaxe inválida ou instruções não reconhecidas podem ser rejeitados pelo simulador.

## 15. Programas simulados

O diretório `progs/` contém os programas utilizados nos cenários de execução.

Para estudar o comportamento do sistema, é recomendável identificar os programas destinados a diferentes situações, como:

- **CPU-bound:** concentra-se em operações que consomem processamento.
- **E/S intensiva:** realiza várias solicitações de entrada e saída.
- **Uso intenso de memória:** acessa diferentes regiões da memória simulada.
- **Falha de proteção:** representa uma operação proibida.
- **Recurso adicional:** exercita algum mecanismo extra previsto no escopo.

Os nomes e o comportamento efetivo de cada programa devem ser confirmados nos arquivos presentes no repositório.

## 16. Testes automatizados

Os testes ajudam a verificar o funcionamento dos componentes e a identificar regressões.

### 16.1. Executar a suíte completa

Em um ambiente compatível com o Makefile:

```bash
make test
```

### 16.2. Verificar gerenciamento de memória

Se o alvo estiver disponível e o Valgrind estiver instalado:

```bash
make valgrind
```

### 16.3. Consultar os arquivos de teste

No Linux:

```bash
find tests -maxdepth 2 -type f
```

Se existir um script compatível com Bash:

```bash
bash tests/test.sh
```

Consulte o conteúdo do script antes de executá-lo caso ele tenha dependências adicionais.

### 16.4. Cenários verificados durante o desenvolvimento

Os testes registrados durante o desenvolvimento incluíram:

| Identificador | Cenário | Resultado registrado |
|---|---|---|
| TA-01 | Contador HALT, CSV e `MEM_FREE` | Aprovado |
| TA-02 | Sequência de dispatch e término de quatro VMs | Aprovado |
| TA-03 | E/S de console, disco e rede | Aprovado |
| TA-04/05 | Faltas de página, substituição, watchdog e proteção | Aprovado |
| TA-06 | Configurações e programas inválidos | Aprovado |
| TA-07 | Reprodutibilidade entre duas execuções | Aprovado |
| TA-08 | Limite de watchdog | Aprovado |
| C-LOOK | Ordenação e atendimento de solicitações de disco | Aprovado |
| CLI | Comandos básicos e tratamento de comando inválido | Aprovado |

Esses resultados correspondem às execuções registradas durante o desenvolvimento. Para reproduzi-los em uma nova versão, execute os testes presentes no repositório e confira os resultados obtidos no ambiente utilizado.

### 16.5. Compatibilidade dos testes

Alguns scripts podem utilizar comandos específicos de Unix, Bash ou ferramentas como Valgrind. Esses scripts podem não funcionar diretamente no PowerShell.

Para uma execução mais próxima do ambiente de desenvolvimento dos testes, recomenda-se Linux ou Ubuntu pelo WSL2.

## 17. Métricas e resultados

O MiniHV pode registrar métricas que ajudam a avaliar a execução e o comportamento dos recursos simulados.

### 17.1. Principais métricas

| Métrica | Significado |
|---|---|
| Ticks | Unidades de tempo da simulação. |
| Trocas de contexto | Mudanças entre unidades de execução. |
| Utilização da CPU | Percentual de utilização do processador simulado. |
| Turnaround | Tempo entre a entrada de um trabalho e sua conclusão, conforme a definição adotada. |
| Tempo de espera | Tempo gasto aguardando processamento ou recurso. |
| Tempo de resposta | Tempo até a primeira resposta, conforme a definição adotada. |
| Throughput | Quantidade de trabalhos concluídos por unidade de tempo. |
| Faltas de página | Ocorrências em que uma página solicitada não está disponível na memória simulada. |
| Substituições de páginas | Quantidade de páginas substituídas. |
| Solicitações de E/S | Pedidos de operações de entrada e saída. |
| Deslocamento do disco | Distância acumulada percorrida pelo cabeçote simulado, conforme a unidade usada pelo projeto. |

As definições exatas, unidades e fórmulas devem ser conferidas na implementação dos contadores e relatórios.

### 17.2. Execução de referência

Durante o desenvolvimento, uma execução de referência apresentou os seguintes resultados:

| Métrica | Resultado observado |
|---|---:|
| Ticks | 553 |
| CPUs simuladas | 1 |
| Trocas de contexto | 19 |
| Faltas de página | 71 |
| Substituições de páginas | 4 |
| Utilização da CPU 0 | 77,4% |
| Throughput | 0,0072 |
| Solicitações de E/S de disco | 1 |
| Deslocamento total do disco | 125 cilindros |

Esses valores são referências de uma execução específica e não garantias de desempenho. Alterações na configuração, nos programas ou no código podem produzir resultados diferentes.

### 17.3. Estados finais observados

Na mesma execução, foram observados os seguintes estados:

| VM | Estado final |
|---|---|
| `web` | `HALT` |
| `io_worker` | `HALT` |
| `memory_worker` | `WATCHDOG` |
| `guard` | `PROTECTION_FAULT` |

Os estados representam diferentes resultados da simulação: término normal, detecção de execução excessiva e falha de proteção.

### 17.4. Comparação de políticas

Para comparar políticas de escalonamento, como SJF e FCFS, é necessário executar os mesmos programas sob condições equivalentes.

Recomenda-se registrar:

- Turnaround médio.
- Tempo médio de espera.
- Tempo médio de resposta.
- Throughput.
- Utilização da CPU.
- Número de trocas de contexto.

**Os resultados comparativos devem ser medidos experimentalmente.** Não se deve apresentar valores estimados como se fossem resultados de testes reais.

## 18. Solução de problemas

### `gcc: command not found`

O compilador não está instalado ou não está disponível no `PATH`.

No Ubuntu:

```bash
sudo apt update
sudo apt install gcc build-essential
```

No Windows, confira a instalação do GCC e as variáveis de ambiente.

No macOS:

```bash
xcode-select --install
```

### `make: command not found`

Instale o Make e as ferramentas de compilação.

No Ubuntu:

```bash
sudo apt install make build-essential
```

### `No such file or directory`

Confirme que você está na raiz do projeto:

```bash
pwd
ls
```

No PowerShell:

```powershell
Get-Location
Get-ChildItem
```

Verifique também se o executável foi gerado no caminho esperado.

### Erros de compilação

Confira:

1. Se todos os arquivos do projeto estão presentes.
2. Se o compilador suporta C11.
3. Se os caminhos de inclusão estão corretos.
4. Se as opções de compilação são compatíveis com o ambiente.
5. Qual é a primeira mensagem de erro emitida pelo compilador.

### Os testes não funcionam no Windows

Scripts baseados em Bash, comandos Unix ou ferramentas específicas do Linux podem não funcionar diretamente no PowerShell.

Utilize Ubuntu ou WSL2 e execute:

```bash
make test
```

### O simulador não encontra a configuração

Execute o programa a partir de um diretório compatível com os caminhos utilizados pela implementação.

Confira a presença de:

```text
conf/hv.conf
progs/
```

### O comando `make test` não existe

Confira os alvos declarados no Makefile. Se o projeto não possuir esse alvo, será necessário ajustar o Makefile ou executar os testes disponibilizados individualmente.

## 19. Limitações conhecidas

- O MiniHV é um simulador educacional e não um hypervisor de produção.
- Não executa sistemas operacionais convidados reais.
- O comportamento depende dos programas simulados e da configuração utilizada.
- Algumas operações da CLI dependem do estado da VM e podem exigir argumentos.
- Os scripts de teste podem depender de ferramentas específicas do ambiente.
- A compatibilidade multiplataforma depende do compilador, do Makefile e das dependências utilizadas.
- Os resultados de desempenho são específicos de cada execução.
- Comparações entre políticas de escalonamento exigem medições experimentais em condições equivalentes.
- O comportamento de mecanismos adicionais deve ser confirmado na implementação efetivamente publicada.

## 20. Contribuições

Contribuições são bem-vindas, especialmente para fins educacionais.

### Como contribuir

1. Faça um fork do repositório.
2. Crie uma branch para sua alteração.
3. Implemente a melhoria.
4. Execute os testes disponíveis.
5. Atualize a documentação, se necessário.
6. Envie um pull request descrevendo as alterações.

Exemplo:

```bash
git switch -c melhoria/nome-da-melhoria
```

Antes de enviar uma contribuição, verifique se as alterações preservam o funcionamento dos componentes existentes.

### Relato de problemas

Ao abrir uma issue, inclua, sempre que possível:

- Sistema operacional.
- Versão do compilador.
- Comando executado.
- Mensagem de erro completa.
- Etapas para reproduzir o problema.
- Resultado esperado e resultado obtido.

Evite incluir informações pessoais ou dados sensíveis nos relatórios.

## 21. Licença

Este projeto está licenciado sob a **Licença MIT**.

Consulte o arquivo [`LICENSE`](LICENSE) na raiz do repositório para conhecer os termos e as condições de utilização, modificação e distribuição do software.

A licença MIT permite, em geral, utilizar, copiar, modificar, distribuir, sublicenciar e comercializar o software, desde que sejam preservados os avisos de copyright e o texto da licença, conforme seus termos.

## 22. Autor

**Ana Vitória Oliveira Ribeiro**

**Projeto acadêmico:** MiniHV — Escopo 02.

Desenvolvido em C para o estudo de sistemas operacionais, virtualização, escalonamento de CPU, gerenciamento de memória e operações de entrada e saída.

---

<p align="center">
  <strong>MiniHV — Aprendendo os fundamentos da virtualização por meio da simulação.</strong>
</p>