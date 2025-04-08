#include "compiler.h"
#include "scanner.h"

int line_number = -1;

void compile(char *source)
{
    setup_scanner(source);

    for (;;)
    {
        Token token = scan_token();

        if (line_number != token.line_number)
        {
            line_number = token.line_number;
            printf("%02d ", token.line_number);
        }
        else
        {
            printf("|  ");
        }

        printf("%d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_ERROR)
            break;

        if (token.type == TOKEN_EOF)
            break;
    }
}
