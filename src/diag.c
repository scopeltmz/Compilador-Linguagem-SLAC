/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * diag.c - Implementação dos diagnósticos: formatação padronizada de erros
 *          e gravação do rastreamento (--trace).
 */
#include <stdarg.h>
#include <stdlib.h>
#include "diag.h"

static FILE *arq_trace = NULL;
static int modo_verbose = 0;

static const char *nome_tipo[] = { "Erro", "Erro léxico", "Erro sintático", "Erro semântico" };

void diag_init(FILE *trace, int verbose)
{
    arq_trace = trace;
    modo_verbose = verbose;
}

void diag_error(DiagTipo tipo, int linha, const char *fmt, ...)
{
    va_list ap;

    if (linha > 0)
        fprintf(stderr, "%s na linha %d: ", nome_tipo[tipo], linha);
    else
        fprintf(stderr, "%s: ", nome_tipo[tipo]);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);

    if (arq_trace) {
        fprintf(arq_trace, "%s (linha %d): ", nome_tipo[tipo], linha);
        va_start(ap, fmt);
        vfprintf(arq_trace, fmt, ap);
        va_end(ap);
        fputc('\n', arq_trace);
    }
    /* o encerramento ordenado dos módulos é feito pela rotina registrada em atexit no main */
    exit(EXIT_FAILURE);
}

void diag_info(const char *fmt, ...)
{
    va_list ap;

    if (arq_trace) {
        va_start(ap, fmt);
        vfprintf(arq_trace, fmt, ap);
        va_end(ap);
        fputc('\n', arq_trace);
    }
    if (modo_verbose) {
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fputc('\n', stderr);
    }
}
