#include "compiler.h"

int line_number = -1;

typedef struct
{
    Token previous;
    Token current;
    int is_error;
    int is_panic;
} Parser;

Parser parser;
Chunk *compiling_chunk;


Chunk *current_chunk()
{
    return compiling_chunk;
}

static void error_at(Token token, char *message)
{
    if (parser.is_panic)
        return;

    parser.is_panic = 1;

    fprintf(stderr, "[line %d] Error : %s", parser.current.line_number, message);
    if (token.type == TOKEN_EOF)
    {
        fprintf(stderr, " at the end\n");
    }
    else if (token.type == TOKEN_ERROR)
    {
    }
    else
    {
        fprintf(stderr, " at %.*s\n", token.line_number, token.start);
    }

    parser.is_error = 1;
}

static void error_current(char *message)
{
    error_at(parser.current, message);
}

static void error(char *message)
{
    return error_at(parser.previous, message);
}

static void advance()
{
    parser.previous = parser.current;
    for (;;)
    {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR)
            break;

        error_current("Unrecognized type of token");
    }
}

static void expression()
{
    parsePresedence(PREC_ASSIGNMENT);
}

static void consume(TokenType token_type, char *message)
{
    if (parser.current.type == token_type)
    {
        advance();
        return;
    }

    error_current(message);
}

void emit_byte(uint8_t byte)
{
    WriteChunk(current_chunk(), byte, parser.previous.line_number);
}

void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

void emit_return()
{
    emit_byte(OP_RETURN);
}

void end_compiler()
{
    emit_return();
}

/*====== PREFIX & INFIX ====== */

Presedence _get_presedence(TokenType token_type)
{
    switch (token_type)
    {
    case TOKEN_EOF:
        return PREC_NONE;
    case TOKEN_LEFT_PAREN:
        return PREC_NONE;
    case TOKEN_RIGHT_PAREN:
        return PREC_NONE;
    case TOKEN_PLUS:
        return PREC_TERM;
    case TOKEN_MINUS:
        return PREC_TERM;
    case TOKEN_STAR:
        return PREC_FACTOR;
    case TOKEN_SLASH:
        return PREC_FACTOR;
    default:
        return PREC_NONE;
        // another case goes here
    }
}

static void number()
{
    Value value = strtod(parser.previous.start, NULL);
    WriteConstant(current_chunk(), value, parser.previous.line_number);
    // uint8_t idx = AddConstant(current_chunk(), value);
    // emit_bytes(OP_CONSTANT, idx);
}

static void unary()
{
    TokenType token_type = parser.previous.type;
    parsePresedence(get_rule(token_type)->presedence + 1);

    switch (token_type)
    {
    case TOKEN_MINUS: {
        emit_byte(OP_NEGATE);
        break;
    };
    // `!` goes here
    default: {
        assert(0 && "Unreachable at unary");
        break;
    }; // Unreachable
    }
}



static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected closing parentheses");
}


static void binary()
{
    TokenType token_type = parser.previous.type;
    Presedence presedence = get_rule(token_type)->presedence;
    parsePresedence(presedence + 1);

    switch (token_type)
    {
    case TOKEN_PLUS: {
        emit_byte(OP_ADD);
        break;
    };
    case TOKEN_MINUS: {
        emit_byte(OP_SUBTRACT);
        break;
    };
    case TOKEN_STAR: {
        emit_byte(OP_MULTIPLY);
        break;
    };
    case TOKEN_SLASH: {
        emit_byte(OP_DIVIDE);
        break;
    };
    default: {
        break;
    };
    }
}
/*===================== */


ParseRule rules[] = {
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_PRIMARY},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

ParseRule *get_rule(TokenType token_type) {
    return &rules[token_type];
}

void parsePresedence(Presedence presedence)
{
    advance();
    ParseRule *prev_table = get_rule(parser.previous.type);
    if(prev_table->prefix == NULL) {
        error("Syntax error");
        return;
    }
    prev_table->prefix();

    while(presedence <= get_rule(parser.current.type)->presedence)     {
        advance();
        ParseFn infix = get_rule(parser.previous.type)->infix;
        if(infix)
            infix();
    }
}

int compile(const char *source, Chunk *chunk)
{
    compiling_chunk = chunk;

    parser.is_error = 0;
    parser.is_panic = 0;

    init_scanner(source);
    advance();
    expression();
    consume(TOKEN_EOF, "Expect the end of expression");

    end_compiler();

    return parser.is_error;
}

// int compile(char *source, Chunk *chunk)
// {
//     setup_scanner(source);
//
//     for (;;)
//     {
//         Token token = scan_token();
//
//         if (line_number != token.line_number)
//         {
//             line_number = token.line_number;
//             printf("%02d ", token.line_number);
//         }
//         else
//         {
//             printf("|  ");
//         }
//
//         printf("%d '%.*s'\n", token.type, token.length, token.start);
//
//         if (token.type == TOKEN_ERROR)
//             break;
//
//         if (token.type == TOKEN_EOF)
//             break;
//     }
// }
