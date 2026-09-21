#include <stdio.h>
#include <stdlib.h>

typedef enum {
    sGLOBVARS,
    sLOCVARS,
    sKIND,
    sIDENTIF,
    sINT,
    sLOGIC,
    sCHR,
    sCTEINT,
    sFUNC,
    sPROC,
    sREF,
    sSTART,
    sEND,
    sECHO,
    sGET,
    sCASE,
    sOTHERWISE,
    sCHOOSE,
    sMATCH,
    sIMPLIC,
    sOTHERS,
    sFOR,
    sFROM,
    sTO,
    sBY,
    sDO,
    sWHILE,
    sVIRG,
    sPONTOVIRG,
    sABREP,
    sFECHAP,
    sDOISPONTO,
    sABRECHV,
    sFECHACHV,
} TAtomo

typedef struct {
    TAtomo tk;
    char[255] valor;
    int lin;
} TInfoAtomo;