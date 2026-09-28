# COMPLAC — Compilador da linguagem SLAC²

**Autores:** Thomaz de Souza Scopel (RA 10417183) · Matteo Porcare (RA 10417286)

Primeira fase do compilador COMPLAC. Esta fase implementa a análise léxica, a análise sintática (ASDR) e a Tabela de Símbolos com escopos da linguagem SLAC². O enunciado original está em [docs/instrucoes.md](docs/instrucoes.md) e a especificação da linguagem em [docs/](docs/).

## Compilação

Requisitos: `gcc` e `make` (Linux).

```sh
make          # gera o binário ./complac (objetos em build/)
make clean    # remove build/, o binário e os logs gerados em testes/
```

O Makefile usa as flags `-Wall -Wextra -std=c99`, e o projeto compila sem avisos.

No Windows, instale o GCC (MinGW-w64), por exemplo com `winget install BrechtSanders.WinLibs.POSIX.UCRT`. Depois, reabra o terminal e use `mingw32-make` / `mingw32-make clean`; o binário gerado é `complac.exe`.

## Execução

```sh
./complac <arquivo.slac> [--tokens] [--symtab] [--trace] [--verbose]
```

| Opção | Efeito |
|---|---|
| `--tokens` | cria `<arquivo>.tk` com a lista de tokens: `<linha>  <CATEGORIA>  "<lexema>"` |
| `--symtab` | cria `<arquivo>.ts` com a tabela de símbolos: `SCOPE=<escopo>  id="<lexema>"  cat=<categ>  tipo=<tipo>  extra=<atrib>` |
| `--trace`  | cria `<arquivo>.trc` com o rastreamento da análise (entrada e saída de não-terminais, tokens consumidos, escopos e buscas na TS) |
| `--verbose` | também exibe o rastreamento no terminal (stderr) |

As opções podem ser combinadas. Os logs são criados na mesma pasta do fonte, com o mesmo nome e a extensão correspondente.

Exemplo:

```sh
./complac testes/ok_soma.slac --tokens --symtab
```

### Saída e códigos de retorno

- **Sucesso:** a mensagem `<arquivo>: análise léxica e sintática concluída com sucesso.` e o código de retorno `0`.
- **Erro:** a análise é interrompida no primeiro erro, com uma mensagem em `stderr` e o código de retorno `1`. Exemplos:
  ```
  Erro léxico na linha 5: caractere inválido '$'
  Erro sintático na linha 6: esperado ';', encontrado 'echo'
  Erro semântico na linha 3: identificador 'x' já declarado neste escopo
  ```
  Mesmo após um erro, os logs solicitados são gravados com o conteúdo processado até aquele ponto.

## Estrutura do projeto

```
src/        código-fonte (.c)
include/    cabeçalhos públicos (.h)
docs/       enunciado e especificação da linguagem
testes/     programas SLAC² de teste (ok_*: válidos, err_*: com erro intencional)
Makefile
```

| Módulo | Responsabilidade | Interface principal |
|---|---|---|
| `main` | orquestração: CLI, abertura de arquivos, inicialização e encerramento ordenado dos módulos | — |
| `opt` | interpretação da linha de comando | `opts_parse`, `opts_get` |
| `lex` | analisador léxico, um token por chamada | `lex_next` |
| `parser` | ASDR, uma função por não-terminal; controla os escopos | `parse_program` |
| `symtab` | tabela de símbolos com escopos aninhados | `ts_insert`, `ts_lookup` |
| `diag` | mensagens de erro e rastreamento padronizados | `diag_error`, `diag_info` |
| `log` | criação dos arquivos `.tk`, `.ts` e `.trc` | `log_abrir`, `log_token` |

Os módulos se comunicam apenas pelas funções declaradas nos cabeçalhos. Os estados internos são `static` em cada módulo.

## Particularidades da implementação

### Análise léxica
- Comentários de linha (`# ...`) e de bloco (`/# ... #/`) são descartados.
- O lexema de strings e caracteres é registrado sem as aspas.
- O sinal `-` é sempre emitido como `sSUBRAT`. O parser decide pelo contexto se é unário (posição de fator) ou binário.
- Erros léxicos detectados:
  - caractere inválido;
  - `/` isolado;
  - string ou comentário de bloco não finalizado;
  - constante caractere vazia ou com mais de um caractere;
  - número mal formado (ex.: `12ab`);
  - inteiro acima de 2147483647;
  - string ou identificador com mais de 255 caracteres.

### Análise sintática
O parser segue a EBNF do Apêndice A da especificação. Onde a EBNF diverge do texto e dos exemplos oficiais, seguimos os exemplos:

- **`<princ>`**, que a EBNF não define, foi tratado como `proc main ( ) [<lvars>] <bco>`, sempre ao final do programa.
- **Corpo de `case`, `for` e `while`:** `{<cmd> ";"} end`, como nos Programas 4, 5 e 6 da especificação. O `case` aceita `otherwise` com uma lista de comandos.
- **Bloco `start ... end`:** aceito como comando, pois a especificação permite blocos aninhados.
- **Precedência:** os relacionais `< <= > >=` têm precedência maior que `= ~=`, conforme a tabela de precedência. Para isso foi criado o não-terminal `<ecomp>`.
- **Sinal negativo em constantes:** as constantes inteiras de `from`, `to`, `by` e `match` aceitam `-`, como no exemplo `by -1`.
- **`<elem>`** é analisado como `<expr>`, pois literais, identificadores, vetores e chamadas já são fatores da expressão.
- **Índice de vetor:** apenas constante inteira ou identificador, conforme `<vetr>`.

### Tabela de símbolos
- **Categorias:** `var`, `param`, `param_ref`, `proc`, `func`.
- **Tipos:** `int`, `logic` e `chr`. Vetores aparecem com `[]` (ex.: `int[]`) e procedimentos com `-`.
- **`extra`:** o tamanho, para vetores, ou a quantidade de parâmetros, para sub-rotinas. Nos demais casos aparece `-`.
- **Escopos:**
  - `global`;
  - `fn:<nome>.params` / `proc:<nome>.params` para os parâmetros;
  - `.locals` para as variáveis locais;
  - `.block#n` para cada bloco `start...end` da sub-rotina.
- **Escopo de parâmetros e locais:** parâmetros e variáveis locais compartilham o mesmo escopo, de modo que redeclarar um parâmetro como local é considerado duplicidade.
- **Ordem do relatório:** o relatório `.ts` lista o escopo global e, em seguida, cada sub-rotina, preservando a ordem de inserção.
- **Declarações duplicadas no mesmo escopo são reportadas.** O enunciado exige isso da TS nesta fase.
- **Buscas por identificadores** são feitas durante a análise e registradas no `--trace`. Identificadores não declarados **não** geram erro nesta fase: essa validação, junto com a verificação de tipos, fica para a análise semântica (fase 2).

## Testes

A pasta `testes/` contém os programas de exemplo da especificação e casos de erro:

```sh
for f in testes/*.slac; do ./complac "$f"; done
```

Os arquivos `ok_*.slac` devem compilar com sucesso. Os `err_*.slac` devem parar com o erro indicado pelo nome (léxico, sintático ou de declaração duplicada).
