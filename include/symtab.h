/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * symtab.h - Interface da Tabela de Símbolos com escopos aninhados:
 *            abertura/fechamento de escopos, inserção, busca e relatório.
 */
#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>
#include "lex.h"

#define TAM_ESCOPO 300

typedef enum { CAT_VAR, CAT_PARAM, CAT_PARAM_REF, CAT_PROC, CAT_FUNC } Categoria;
typedef enum { TIPO_INDEF, TIPO_NENHUM, TIPO_INT, TIPO_LOGIC, TIPO_CHR } Tipo;

typedef struct Simbolo {
    char lexema[TAM_LEXEMA];
    Categoria cat;
    Tipo tipo;
    int vetor;              /* 1 se arranjo */
    int extra;              /* tamanho do vetor ou nº de parâmetros; -1 se não se aplica */
    int linha;
    char escopo[TAM_ESCOPO];
    struct Simbolo *prox;
} Simbolo;

void ts_init(void);
void ts_free(void);

/* Abre um escopo filho do atual, identificado por 'descr' (ex.: "fn:SOMA"). */
void ts_open_scope(const char *descr);
void ts_close_scope(void);

/* Altera a descrição do escopo atual para as próximas inserções (ex.: ".locals"). */
void ts_set_scope_descr(const char *descr);

/* Insere no escopo atual; devolve NULL se o lexema já existir nesse escopo. */
Simbolo *ts_insert(const char *lexema, Categoria cat, Tipo tipo, int linha);

/* Busca do escopo atual até o global; devolve NULL se não encontrado. */
Simbolo *ts_lookup(const char *lexema);

/* Atribui tipo/tamanho aos símbolos do escopo atual ainda com TIPO_INDEF. */
void ts_set_pending_type(Tipo tipo, int vetor, int tamanho);

/* Grava a tabela consolidada, por escopo (ordem de criação) e ordem de inserção. */
void ts_dump(FILE *arq);

#endif
