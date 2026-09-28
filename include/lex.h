/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * lex.h - Interface do analisador léxico: categorias de átomos (tokens),
 *         estrutura do token e funções públicas de varredura.
 */
#ifndef LEX_H
#define LEX_H

#include <stdio.h>

#define TAM_LEXEMA 256   /* 255 caracteres + terminador */

typedef enum {
    /* palavras reservadas */
    sGLOBVARS, sLOCVARS, sKIND, sINT, sLOGIC, sCHR,
    sFUNC, sPROC, sREF, sSTART, sEND, sECHO, sGET,
    sCASE, sOTHERWISE, sCHOOSE, sMATCH, sOTHERS,
    sFOR, sFROM, sTO, sBY, sDO, sWHILE, sREPEAT, sUNTIL, sRETURN,
    /* identificadores e literais */
    sIDENTIF, sCTEINT, sCTECHAR, sSTRING,
    /* operadores */
    sATRIB, sSOMA, sSUBRAT, sMULT, sDIV,
    sIGUAL, sDIFERENTE, sMAIOR, sMENOR, sMAIORIGUAL, sMENORIGUAL,
    sAND, sOR, sNEG, sIMPLIC,
    /* delimitadores */
    sVIRG, sPONTOVIRG, sDOISPONTOS, sABREPAR, sFECHAPAR, sABRECOL, sFECHACOL,
    /* fim de arquivo */
    sEOF
} TAtomo;

typedef struct {
    TAtomo tk;
    char lexema[TAM_LEXEMA];
    int linha;
} TInfoAtomo;

/* Prepara a varredura de 'fonte'; 'log_tk' (opcional) recebe a lista de tokens. */
void lex_init(FILE *fonte, FILE *log_tk);

/* Devolve o próximo token do arquivo fonte. */
TInfoAtomo lex_next(void);

/* Nome da categoria (ex.: "sIDENTIF") e forma legível (ex.: "';'"). */
const char *lex_nome(TAtomo tk);
const char *lex_descricao(TAtomo tk);

#endif
