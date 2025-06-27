#include "scanner.h"
#include "string.h"

Scanner scanner;

char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

char peek()
{
    return scanner.current[0];
}

char peek_next()
{
    return scanner.current[1];
}

char is_at_end()
{
    return peek() == '\0';
}

bool match(char c)
{
    if (c == scanner.current[0])
    {
        scanner.current++;
        return true;
    }

    return false;
}

static Token make_token(TokenType type)
{
    Token token;
    token.type = type;
    token.line_number = scanner.line_number;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);

    return token;
}

static Token error_token(char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.line_number = scanner.line_number;
    token.start = message;
    token.length = strlen(message);

    return token;
}

static bool is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static Token number()
{
    while (is_digit(peek()))
        advance();

    if (peek() == '.')
    {
        advance();
        while (is_digit(peek()))
            advance();
    }

    return make_token(TOKEN_NUMBER);
}

static TokenType match_token(int start, int rest_length, char *rest, TokenType type)
{
    int dist = (int)(scanner.current - scanner.start);
    int compare = (memcmp(scanner.start + start, rest, rest_length) == 0);
    if ((dist == rest_length + start) && compare)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType get_token_type()
{
    switch (scanner.start[0])
    {
    case 'a': {
        if ((scanner.current - scanner.start) > 1)
        {
            switch (scanner.start[1])
            {
            case 'n': {
                switch (scanner.start[2])
                {
                case 'u':
                    return match_token(3, 0, "", TOKEN_ANU);
                case 'd':
                    return match_token(3, 2, "ai", TOKEN_ANDAI);
                }
            }
            break;
            case 't': {
                return match_token(2, 2, "au", TOKEN_ATAU);
            }
            break;
            }
        }
        break;
    }
    case 'b': {
        if ((scanner.current - scanner.start) > 1)
        {
            if (scanner.start[1] == 'a')
            {
                switch (scanner.start[2])
                {
                case 'w':
                    return match_token(3, 3, "aan", TOKEN_BAWAAN);
                case 's':
                    return match_token(3, 2, "mi", TOKEN_BASMI);
                case 'l':
                    return match_token(3, 2, "ik", TOKEN_BALIK);
                }
            }
        }
        break;
    }
    case 'd':
        return match_token(1, 2, "an", TOKEN_DAN);
    case 'f':
        return match_token(1, 5, "ungsi", TOKEN_FUNGSI);
    case 'h':
        return match_token(1, 2, "al", TOKEN_HAL);
    case 'j': {
        if ((scanner.current - scanner.start) > 1)
        {
            switch (scanner.start[1])
            {
            case 'm':
                return match_token(2, 2, "lh", TOKEN_JMLH);
            case 'i':
                return match_token(2, 2, "ka", TOKEN_JIKA);
            }
        }
        break;
    }

    case 'k': {
        if ((scanner.current - scanner.start) > 1)
        {
            switch (scanner.start[1])
            {
            case 'e': {
                switch (scanner.start[2])
                {
                case 'l':
                    switch (scanner.start[3])
                    {
                    case 'a':
                        switch (scanner.start[4])
                        {
                        case 'r':
                            return match_token(5, 0, "", TOKEN_KELAR);
                        case 's':
                            return match_token(5, 0, "", TOKEN_KELAS);
                        }
                    }
                }
                break;
            }
            case 'a':
                return match_token(2, 3, "wal", TOKEN_KAWAL);
            case 'o':
                return match_token(2, 3, "nst", TOKEN_KONST);
            }
        }
        break;
    }
    case 'l':
        return match_token(1, 3, "agi", TOKEN_LAGI);
    case 'n':
        return match_token(1, 4, "ihil", TOKEN_NIHIL);
    case 'p':
        return match_token(1, 3, "ula", TOKEN_PULA);
    case 's': {
        if ((scanner.current - scanner.start) > 1)
        {
            switch (scanner.start[1])
            {
            case 'a': {
                switch (scanner.start[2])
                {
                case 'a':
                    return match_token(3, 1, "t", TOKEN_SAAT);
                case 'h':
                    return match_token(3, 0, "", TOKEN_SAH);
                }
            }
            break;
            case 'e':
                return match_token(2, 3, "sat", TOKEN_SESAT);
            case 'u':
                return match_token(2, 3, "per", TOKEN_SUPER);
            }
        }
        break;
    }
    case 't':
        return match_token(1, 5, "ampil", TOKEN_TAMPIL);
    case 'u':
        return match_token(1, 4, "lang", TOKEN_ULANG);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    for (;;)
    {
        char c = peek();
        if (is_alpha(c) || is_digit(c))
            advance();
        else
            break;
    }
    return make_token(get_token_type());
}

static Token comment()
{
    while (peek() != '\n' && !is_at_end())
        advance();

    return make_token(TOKEN_COMMENT);
}

static Token string()
{
    while (peek() != '"' && !is_at_end())
    {
        if (peek() == '\n')
            scanner.line_number++;
        advance();
    }

    if (peek() == '"')
    {
        advance();
        return make_token(TOKEN_STRING);
    }

    return make_token(TOKEN_ERROR);
}

static void trim()
{
    for (;;)
    {
        char c = peek();
        if (isspace(c) || (c > '\x00' && c < '\x08') || (c < 0))
        {
            advance();
        }
        else
        {
            return;
        }
        if (c == '\n')
        {
            scanner.line_number++;
        }
    }
}

Token scan_token()
{
    trim();
    scanner.start = scanner.current;

    char cur = advance();
    switch (cur)
    {
    case '{':
        return make_token(TOKEN_LEFT_BRACE);
    case '}':
        return make_token(TOKEN_RIGHT_BRACE);
    case '(':
        return make_token(TOKEN_LEFT_PAREN);
    case ')':
        return make_token(TOKEN_RIGHT_PAREN);
    case '[':
        return make_token(TOKEN_LEFT_SQR_BRACKET);
    case ']':
        return make_token(TOKEN_RIGHT_SQR_BRACKET);
    case ';':
        return make_token(TOKEN_SEMICOLON);
    case '?':
        return make_token(TOKEN_QUESTION_MARK);
    case ':':
        return make_token(TOKEN_COLON);
    case ',':
        return make_token(TOKEN_COMMA);

    case '+':
        if (match('='))
            return make_token(TOKEN_PLUS_EQUAL);
        return make_token(TOKEN_PLUS);

    case '-':
        if (match('='))
            return make_token(TOKEN_MINUS_EQUAL);
        return make_token(TOKEN_MINUS);

    case '*':
        return make_token(TOKEN_STAR);

    case '=':
        if (match('='))
            return make_token(TOKEN_EQUAL_EQUAL);
        return make_token(TOKEN_EQUAL);

    case '<':
        if (match('='))
            return make_token(TOKEN_LESS_EQUAL);
        return make_token(TOKEN_LESS);

    case '>':
        if (match('='))
            return make_token(TOKEN_GREATER_EQUAL);
        return make_token(TOKEN_GREATER);

    case '!':
        if (match('='))
            return make_token(TOKEN_BANG_EQUAL);
        return make_token(TOKEN_BANG);

    case '\0':
        return make_token(TOKEN_EOF);
    case '/':
        if (match('/'))
            return comment();

        return make_token(TOKEN_SLASH);

    case '"':
        return string();
    case '`':
        return make_token(TOKEN_BACKTICK);

    case '.':
        return make_token(TOKEN_DOT);
    }

    if (is_digit(cur))
        return number();

    if (is_alpha(cur))
        return identifier();

    printf("Unrecognized : %d\n", cur);
    return error_token("Kesalahan kompilasi : Token tidak dikenali");
}

void init_scanner(const char *source)
{
    scanner.start = NULL;
    scanner.current = source;
    scanner.line_number = 1;
}

void setup_scanner(char *source)
{
    scanner.current = source;
}
