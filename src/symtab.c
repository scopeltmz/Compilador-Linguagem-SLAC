/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * symtab.c - Implementação da Tabela de Símbolos. Cada escopo mantém seus
 *            símbolos em ordem de inserção e aponta para o escopo pai; os
 *            escopos fechados são preservados para o relatório final.
 */
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "diag.h"

typedef struct Escopo {
    char descr[TAM_ESCOPO];
    Simbolo *ini, *fim;
    struct Escopo *pai;
    struct Escopo *seguinte;    /* ordem de criação */
} Escopo;

static Escopo *primeiro = NULL, *ultimo = NULL, *atual = NULL;

static void *alocar(size_t n)
{
    void *p = calloc(1, n);
    if (!p)
        diag_error(DIAG_GERAL, 0, "memória insuficiente");
    return p;
}

static void copiar(char *dest, const char *orig, size_t tam)
{
    size_t n = strlen(orig);
    if (n >= tam)
        n = tam - 1;
    memcpy(dest, orig, n);
    dest[n] = '\0';
}

void ts_init(void)
{
    ts_open_scope("global");
}

void ts_free(void)
{
    Escopo *e = primeiro, *pe;
    Simbolo *s, *ps;

    while (e) {
        for (s = e->ini; s; s = ps) {
            ps = s->prox;
            free(s);
        }
        pe = e->seguinte;
        free(e);
        e = pe;
    }
    primeiro = ultimo = atual = NULL;
}

void ts_open_scope(const char *descr)
{
    Escopo *e = alocar(sizeof(Escopo));

    copiar(e->descr, descr, TAM_ESCOPO);
    e->pai = atual;
    if (ultimo)
        ultimo->seguinte = e;
    else
        primeiro = e;
    ultimo = e;
    atual = e;
}

void ts_close_scope(void)
{
    if (atual)
        atual = atual->pai;
}

void ts_set_scope_descr(const char *descr)
{
    if (atual)
        copiar(atual->descr, descr, TAM_ESCOPO);
}

static Simbolo *buscar_em(const Escopo *e, const char *lexema)
{
    Simbolo *s;
    for (s = e->ini; s; s = s->prox)
        if (strcmp(s->lexema, lexema) == 0)
            return s;
    return NULL;
}

Simbolo *ts_insert(const char *lexema, Categoria cat, Tipo tipo, int linha)
{
    Simbolo *s;

    if (buscar_em(atual, lexema))
        return NULL;

    s = alocar(sizeof(Simbolo));
    copiar(s->lexema, lexema, TAM_LEXEMA);
    copiar(s->escopo, atual->descr, TAM_ESCOPO);
    s->cat = cat;
    s->tipo = tipo;
    s->extra = -1;
    s->linha = linha;

    if (atual->fim)
        atual->fim->prox = s;
    else
        atual->ini = s;
    atual->fim = s;
    return s;
}

Simbolo *ts_lookup(const char *lexema)
{
    const Escopo *e;
    Simbolo *s;

    for (e = atual; e; e = e->pai)
        if ((s = buscar_em(e, lexema)) != NULL)
            return s;
    return NULL;
}

void ts_set_pending_type(Tipo tipo, int vetor, int tamanho)
{
    Simbolo *s;

    for (s = atual->ini; s; s = s->prox)
        if (s->tipo == TIPO_INDEF) {
            s->tipo = tipo;
            s->vetor = vetor;
            s->extra = vetor ? tamanho : -1;
        }
}

static const char *nome_categoria(Categoria c)
{
    static const char *nomes[] = { "var", "param", "param_ref", "proc", "func" };
    return nomes[c];
}

static const char *nome_tipo(Tipo t)
{
    static const char *nomes[] = { "?", "-", "int", "logic", "chr" };
    return nomes[t];
}

void ts_dump(FILE *arq)
{
    const Escopo *e;
    const Simbolo *s;

    if (!arq)
        return;
    for (e = primeiro; e; e = e->seguinte)
        for (s = e->ini; s; s = s->prox) {
            fprintf(arq, "SCOPE=%s  id=\"%s\"  cat=%s  tipo=%s%s  extra=",
                    s->escopo, s->lexema, nome_categoria(s->cat),
                    nome_tipo(s->tipo), s->vetor ? "[]" : "");
            if (s->extra >= 0)
                fprintf(arq, "%d\n", s->extra);
            else
                fputs("-\n", arq);
        }
}
