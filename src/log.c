/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * log.c - Implementação da criação e escrita dos arquivos de log.
 */
#include <string.h>
#include "log.h"
#include "diag.h"

#define TAM_CAMINHO 1024

FILE *log_abrir(const char *fonte, const char *ext)
{
    char caminho[TAM_CAMINHO];
    const char *barra, *ponto;
    size_t base;
    FILE *arq;

    /* remove a extensão do fonte (apenas a do último componente do caminho) */
    barra = strrchr(fonte, '/');
    ponto = strrchr(fonte, '.');
    base = (ponto && (!barra || ponto > barra)) ? (size_t)(ponto - fonte) : strlen(fonte);

    if (base + strlen(ext) + 2 > TAM_CAMINHO)
        diag_error(DIAG_GERAL, 0, "caminho do arquivo de log muito longo");

    memcpy(caminho, fonte, base);
    caminho[base] = '.';
    strcpy(caminho + base + 1, ext);

    arq = fopen(caminho, "w");
    if (!arq)
        diag_error(DIAG_GERAL, 0, "não foi possível criar o arquivo de log '%s'", caminho);
    return arq;
}

void log_token(FILE *arq, const TInfoAtomo *at)
{
    if (arq)
        fprintf(arq, "%d  %s  \"%s\"\n", at->linha, lex_nome(at->tk), at->lexema);
}

void log_fechar(FILE *arq)
{
    if (arq)
        fclose(arq);
}
