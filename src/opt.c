/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * opt.c - Interpretação dos parâmetros de linha de comando.
 *         Uso: complac <arquivo.slac> [--tokens] [--symtab] [--trace] [--verbose]
 */
#include <string.h>
#include "opt.h"

static Opcoes opcoes;

int opts_parse(int argc, char *argv[])
{
    int i;

    memset(&opcoes, 0, sizeof(opcoes));
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0)
            opcoes.tokens = 1;
        else if (strcmp(argv[i], "--symtab") == 0)
            opcoes.symtab = 1;
        else if (strcmp(argv[i], "--trace") == 0)
            opcoes.trace = 1;
        else if (strcmp(argv[i], "--verbose") == 0)
            opcoes.verbose = 1;
        else if (argv[i][0] == '-' || opcoes.arquivo)
            return 0;                     /* opção desconhecida ou mais de um fonte */
        else
            opcoes.arquivo = argv[i];
    }
    return opcoes.arquivo != NULL;
}

const Opcoes *opts_get(void)
{
    return &opcoes;
}

const char *opts_usage(void)
{
    return "uso: complac <arquivo.slac> [--tokens] [--symtab] [--trace] [--verbose]";
}
