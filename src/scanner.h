#include "common.h"
#include "token.h"

#ifndef CWS_SCANNER_H

typedef struct
{
    const char *start;
    const char *current;
    int line_number;

} Scanner;

void init_scanner(const char *source);
void setup_scanner(char *source);
Token scan_token();


char advance();
char expression();
void consume(TokenType type, char *message);

#endif // !CWS_SCANNER_H
