/*
 * COMPLAC - Compilador da linguagem SLAC²
 * Autores: Thomaz de Souza Scopel (RA 10417183)
 *          Matteo Porcare (RA 10417286)
 *
 * parser.c - ASDR da SLAC²: cada não-terminal da gramática (Apêndice A) é uma
 *            função p_<nome>. O parser também abre/fecha escopos e alimenta a
 *            Tabela de Símbolos, reportando declarações duplicadas no mesmo
 *            escopo, como exigido para a TS nesta fase.
 *
 * Ajustes à EBNF, conforme o texto e os exemplos da especificação:
 *  - <princ> ::= sPROC "main" "(" ")" [<lvars>] <bco>
 *  - case/for/while têm corpo {<cmd> ";"} fechado por sEND, e case aceita
 *    otherwise com lista de comandos; repeat segue a EBNF.
 *  - um bloco start...end é aceito como comando (abre escopo próprio).
 *  - <elem> é tratado como <expr>, pois literais, identificadores, vetores e
 *    chamadas já são fatores da expressão.
 *  - relacionais (<,<=,>,>=) têm precedência maior que igualdade (=,~=),
 *    conforme a tabela de precedência (não-terminal extra <ecomp>).
 *  - constantes inteiras em from/to/by/match aceitam sinal '-' (ex.: by -1).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "lex.h"
#include "symtab.h"
#include "diag.h"

static TInfoAtomo atual, seguinte;
static int tem_seguinte = 0;
static int nivel = 0;
static char rotina[TAM_ESCOPO - 20]; /* prefixo do escopo da sub-rotina; folga para os sufixos */
static int cont_blocos = 0;

#define ENTRA(nt) diag_info("%*s-> <%s>  (linha %d)", 2 * nivel++, "", nt, atual.linha)
#define SAI(nt)   diag_info("%*s<- <%s>", 2 * --nivel, "", nt)

/* ---------- controle de tokens ---------- */

static void avancar(void)
{
    diag_info("%*s   %s \"%s\"", 2 * nivel, "", lex_nome(atual.tk), atual.lexema);
    if (tem_seguinte) {
        atual = seguinte;
        tem_seguinte = 0;
    } else {
        atual = lex_next();
    }
}

/* Lookahead de dois tokens, usado para decidir entre id, vetor e chamada. */
static const TInfoAtomo *espiar(void)
{
    if (!tem_seguinte) {
        seguinte = lex_next();
        tem_seguinte = 1;
    }
    return &seguinte;
}

static void erro_esperado(const char *esperado)
{
    if (atual.tk == sIDENTIF || atual.tk == sCTEINT || atual.tk == sCTECHAR || atual.tk == sSTRING)
        diag_error(DIAG_SINTATICO, atual.linha, "esperado %s, encontrado %s \"%s\"",
                   esperado, lex_descricao(atual.tk), atual.lexema);
    diag_error(DIAG_SINTATICO, atual.linha, "esperado %s, encontrado %s",
               esperado, lex_descricao(atual.tk));
}

static void consumir(TAtomo tk)
{
    if (atual.tk != tk)
        erro_esperado(lex_descricao(tk));
    avancar();
}

/* ---------- escopos e tabela de símbolos ---------- */

static void abrir_rotina(const char *tipo, const char *nome)
{
    char descr[TAM_ESCOPO];

    snprintf(rotina, sizeof(rotina), "%s:%s", tipo, nome);
    snprintf(descr, sizeof(descr), "%s.params", rotina);
    ts_open_scope(descr);
    cont_blocos = 0;
    diag_info("%*s[abre escopo %s]", 2 * nivel, "", rotina);
}

static void fechar_escopo(void)
{
    ts_close_scope();
    diag_info("%*s[fecha escopo]", 2 * nivel, "");
}

static void p_id(char *lexema, int *linha)
{
    ENTRA("id");
    if (atual.tk != sIDENTIF)
        erro_esperado(lex_descricao(sIDENTIF));
    strcpy(lexema, atual.lexema);
    *linha = atual.linha;
    avancar();
    SAI("id");
}

