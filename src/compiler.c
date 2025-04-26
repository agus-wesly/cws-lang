#include "compiler.h"
#include "object.h"

extern int IS_IN_REPL;

int line_number = -1;

typedef struct
{
    Token previous;
    Token current;
    int is_error;
    int is_panic;
} Parser;

#define LOCAL_MAX_LENGTH UINT8_MAX + 1

typedef struct
{
    Token name;
    int depth;
} Local;

typedef struct
{
    Local locals[LOCAL_MAX_LENGTH];
    int count;
    int depth;
} Compiler;

static void expression();
static void parsePrecedence(Precedence presedence);
static ParseRule *get_rule(TokenType token_type);
static void declaration();
static void statement();
static uint32_t identifier_constant(const Token *token);
static int match(TokenType type);

Parser parser;
Chunk *compiling_chunk;
Compiler *current = NULL;

Chunk *current_chunk()
{
    return compiling_chunk;
}

static void error_at(Token token, char *message)
{
    if (parser.is_panic)
        return;

    parser.is_panic = 1;

    fprintf(stderr, "[line %d] Error : %s", token.line_number, message);
    if (token.type == TOKEN_EOF)
    {
        fprintf(stderr, " at the end");
    }
    else if (token.type == TOKEN_ERROR)
    {
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token.length, token.start);
    }
    fprintf(stderr, "\n");
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
        if (parser.current.type == TOKEN_COMMENT)
        {
            continue;
        }

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

    error(message);
}

static int peek(TokenType token_type)
{
    return parser.current.type == token_type;
}

void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line_number);
}

void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

void emit_constant_byte(uint32_t idx)
{
    for (size_t i = 0; i < 4; ++i)
    {
        uint8_t chunkIdx = (idx >> (8 * (3 - i)));
        emit_byte(chunkIdx);
    }
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
    write_constant(current_chunk(), value, parser.previous.line_number);
    // uint8_t idx = AddConstant(current_chunk(), value);
    // emit_bytes(OP_CONSTANT, idx);
}

static void nil(int can_assign)
{
    if (can_assign)
    {
    }
    emit_byte(OP_NIL);
}

static void boolean(int can_assign)
{
    if (can_assign)
    {
    }
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

int compare_token(Token *t1, Token *t2)
{
    if (t1->type != t2->type)
        return 0;

    return (memcmp(t1->start, t2->start, t1->length) == 0);
}

int find_local(Token token)
{
    for (int i = current->count - 1; i >= 0; --i)
    {
        Local *local = &current->locals[i];
        if (compare_token(&token, &local->name))
            return i;
    }

    return -1;
}

static void variable(int can_assign)
{
    uint8_t OP_GET, OP_SET;
    uint32_t identifier_idx;

    if (current->depth > 0)
    {
        OP_GET = OP_GET_LOCAL;
        OP_SET = OP_SET_LOCAL;
        int find_idx = find_local(parser.previous);
        if (find_idx < 0)
        {
            error("Unknown variable");
            identifier_idx = 0;
        }
        else
        {
            identifier_idx = find_idx;
        }
    }
    else
    {
        OP_GET = OP_GET_GLOBAL;
        OP_SET = OP_SET_GLOBAL;
        identifier_idx = identifier_constant(&parser.previous);
    }

    if (can_assign && match(TOKEN_EQUAL))
    {
        expression();
        emit_byte(OP_SET);
        emit_constant_byte(identifier_idx);
    }
    else
    {
        emit_byte(OP_GET);
        emit_constant_byte(identifier_idx);
    }
}

static void string(int can_assign)
{
    if (can_assign)
    {
    }
    write_constant(current_chunk(), VALUE_OBJ(copy_string(parser.previous.start + 1, parser.previous.length - 2)),
                   parser.previous.line_number);
}

static void unary(int can_assign)
{
    if (can_assign)
    {
    }
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

static void grouping(int can_assign)
{
    if (can_assign)
    {
    }
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

static void binary(int can_assign)
{
    if (can_assign)
    {
    }
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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},

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

    int can_parse_set = precedence <= PREC_ASSIGNMENT;
    prefix_rule(can_parse_set);

    while (precedence <= get_rule(parser.current.type)->precedence)
    {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        if (infix_rule == NULL)
        {
            error("Syntax error");
            return;
        }
        infix_rule(can_parse_set);
    }

    if (can_parse_set && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target");
    }
}

static int check(TokenType type)
{
    return parser.current.type == type;
}

static int match(TokenType type)
{
    if (parser.current.type == type)
    {
        advance();
        return 1;
    };

    return 0;
}

static void print_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after value");
    emit_byte(OP_PRINT);
}

static void begin_scope()
{
    current->depth++;
}

static void end_scope()
{
    current->depth--;

    for (int i = current->count - 1; i >= 0; --i)
    {
        Local local = current->locals[i];
        if (local.depth > current->depth)
        {
            emit_byte(OP_POP);
            current->count--;
        }
        else
        {
            break;
        }
    }
}

static void block_statement()
{
    begin_scope();
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expected '}' at the end of block");

    end_scope();
}

static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after value");
    if (IS_IN_REPL)
    {
        emit_byte(OP_PRINT);
    }
    else
    {
        emit_byte(OP_POP);
    }
}

static void statement()
{
    if (match(TOKEN_PRINT))
    {
        print_statement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        block_statement();
    }
    else
    {
        expression_statement();
    }
}

static void synchronize()
{
    parser.is_panic = 0;
    while (parser.current.type != TOKEN_EOF)
    {
        if (match(TOKEN_SEMICOLON))
            return;

        switch (parser.current.type)
        {
        case TOKEN_PRINT:
        case TOKEN_RIGHT_BRACE:
            return;

        default:;
        }

        advance();
    }
}

static uint32_t identifier_constant(const Token *token)
{
    ObjectString *string = copy_string(token->start, token->length);
    return add_long_constant(current_chunk(), VALUE_OBJ(string));
}

static void add_local(Token identifier)
{
    if (current->count == LOCAL_MAX_LENGTH)
        return;

    Local *local = &current->locals[current->count++];
    local->name = identifier;
    local->depth = current->depth;
}

static void define_variable(uint32_t identifier_idx)
{
    emit_byte(OP_GLOBAL_VAR);
    // TODO : Maybe we can check in compile time ??
    emit_constant_byte(identifier_idx);
}

static int parse_variable()
{
    consume(TOKEN_IDENTIFIER, "Expected variable name.");

    if (current->depth > 0)
    {
        add_local(parser.previous);
        return 0;
    }
    else
    {
        return identifier_constant(&parser.previous);
    }
}

static void var_declaration()
{
    uint32_t identifier_idx = parse_variable();

    if (peek(TOKEN_EQUAL))
    {
        advance();
        expression();
    }
    else
    {
        emit_byte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration");

    if (current->depth > 0)
    {
    }
    else
    {
        define_variable(identifier_idx);
    }
}

static void declaration()
{
    if (match(TOKEN_LET))
    {
        var_declaration();
    }
    else
    {
        statement();
    }
    if (parser.is_panic)
    {
        synchronize();
    }
}

void init_compiler(Compiler *compiler)
{
    compiler->count = 0;
    compiler->depth = 0;
    current = compiler;
}

int compile(const char *source, Chunk *chunk)
{
    compiling_chunk = chunk;

    parser.is_error = 0;
    parser.is_panic = 0;

    init_scanner(source);

    Compiler compiler;
    init_compiler(&compiler);

    advance();
    while (!match(TOKEN_EOF))
    {
        declaration();
    }

    end_compiler();

    return parser.is_error;
}
