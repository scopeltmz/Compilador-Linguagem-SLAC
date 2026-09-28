/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * log.h - Interface para criação dos arquivos de artefatos intermediários
 *         (.tk lista de tokens, .ts tabela de símbolos, .trc rastreamento).
 */
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include "lex.h"

/* Cria o arquivo de log com o nome do fonte e a extensão 'ext' (sem ponto). */
FILE *log_abrir(const char *fonte, const char *ext);

/* Grava um token no formato: <L>  <CAT>  "<LEX>" */
void log_token(FILE *arq, const TInfoAtomo *at);

void log_fechar(FILE *arq);

#endif
