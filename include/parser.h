/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * parser.h - Interface do Analisador Sintático Descendente Recursivo (ASDR).
 */
#ifndef PARSER_H
#define PARSER_H

/* Analisa o programa completo; requer lex_init e ts_init já executados.
 * Em caso de erro, a execução é interrompida via diag_error. */
void parse_program(void);

#endif
