#ifndef CWS_SCANNER_H
#define CWS_SCANNER_H

#include "common.h"
#include "token.h"

typedef struct
{
    const char *start;
    const char *current;
    int line_number;

} Scanner;

void init_scanner(const char *source);
void setup_scanner(char *source);
Token scan_token();


#endif // !CWS_SCANNER_H
