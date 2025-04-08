#include "common.h"
#include "token.h"

#ifndef CWS_SCANNER_H

typedef struct
{
    char *start;
    char *current;
    int line_number;

} Scanner;

void init_scanner();
void setup_scanner(char *source);
Token scan_token();

#endif // !CWS_SCANNER_H