static Simbolo *declarar(Categoria cat, Tipo tipo)
{
    char lex[TAM_LEXEMA];
    int lin;
    Simbolo *s;

    p_id(lex, &lin);
    s = ts_insert(lex, cat, tipo, lin);
    if (!s)
        diag_error(DIAG_SEMANTICO, lin, "identificador '%s' já declarado neste escopo", lex);
    return s;
}

/* Uso de identificador: a busca na TS é apenas registrada no rastreamento;
 * a validação de identificadores não declarados cabe à análise semântica (fase 2). */
static void referenciar(void)
{
    char lex[TAM_LEXEMA];
    int lin;
    Simbolo *s;

    p_id(lex, &lin);
    s = ts_lookup(lex);
    diag_info("%*s[busca '%s': %s]", 2 * nivel, "", lex, s ? s->escopo : "não encontrado");
}

/* ---------- protótipos ---------- */

static void p_gvars(void);
static void p_lvars(void);
static void p_decls(Categoria cat);
static Tipo p_type(int *vetor, int *tam);
static void p_subs(void);
static void p_func(void);
static void p_proc(void);
static void p_princ(void);
static int  p_param(void);
static void p_bco(void);
static void p_cmd(void);
static void p_vetr(void);
static void p_echo(void);
static void p_get(void);
static void p_case(void);
static void p_chse(void);
static void p_mlst(void);
static void p_mtch(void);
static void p_mvlrs(void);
static void p_othr(void);
static void p_for(void);
static void p_whle(void);
static void p_rept(void);
static void p_call(void);
static void p_ret(void);
static void p_atr(void);
static void p_elem(void);
static void p_litl(void);
static void p_expr(void);
static void p_elogc(void);
static void p_erlac(void);
static void p_ecomp(void);
static void p_earit(void);
static void p_earip(void);
static void p_fact(void);
static void p_oprel(void);
static void p_opari(void);
static void p_oparp(void);

/* ---------- auxiliares ---------- */

static int inicia_cmd(TAtomo tk)
{
    switch (tk) {
    case sECHO: case sGET: case sCASE: case sCHOOSE: case sFOR: case sWHILE:
    case sREPEAT: case sRETURN: case sIDENTIF: case sSTART:
        return 1;
    default:
        return 0;
    }
}

/* {<cmd> ";"} */
static void lista_cmds(void)
{
    while (inicia_cmd(atual.tk)) {
        p_cmd();
        consumir(sPONTOVIRG);
    }
}

/* ["-"] sCTEINT */
static void cte_inteira(void)
{
    if (atual.tk == sSUBRAT)
        avancar();
    consumir(sCTEINT);
}

/* <id> | ["-"] sCTEINT  (limites do for) */
static void limite_for(void)
{
    if (atual.tk == sIDENTIF)
        referenciar();
    else
        cte_inteira();
}

/* ---------- estrutura do programa ---------- */

/* <prg> ::= [<gvars>] {<subs>} <princ> */
static void p_prg(void)
{
    ENTRA("prg");
    if (atual.tk == sGLOBVARS)
        p_gvars();
    while (atual.tk == sFUNC ||
           (atual.tk == sPROC && !(espiar()->tk == sIDENTIF && strcmp(seguinte.lexema, "main") == 0)))
        p_subs();
    p_princ();
    if (atual.tk != sEOF)
        erro_esperado("fim de arquivo após o procedimento main");
    SAI("prg");
}

/* <gvars> ::= sGLOBVARS <decls> {<decls>} */
static void p_gvars(void)
{
    ENTRA("gvars");
    consumir(sGLOBVARS);
    do
        p_decls(CAT_VAR);
    while (atual.tk == sIDENTIF);
    SAI("gvars");
}

