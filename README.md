# COMPLAC — Compilador da linguagem SLAC²

**Autores:** Thomaz de Souza Scopel (RA 10417183) · Matteo Porcare (RA 10417286)

Fase 1: análise léxica, análise sintática (ASDR) e Tabela de Símbolos.

## Compilação

### Linux

```sh
make          # gera o binário ./complac
make clean    # remove objetos, binário e logs gerados
```

### Windows

Requer o GCC para Windows (MinGW-w64). Uma forma de instalá-lo é pelo PowerShell:

```powershell
winget install BrechtSanders.WinLibs.POSIX.UCRT
```

Reabra o terminal após a instalação e, na pasta do projeto, execute:

```powershell
mingw32-make          # gera o binário complac.exe
mingw32-make clean    # remove objetos, binário e logs gerados
```

## Execução

```sh
./complac <arquivo.slac> [--tokens] [--symtab] [--trace] [--verbose]     # Linux
.\complac.exe <arquivo.slac> [--tokens] [--symtab] [--trace] [--verbose] # Windows
```

- `--tokens`: gera `<arquivo>.tk` com a lista de tokens.
- `--symtab`: gera `<arquivo>.ts` com a tabela de símbolos.
- `--trace`: gera `<arquivo>.trc` com o rastreamento da análise.
- `--verbose`: exibe também o rastreamento no terminal.

As opções podem ser combinadas. A análise termina no primeiro erro, com mensagem indicando o tipo, a linha, o token esperado e o encontrado. Nesse caso o programa retorna o código 1.

## Estrutura

```
src/        código-fonte dos módulos (main, opt, lex, parser, symtab, diag, log)
include/    cabeçalhos públicos
testes/     programas de teste (ok_*: válidos, err_*: com erro)
```

## Observações

- **Programa principal:** `proc main()` deve ser a última sub-rotina do programa.
- **Corpo de `case`, `for` e `while`:** é uma lista de comandos terminados por `;`, fechada por `end`, conforme os exemplos da especificação.
- **Blocos `start...end`:** podem ser usados como comando.
- **Precedência:** os relacionais `< <= > >=` têm precedência maior que `= ~=`.
- **Constantes negativas:** são aceitas em `from`, `to`, `by` e `match` (ex.: `by -1`).
- **Escopos na tabela de símbolos:** `global`, `fn:<nome>.params`, `fn:<nome>.locals` e `fn:<nome>.block#n`. Procedimentos usam o prefixo `proc:`.
- **Erros da tabela de símbolos:** declarações duplicadas no mesmo escopo são reportadas. A verificação de identificadores não declarados e de tipos fica para a fase 2.
