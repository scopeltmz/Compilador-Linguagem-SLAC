/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * opt.h - Interface das opções de execução obtidas da linha de comando.
 */
#ifndef OPT_H
#define OPT_H

typedef struct {
    const char *arquivo;    /* caminho do fonte .slac */
    int tokens;             /* --tokens  : gera <fonte>.tk  */
    int symtab;             /* --symtab  : gera <fonte>.ts  */
    int trace;              /* --trace   : gera <fonte>.trc */
    int verbose;            /* --verbose : ecoa o rastreamento em stderr */
} Opcoes;

/* Interpreta argv; devolve 0 em caso de uso inválido. */
int opts_parse(int argc, char *argv[]);

const Opcoes *opts_get(void);

/* Texto de ajuda da linha de comando. */
const char *opts_usage(void);

#endif