/* <lvars> ::= sLOCVARS <decls> {<decls>} */
static void p_lvars(void)
{
    char descr[TAM_ESCOPO];

    ENTRA("lvars");
    snprintf(descr, sizeof(descr), "%s.locals", rotina);
    ts_set_scope_descr(descr);
    consumir(sLOCVARS);
    do
        p_decls(CAT_VAR);
    while (atual.tk == sIDENTIF);
    SAI("lvars");
}

/* <decls> ::= <id> {"," <id>} sKIND <type> ";" */
static void p_decls(Categoria cat)
{
    Tipo t;
    int vetor, tam;

    ENTRA("decls");
    declarar(cat, TIPO_INDEF);
    while (atual.tk == sVIRG) {
        avancar();
        declarar(cat, TIPO_INDEF);
    }
    consumir(sKIND);
    t = p_type(&vetor, &tam);
    ts_set_pending_type(t, vetor, tam);
    consumir(sPONTOVIRG);
    SAI("decls");
}

/* <type> ::= (sINT | sLOGIC | sCHR) ["[" sCTEINT "]"] */
static Tipo p_type(int *vetor, int *tam)
{
    Tipo t = TIPO_INDEF;

    ENTRA("type");
    switch (atual.tk) {
    case sINT:   t = TIPO_INT;   break;
    case sLOGIC: t = TIPO_LOGIC; break;
    case sCHR:   t = TIPO_CHR;   break;
    default:     erro_esperado("tipo ('int', 'logic' ou 'chr')");
    }
    avancar();

    *vetor = 0;
    *tam = 0;
    if (atual.tk == sABRECOL) {
        avancar();
        if (atual.tk == sCTEINT)
            *tam = atoi(atual.lexema);
        consumir(sCTEINT);
        consumir(sFECHACOL);
        *vetor = 1;
    }
    SAI("type");
    return t;
}

/* <subs> ::= <func> | <proc> */
static void p_subs(void)
{
    ENTRA("subs");
    if (atual.tk == sFUNC)
        p_func();
    else
        p_proc();
    SAI("subs");
}

/* <func> ::= sFUNC <id> "(" [<param>] ")" ":" <type> [<lvars>] <bco> */
static void p_func(void)
{
    Simbolo *s;
    int tam;

    ENTRA("func");
    consumir(sFUNC);
    s = declarar(CAT_FUNC, TIPO_NENHUM);
    abrir_rotina("fn", s->lexema);
    consumir(sABREPAR);
    s->extra = (atual.tk == sREF || atual.tk == sIDENTIF) ? p_param() : 0;
    consumir(sFECHAPAR);
    consumir(sDOISPONTOS);
    s->tipo = p_type(&s->vetor, &tam);
    if (atual.tk == sLOCVARS)
        p_lvars();
    p_bco();
    fechar_escopo();
    SAI("func");
}

/* <proc> ::= sPROC <id> "(" [<param>] ")" [<lvars>] <bco> */
static void p_proc(void)
{
    Simbolo *s;

    ENTRA("proc");
    consumir(sPROC);
    s = declarar(CAT_PROC, TIPO_NENHUM);
    abrir_rotina("proc", s->lexema);
    consumir(sABREPAR);
    s->extra = (atual.tk == sREF || atual.tk == sIDENTIF) ? p_param() : 0;
    consumir(sFECHAPAR);
    if (atual.tk == sLOCVARS)
        p_lvars();
    p_bco();
    fechar_escopo();
    SAI("proc");
}

/* <princ> ::= sPROC "main" "(" ")" [<lvars>] <bco> */
static void p_princ(void)
{
    Simbolo *s;

    ENTRA("princ");
    if (atual.tk != sPROC)
        erro_esperado("procedimento principal 'proc main()'");
    consumir(sPROC);
    if (atual.tk != sIDENTIF || strcmp(atual.lexema, "main") != 0)
        erro_esperado("'main'");
    s = declarar(CAT_PROC, TIPO_NENHUM);
    s->extra = 0;
    abrir_rotina("proc", "main");
    consumir(sABREPAR);
    consumir(sFECHAPAR);
    if (atual.tk == sLOCVARS)
        p_lvars();
    p_bco();
    fechar_escopo();
    SAI("princ");
}

