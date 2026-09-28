/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * main.c - Orquestração do compilador: interpreta a linha de comando, abre
 *          o fonte e os logs, inicializa os módulos, aciona a análise e
 *          encerra tudo de forma ordenada (inclusive após erros).
 */
#include <stdio.h>
#include <stdlib.h>
#include "opt.h"
#include "diag.h"
#include "log.h"
#include "lex.h"
#include "symtab.h"
#include "parser.h"

static FILE *fonte = NULL;
static FILE *arq_tk = NULL;
static FILE *arq_ts = NULL;
static FILE *arq_trc = NULL;

/* Registrada em atexit: executa também quando diag_error interrompe a análise. */
static void finalizar(void)
{
    ts_dump(arq_ts);
    ts_free();
    log_fechar(arq_tk);
    log_fechar(arq_ts);
    log_fechar(arq_trc);
    if (fonte)
        fclose(fonte);
}

int main(int argc, char *argv[])
{
    const Opcoes *op;

    diag_init(NULL, 0);
    if (!opts_parse(argc, argv))
        diag_error(DIAG_GERAL, 0, "%s", opts_usage());
    op = opts_get();

    atexit(finalizar);

    fonte = fopen(op->arquivo, "r");
    if (!fonte)
        diag_error(DIAG_GERAL, 0, "não foi possível abrir o arquivo '%s'", op->arquivo);

    if (op->tokens) arq_tk  = log_abrir(op->arquivo, "tk");
    if (op->symtab) arq_ts  = log_abrir(op->arquivo, "ts");
    if (op->trace)  arq_trc = log_abrir(op->arquivo, "trc");

    diag_init(arq_trc, op->verbose);
    lex_init(fonte, arq_tk);
    ts_init();

    parse_program();

    printf("%s: análise léxica e sintática concluída com sucesso.\n", op->arquivo);
    return EXIT_SUCCESS;
}
