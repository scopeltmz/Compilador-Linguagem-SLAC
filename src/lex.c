/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * lex.c - Analisador léxico: varre o fonte sequencialmente e devolve um
 *         token por chamada, ignorando espaços e comentários (# e /# #/).
 *         O sinal '-' é sempre sSUBRAT; unário ou binário é decidido pelo parser.
 */
#include <ctype.h>
#include <string.h>
#include "lex.h"
#include "log.h"
#include "diag.h"

#define MAX_DIGITOS 10   /* limite do inteiro de 32 bits: 2147483647 */

static FILE *src = NULL;
static FILE *arq_tk = NULL;
static int linha_atual = 1;

/* Pilha própria de devolução: ungetc garante apenas um caractere. */
static int pilha[2];
static int topo = 0;

static const struct { const char *nome; const char *desc; } tab_atomos[] = {
    {"sGLOBVARS", "'globvars'"}, {"sLOCVARS", "'locvars'"}, {"sKIND", "'is'"},
    {"sINT", "'int'"}, {"sLOGIC", "'logic'"}, {"sCHR", "'chr'"},
    {"sFUNC", "'func'"}, {"sPROC", "'proc'"}, {"sREF", "'ref'"},
    {"sSTART", "'start'"}, {"sEND", "'end'"}, {"sECHO", "'echo'"}, {"sGET", "'get'"},
    {"sCASE", "'case'"}, {"sOTHERWISE", "'otherwise'"}, {"sCHOOSE", "'choose'"},
    {"sMATCH", "'match'"}, {"sOTHERS", "'others'"},
    {"sFOR", "'for'"}, {"sFROM", "'from'"}, {"sTO", "'to'"}, {"sBY", "'by'"},
    {"sDO", "'do'"}, {"sWHILE", "'while'"}, {"sREPEAT", "'repeat'"},
    {"sUNTIL", "'until'"}, {"sRETURN", "'return'"},
    {"sIDENTIF", "identificador"}, {"sCTEINT", "constante inteira"},
    {"sCTECHAR", "constante caractere"}, {"sSTRING", "string"},
    {"sATRIB", "'<<'"}, {"sSOMA", "'+'"}, {"sSUBRAT", "'-'"}, {"sMULT", "'*'"},
    {"sDIV", "'//'"}, {"sIGUAL", "'='"}, {"sDIFERENTE", "'~='"}, {"sMAIOR", "'>'"},
    {"sMENOR", "'<'"}, {"sMAIORIGUAL", "'>='"}, {"sMENORIGUAL", "'<='"},
    {"sAND", "'&'"}, {"sOR", "'|'"}, {"sNEG", "'~'"}, {"sIMPLIC", "'->'"},
    {"sVIRG", "','"}, {"sPONTOVIRG", "';'"}, {"sDOISPONTOS", "':'"},
    {"sABREPAR", "'('"}, {"sFECHAPAR", "')'"}, {"sABRECOL", "'['"}, {"sFECHACOL", "']'"},
    {"sEOF", "fim de arquivo"}
};

static const struct { const char *lexema; TAtomo tk; } reservadas[] = {
    {"globvars", sGLOBVARS}, {"locvars", sLOCVARS}, {"is", sKIND},
    {"int", sINT}, {"logic", sLOGIC}, {"chr", sCHR},
    {"func", sFUNC}, {"proc", sPROC}, {"ref", sREF},
    {"start", sSTART}, {"end", sEND}, {"echo", sECHO}, {"get", sGET},
    {"case", sCASE}, {"otherwise", sOTHERWISE}, {"choose", sCHOOSE},
    {"match", sMATCH}, {"others", sOTHERS},
    {"for", sFOR}, {"from", sFROM}, {"to", sTO}, {"by", sBY}, {"do", sDO},
    {"while", sWHILE}, {"repeat", sREPEAT}, {"until", sUNTIL}, {"return", sRETURN}
};

const char *lex_nome(TAtomo tk)      { return tab_atomos[tk].nome; }
const char *lex_descricao(TAtomo tk) { return tab_atomos[tk].desc; }

void lex_init(FILE *fonte, FILE *log_tk)
{
    src = fonte;
    arq_tk = log_tk;
    linha_atual = 1;
    topo = 0;
}

static int ler(void)
{
    int c = topo > 0 ? pilha[--topo] : fgetc(src);
    if (c == '\n')
        linha_atual++;
    return c;
}

static void devolver(int c)
{
    if (c == EOF)
        return;
    if (c == '\n')
        linha_atual--;
    pilha[topo++] = c;
}

static int eh_letra(int c) { return isalpha(c) || c == '_'; }

/* Consome espaços e comentários até o início do próximo token. */
static void ignorar_brancos(void)
{
    int c, prox, lin_ini;

    for (;;) {
        c = ler();
        if (isspace(c))
            continue;
        if (c == '#') {                       /* comentário de linha */
            while ((c = ler()) != '\n' && c != EOF)
                ;
            continue;
        }
        if (c == '/') {
            prox = ler();
            if (prox == '#') {                /* comentário de bloco */
                lin_ini = linha_atual;
                c = ler();
                for (;;) {
                    if (c == EOF)
                        diag_error(DIAG_LEXICO, lin_ini, "comentário de bloco '/#' não finalizado com '#/'");
                    prox = ler();
                    if (c == '#' && prox == '/')
                        break;
                    c = prox;
                }
                continue;
            }
            devolver(prox);
        }
        devolver(c);
        return;
    }
}

static void ler_identificador(TInfoAtomo *at, int c)
{
    size_t n = 0, i;

    do {
        if (n >= TAM_LEXEMA - 1)
            diag_error(DIAG_LEXICO, at->linha, "identificador excede %d caracteres", TAM_LEXEMA - 1);
        at->lexema[n++] = (char)c;
        c = ler();
    } while (eh_letra(c) || isdigit(c));
    devolver(c);
    at->lexema[n] = '\0';

    at->tk = sIDENTIF;
    for (i = 0; i < sizeof(reservadas) / sizeof(reservadas[0]); i++)
        if (strcmp(at->lexema, reservadas[i].lexema) == 0) {
            at->tk = reservadas[i].tk;
            break;
        }
}

static void ler_numero(TInfoAtomo *at, int c)
{
    size_t n = 0;

    do {
        if (n < TAM_LEXEMA - 1)
            at->lexema[n++] = (char)c;
        c = ler();
    } while (isdigit(c));
    at->lexema[n] = '\0';

    if (eh_letra(c))
        diag_error(DIAG_LEXICO, at->linha, "constante numérica mal formada '%s%c'", at->lexema, c);
    devolver(c);
    if (n > MAX_DIGITOS || (n == MAX_DIGITOS && strcmp(at->lexema, "2147483647") > 0))
        diag_error(DIAG_LEXICO, at->linha, "constante inteira '%s' fora do intervalo permitido", at->lexema);
    at->tk = sCTEINT;
}

static void ler_string(TInfoAtomo *at)
{
    size_t n = 0;
    int c;

    while ((c = ler()) != '"') {
        if (c == '\n' || c == EOF)
            diag_error(DIAG_LEXICO, at->linha, "string não finalizada com '\"'");
        if (n >= TAM_LEXEMA - 1)
            diag_error(DIAG_LEXICO, at->linha, "string excede %d caracteres", TAM_LEXEMA - 1);
        at->lexema[n++] = (char)c;
    }
    at->lexema[n] = '\0';
    at->tk = sSTRING;
}

static void ler_caractere(TInfoAtomo *at)
{
    int c = ler();

    if (c == '\'')
        diag_error(DIAG_LEXICO, at->linha, "constante caractere vazia");
    if (c == '\n' || c == EOF)
        diag_error(DIAG_LEXICO, at->linha, "constante caractere não finalizada com \"'\"");
    at->lexema[0] = (char)c;
    at->lexema[1] = '\0';
    if (ler() != '\'')
        diag_error(DIAG_LEXICO, at->linha, "constante caractere deve conter um único caractere entre aspas simples");
    at->tk = sCTECHAR;
}

/* Reconhece operadores e delimitadores, com lookahead de um caractere. */
static void ler_simbolo(TInfoAtomo *at, int c)
{
    int prox = ler();
    int duplo = 1;

    at->lexema[0] = (char)c;
    switch (c) {
    case '<':
        if (prox == '<')      at->tk = sATRIB;
        else if (prox == '=') at->tk = sMENORIGUAL;
        else                { at->tk = sMENOR; duplo = 0; }
        break;
    case '>':
        if (prox == '=')      at->tk = sMAIORIGUAL;
        else                { at->tk = sMAIOR; duplo = 0; }
        break;
    case '~':
        if (prox == '=')      at->tk = sDIFERENTE;
        else                { at->tk = sNEG; duplo = 0; }
        break;
    case '-':
        if (prox == '>')      at->tk = sIMPLIC;
        else                { at->tk = sSUBRAT; duplo = 0; }
        break;
    case '/':
        if (prox == '/')      at->tk = sDIV;
        else
            diag_error(DIAG_LEXICO, at->linha, "símbolo '/' inválido (esperado '//' ou '/#')");
        break;
    default:
        duplo = 0;
        switch (c) {
        case '+': at->tk = sSOMA; break;
        case '*': at->tk = sMULT; break;
        case '=': at->tk = sIGUAL; break;
        case '&': at->tk = sAND; break;
        case '|': at->tk = sOR; break;
        case ',': at->tk = sVIRG; break;
        case ';': at->tk = sPONTOVIRG; break;
        case ':': at->tk = sDOISPONTOS; break;
        case '(': at->tk = sABREPAR; break;
        case ')': at->tk = sFECHAPAR; break;
        case '[': at->tk = sABRECOL; break;
        case ']': at->tk = sFECHACOL; break;
        default:
            if (isprint(c))
                diag_error(DIAG_LEXICO, at->linha, "caractere inválido '%c'", c);
            diag_error(DIAG_LEXICO, at->linha, "caractere inválido (código %d)", c);
        }
    }

    if (duplo) {
        at->lexema[1] = (char)prox;
        at->lexema[2] = '\0';
    } else {
        at->lexema[1] = '\0';
        devolver(prox);
    }
}

TInfoAtomo lex_next(void)
{
    TInfoAtomo at;
    int c;

    ignorar_brancos();
    at.linha = linha_atual;
    at.lexema[0] = '\0';
    c = ler();

    if (c == EOF)
        at.tk = sEOF;
    else if (eh_letra(c))
        ler_identificador(&at, c);
    else if (isdigit(c))
        ler_numero(&at, c);
    else if (c == '"')
        ler_string(&at);
    else if (c == '\'')
        ler_caractere(&at);
    else
        ler_simbolo(&at, c);

    if (at.tk != sEOF)
        log_token(arq_tk, &at);
    return at;
}