/* <param> ::= [sREF] <id> ":" <type> {"," [sREF] <id> ":" <type>} ; devolve a quantidade */
static int p_param(void)
{
    Categoria cat;
    Tipo t;
    int vetor, tam, n = 0;

    ENTRA("param");
    for (;;) {
        cat = CAT_PARAM;
        if (atual.tk == sREF) {
            avancar();
            cat = CAT_PARAM_REF;
        }
        declarar(cat, TIPO_INDEF);
        consumir(sDOISPONTOS);
        t = p_type(&vetor, &tam);
        ts_set_pending_type(t, vetor, tam);
        n++;
        if (atual.tk != sVIRG)
            break;
        avancar();
    }
    SAI("param");
    return n;
}

/* <bco> ::= sSTART {<cmd> ";"} sEND */
static void p_bco(void)
{
    char descr[TAM_ESCOPO];

    ENTRA("bco");
    consumir(sSTART);
    snprintf(descr, sizeof(descr), "%s.block#%d", rotina, ++cont_blocos);
    ts_open_scope(descr);
    lista_cmds();
    consumir(sEND);
    ts_close_scope();
    SAI("bco");
}

/* ---------- comandos ---------- */

/* <cmd> ::= <echo> | <get> | <case> | <chse> | <for> | <whle> | <rept> | <call> | <ret> | <atr> | <bco> */
static void p_cmd(void)
{
    ENTRA("cmd");
    switch (atual.tk) {
    case sECHO:   p_echo(); break;
    case sGET:    p_get();  break;
    case sCASE:   p_case(); break;
    case sCHOOSE: p_chse(); break;
    case sFOR:    p_for();  break;
    case sWHILE:  p_whle(); break;
    case sREPEAT: p_rept(); break;
    case sRETURN: p_ret();  break;
    case sSTART:  p_bco();  break;
    case sIDENTIF:
        if (espiar()->tk == sABREPAR)
            p_call();
        else
            p_atr();
        break;
    default:
        erro_esperado("comando");
    }
    SAI("cmd");
}

/* <vetr> ::= <id> "[" (sCTEINT | <id>) "]" */
static void p_vetr(void)
{
    ENTRA("vetr");
    referenciar();
    consumir(sABRECOL);
    if (atual.tk == sIDENTIF)
        referenciar();
    else if (atual.tk == sCTEINT)
        avancar();
    else
        erro_esperado("índice (constante inteira ou identificador)");
    consumir(sFECHACOL);
    SAI("vetr");
}

/* <echo> ::= sECHO "(" <elem> {"," <elem>} ")" */
static void p_echo(void)
{
    ENTRA("echo");
    consumir(sECHO);
    consumir(sABREPAR);
    p_elem();
    while (atual.tk == sVIRG) {
        avancar();
        p_elem();
    }
    consumir(sFECHAPAR);
    SAI("echo");
}

/* <get> ::= sGET "(" (<id> | <vetr>) ")" */
static void p_get(void)
{
    ENTRA("get");
    consumir(sGET);
    consumir(sABREPAR);
    if (atual.tk == sIDENTIF && espiar()->tk == sABRECOL)
        p_vetr();
    else
        referenciar();
    consumir(sFECHAPAR);
    SAI("get");
}

/* <case> ::= sCASE "(" <expr> ")" {<cmd> ";"} [sOTHERWISE {<cmd> ";"}] sEND */
static void p_case(void)
{
    ENTRA("case");
    consumir(sCASE);
    consumir(sABREPAR);
    p_expr();
    consumir(sFECHAPAR);
    lista_cmds();
    if (atual.tk == sOTHERWISE) {
        avancar();
        lista_cmds();
    }
    consumir(sEND);
    SAI("case");
}

