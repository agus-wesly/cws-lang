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

int match(char c)
{
    if (c == scanner.current[0])
    {
        scanner.current++;
        return 1;
    }

    return 0;
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

static int is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

static int is_alpha(char c)
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
    case 'a':
        return match_token(1, 2, "nd", TOKEN_AND);
    case 'e':
        return match_token(1, 3, "lse", TOKEN_ELSE);
    case 'i':
        return match_token(1, 1, "f", TOKEN_IF);
    case 'n':
        return match_token(1, 2, "il", TOKEN_NIL);
    case 'o':
        return match_token(1, 1, "r", TOKEN_OR);
    case 'p':
        return match_token(1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return match_token(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return match_token(1, 4, "uper", TOKEN_SUPER);
    case 'l':
        return match_token(1, 2, "et", TOKEN_LET);
    case 'w':
        return match_token(1, 4, "hile", TOKEN_WHILE);
    case 'c':
        if ((scanner.current - scanner.start) > 1)
        {
            switch (scanner.start[1])
            {
            case 'l':
                return match_token(2, 3, "ass", TOKEN_CLASS);
            case 'o':
                return match_token(2, 3, "nst", TOKEN_CONST);
            }
        }
        break;

    case 'f': {
        if ((scanner.current - scanner.start) > 1)
        {
            switch (scanner.start[1])
            {
            case 'a':
                return match_token(2, 3, "lse", TOKEN_FALSE);
            case 'o':
                return match_token(2, 1, "r", TOKEN_FOR);
            case 'u':
                return match_token(2, 1, "n", TOKEN_FUN);
            }
        }
        break;
    }

    case 't': {
        if ((scanner.current - scanner.start) > 1)
        {
            switch (scanner.start[1])
            {
            case 'h':
                return match_token(2, 2, "is", TOKEN_THIS);
            case 'r':
                return match_token(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    }
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
        switch (c)
        {
        case ' ':
        case '\t':
        case '\n':
            advance();
            break;

        default:
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
    case ';':
        return make_token(TOKEN_SEMICOLON);
    case '?':
        return make_token(TOKEN_QUESTION_MARK);
    case ':':
        return make_token(TOKEN_COLON);

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
    }

    if (is_digit(cur))
        return number();

    if (is_alpha(cur))
        return identifier();

    return error_token("Compile error : Unrecognized token");
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
