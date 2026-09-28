/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * diag.h - Interface de diagnósticos: centraliza mensagens de erro
 *          (que encerram a compilação) e o rastreamento da análise.
 */
#ifndef DIAG_H
#define DIAG_H

#include <stdio.h>

typedef enum { DIAG_GERAL, DIAG_LEXICO, DIAG_SINTATICO, DIAG_SEMANTICO } DiagTipo;

/* 'trace' (opcional) recebe as mensagens de diag_info; 'verbose' as ecoa em stderr. */
void diag_init(FILE *trace, int verbose);

/* Reporta um erro e interrompe a execução. linha <= 0 omite a localização. */
void diag_error(DiagTipo tipo, int linha, const char *fmt, ...);

/* Registra uma mensagem de rastreamento (sem efeito se desabilitado). */
void diag_info(const char *fmt, ...);

#endif