/* <chse> ::= sCHOOSE "(" <expr> ")" <mlst> sEND */
static void p_chse(void)
{
    ENTRA("chse");
    consumir(sCHOOSE);
    consumir(sABREPAR);
    p_expr();
    consumir(sFECHAPAR);
    p_mlst();
    consumir(sEND);
    SAI("chse");
}

/* <mlst> ::= <mtch> {<mtch>} [<othr>] */
static void p_mlst(void)
{
    ENTRA("mlst");
    do
        p_mtch();
    while (atual.tk == sMATCH);
    if (atual.tk == sOTHERS)
        p_othr();
    SAI("mlst");
}

/* <mtch> ::= sMATCH <mvlrs> sIMPLIC <cmd> ";" */
static void p_mtch(void)
{
    ENTRA("mtch");
    consumir(sMATCH);
    p_mvlrs();
    consumir(sIMPLIC);
    p_cmd();
    consumir(sPONTOVIRG);
    SAI("mtch");
}

/* <mvlrs> ::= sCTEINT {"," sCTEINT} */
static void p_mvlrs(void)
{
    ENTRA("mvlrs");
    cte_inteira();
    while (atual.tk == sVIRG) {
        avancar();
        cte_inteira();
    }
    SAI("mvlrs");
}

/* <othr> ::= sOTHERS sIMPLIC <cmd> ";" */
static void p_othr(void)
{
    ENTRA("othr");
    consumir(sOTHERS);
    consumir(sIMPLIC);
    p_cmd();
    consumir(sPONTOVIRG);
    SAI("othr");
}

/* <for> ::= sFOR <id> sFROM (<id>|sCTEINT) sTO (<id>|sCTEINT) [sBY sCTEINT] sDO {<cmd> ";"} sEND */
static void p_for(void)
{
    ENTRA("for");
    consumir(sFOR);
    referenciar();
    consumir(sFROM);
    limite_for();
    consumir(sTO);
    limite_for();
    if (atual.tk == sBY) {
        avancar();
        cte_inteira();
    }
    consumir(sDO);
    lista_cmds();
    consumir(sEND);
    SAI("for");
}

/* <whle> ::= sWHILE "(" <expr> ")" sDO {<cmd> ";"} sEND */
static void p_whle(void)
{
    ENTRA("whle");
    consumir(sWHILE);
    consumir(sABREPAR);
    p_expr();
    consumir(sFECHAPAR);
    consumir(sDO);
    lista_cmds();
    consumir(sEND);
    SAI("whle");
}

/* <rept> ::= sREPEAT {<cmd> ";"} sUNTIL "(" <expr> ")" */
static void p_rept(void)
{
    ENTRA("rept");
    consumir(sREPEAT);
    lista_cmds();
    consumir(sUNTIL);
    consumir(sABREPAR);
    p_expr();
    consumir(sFECHAPAR);
    SAI("rept");
}

/* <call> ::= <id> "(" [<expr> {"," <expr>}] ")" */
static void p_call(void)
{
    ENTRA("call");
    referenciar();
    consumir(sABREPAR);
    if (atual.tk != sFECHAPAR) {
        p_expr();
        while (atual.tk == sVIRG) {
            avancar();
            p_expr();
        }
    }
    consumir(sFECHAPAR);
    SAI("call");
}

/* <ret> ::= sRETURN <elem> */
static void p_ret(void)
{
    ENTRA("ret");
    consumir(sRETURN);
    p_elem();
    SAI("ret");
}

/* <atr> ::= (<id> | <vetr>) sATRIB <elem> */
static void p_atr(void)
{
    ENTRA("atr");
    if (espiar()->tk == sABRECOL)
        p_vetr();
    else
        referenciar();
    consumir(sATRIB);
    p_elem();
    SAI("atr");
}

/* <elem> ::= <litl> | <id> | <vetr> | <call> | <expr>  (todas as formas derivam de <expr>) */
static void p_elem(void)
{
    ENTRA("elem");
    p_expr();
    SAI("elem");
}

