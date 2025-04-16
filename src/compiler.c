#include "compiler.h"
#include "object.h"

int line_number = -1;

typedef struct
{
    Token previous;
    Token current;
    int is_error;
    int is_panic;
} Parser;

static void expression();
static void parsePrecedence(Precedence presedence);
static ParseRule *get_rule(TokenType token_type);

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
#ifdef DEBUG_PRINT
    if (!parser.is_error)
    {
        DisassembleChunk(current_chunk(), "Inside of the chunk");
    }
#endif
}

/*====== PREFIX & INFIX ====== */

Precedence _get_presedence(TokenType token_type)
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
    Value value = VALUE_NUMBER(strtod(parser.previous.start, NULL));
    WriteConstant(current_chunk(), value, parser.previous.line_number);
    // uint8_t idx = AddConstant(current_chunk(), value);
    // emit_bytes(OP_CONSTANT, idx);
}

static void nil()
{
    emit_byte(OP_NIL);
}

static void boolean()
{
    switch (parser.previous.type)
    {
    case TOKEN_TRUE: {
        emit_byte(OP_TRUE);
        break;
    }
    case TOKEN_FALSE: {
        emit_byte(OP_FALSE);
        break;
    }
    default:
        assert(0 && "Unreachable at boolean");
        return;
    }
}


static void string()
{
    WriteConstant(current_chunk(), VALUE_OBJ(copy_string(parser.previous.start + 1, parser.previous.length - 2)),
                  parser.previous.line_number);
}

static void unary()
{
    TokenType token_type = parser.previous.type;
    parsePrecedence(get_rule(token_type)->precedence + 1);

    switch (token_type)
    {
    case TOKEN_MINUS: {
        emit_byte(OP_NEGATE);
        break;
    };
    case TOKEN_BANG: {
        emit_byte(OP_BANG);
        break;
    };
    default: {
        assert(0 && "Unreachable at unary");
        break;
    }; // Unreachable
    }
}

static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected closing parentheses");
}

static void ternary()
{
    expression();
    consume(TOKEN_COLON, "Expected colon inside ternary operator");
    expression();
    emit_byte(OP_TERNARY);
}

static void binary()
{
    TokenType token_type = parser.previous.type;
    Precedence presedence = get_rule(token_type)->precedence;
    parsePrecedence(presedence + 1);

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
    case TOKEN_GREATER: {
        emit_byte(OP_GREATER);
        break;
    };
    case TOKEN_GREATER_EQUAL: {
        emit_bytes(OP_LESS, OP_NEGATE);
        break;
    };
    case TOKEN_LESS: {
        emit_byte(OP_LESS);
        break;
    };
    case TOKEN_LESS_EQUAL: {
        emit_bytes(OP_GREATER, OP_NEGATE);
        break;
    };
    case TOKEN_EQUAL_EQUAL: {
        emit_byte(OP_EQUAL_EQUAL);
        break;
    };
    default: {
        break;
    };
    }
}
/*===================== */

ParseRule rules[] = {
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},

    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LET] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_BREAK] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {nil, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {boolean, NULL, PREC_NONE},
    [TOKEN_FALSE] = {boolean, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},

    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},

    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_QUESTION_MARK] = {NULL, ternary, PREC_TERNARY},

    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_NUMBER] = {number, NULL, PREC_PRIMARY},

    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

static ParseRule *get_rule(TokenType token_type)
{
    return &rules[token_type];
}

static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL)
    {
        error("Syntax error");
        return;
    }

    prefix_rule();

    while (precedence <= get_rule(parser.current.type)->precedence)
    {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
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