/* <litl> ::= sSTRING | sCTEINT | sCTECHAR */
static void p_litl(void)
{
    ENTRA("litl");
    avancar();
    SAI("litl");
}

/* ---------- expressões (da menor para a maior precedência) ---------- */

/* <expr> ::= <elogc> {sOR <elogc>} */
static void p_expr(void)
{
    ENTRA("expr");
    p_elogc();
    while (atual.tk == sOR) {
        avancar();
        p_elogc();
    }
    SAI("expr");
}

/* <elogc> ::= <erlac> {sAND <erlac>} */
static void p_elogc(void)
{
    ENTRA("elogc");
    p_erlac();
    while (atual.tk == sAND) {
        avancar();
        p_erlac();
    }
    SAI("elogc");
}

/* <erlac> ::= <ecomp> {(sIGUAL | sDIFERENTE) <ecomp>} */
static void p_erlac(void)
{
    ENTRA("erlac");
    p_ecomp();
    while (atual.tk == sIGUAL || atual.tk == sDIFERENTE) {
        p_oprel();
        p_ecomp();
    }
    SAI("erlac");
}

/* <ecomp> ::= <earit> {(sMAIOR | sMAIORIGUAL | sMENOR | sMENORIGUAL) <earit>} */
static void p_ecomp(void)
{
    ENTRA("ecomp");
    p_earit();
    while (atual.tk == sMAIOR || atual.tk == sMAIORIGUAL ||
           atual.tk == sMENOR || atual.tk == sMENORIGUAL) {
        p_oprel();
        p_earit();
    }
    SAI("ecomp");
}

/* <earit> ::= <earip> {<opari> <earip>} */
static void p_earit(void)
{
    ENTRA("earit");
    p_earip();
    while (atual.tk == sSOMA || atual.tk == sSUBRAT) {
        p_opari();
        p_earip();
    }
    SAI("earit");
}

/* <earip> ::= <fact> {<oparp> <fact>} */
static void p_earip(void)
{
    ENTRA("earip");
    p_fact();
    while (atual.tk == sMULT || atual.tk == sDIV) {
        p_oparp();
        p_fact();
    }
    SAI("earip");
}

/* <fact> ::= <elem> | sNEG <fact> | sSUBRAT <fact> | "(" <expr> ")" */
static void p_fact(void)
{
    ENTRA("fact");
    switch (atual.tk) {
    case sNEG:
        avancar();
        p_fact();
        break;
    case sSUBRAT:                         /* '-' em posição de fator: operador unário */
        diag_info("%*s[sSUBRAT unário]", 2 * nivel, "");
        avancar();
        p_fact();
        break;
    case sABREPAR:
        avancar();
        p_expr();
        consumir(sFECHAPAR);
        break;
    case sSTRING: case sCTEINT: case sCTECHAR:
        p_litl();
        break;
    case sIDENTIF:
        if (espiar()->tk == sABREPAR)
            p_call();
        else if (seguinte.tk == sABRECOL)
            p_vetr();
        else
            referenciar();
        break;
    default:
        erro_esperado("expressão");
    }
    SAI("fact");
}

/* <oprel> ::= sMAIOR | sMAIORIGUAL | sIGUAL | sMENOR | sMENORIGUAL | sDIFERENTE */
static void p_oprel(void)
{
    ENTRA("oprel");
    avancar();
    SAI("oprel");
}

/* <opari> ::= sSOMA | sSUBRAT */
static void p_opari(void)
{
    ENTRA("opari");
    avancar();
    SAI("opari");
}

/* <oparp> ::= sMULT | sDIV */
static void p_oparp(void)
{
    ENTRA("oparp");
    avancar();
    SAI("oparp");
}

void parse_program(void)
{
    nivel = 0;
    tem_seguinte = 0;
    atual = lex_next();
    p_prg();
}
