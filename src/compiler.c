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

#define LOCAL_MAX_LENGTH 2056
#define LOOP_STACK_MAX_LENGTH 255

typedef struct
{
    Token name;
    int depth;
    int is_assignable;
} Local;

typedef struct
{
    // TODO : make this to be a hashmap
    Local locals[LOCAL_MAX_LENGTH];
    int count;
    int depth;
    int loop_count;
    int *loop_stack[LOOP_STACK_MAX_LENGTH];
} Compiler;

static void expression();
static void parse_precedence(Precedence presedence);
static ParseRule *get_rule(TokenType token_type);
static void declaration();
static void statement();
static uint32_t identifier_constant(const Token *token);
static int match(TokenType type);
static int check(TokenType type);
static void var_declaration(int is_assignable);

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

int emit_jump(uint8_t op)
{
    emit_byte(op);
    emit_byte(0xff);
    emit_byte(0xff);

    return current_chunk()->count - 2;
}

void patch_jump(int jump_idx)
{
    int jump = current_chunk()->count - jump_idx - 2;
    if (jump > UINT16_MAX)
    {
        error("Too many jump statement");
    }

    current_chunk()->code[jump_idx] = (jump >> 8) & 0xff;
    current_chunk()->code[jump_idx + 1] = jump & 0xff;
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

    if (t1->length != t2->length)
        return 0;

    return (memcmp(t1->start, t2->start, t1->length) == 0);
}

int find_local(Token token)
{
    for (int i = current->count - 1; i >= 0; --i)
    {
        Local *local = &current->locals[i];
        if (compare_token(&token, &local->name))
        {
            if (local->depth == -1)
            {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

int is_local_assignable(uint32_t identifier_idx)
{
    Local *local = &current->locals[identifier_idx];
    return local->is_assignable;
}

static void variable(int can_assign)
{
    uint8_t OP_GET, OP_SET;
    uint32_t identifier_idx;

    int find_idx = find_local(parser.previous);

    if (current->depth == 0 || find_idx == -1)
    {
        OP_GET = OP_GET_GLOBAL;
        OP_SET = OP_SET_GLOBAL;
        identifier_idx = identifier_constant(&parser.previous);
    }
    else
    {
        OP_GET = OP_GET_LOCAL;
        OP_SET = OP_SET_LOCAL;
        identifier_idx = find_idx;
    }

    if (can_assign && check(TOKEN_EQUAL))
    {
        // TODO : handle case when trying to mutate global const var
        if (OP_SET == OP_SET_LOCAL && !is_local_assignable(identifier_idx))
        {
            error("Cannot assign to const variable");
        }
        advance();

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
    parse_precedence(get_rule(token_type)->precedence + 1);

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

    parse_precedence(presedence + 1);

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

static void and_(int can_assign)
{
    if (can_assign)
    {
    }

    int jump = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    parse_precedence(PREC_AND);

    patch_jump(jump);
}

static void or_(int can_assign)
{
    if (can_assign)
    {
    }

    int jump = emit_jump(OP_JUMP_IF_TRUE);
    emit_byte(OP_POP);
    parse_precedence(PREC_OR);

    patch_jump(jump);
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
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
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
    parse_precedence(PREC_ASSIGNMENT);
}

static ParseRule *get_rule(TokenType token_type)
{
    return &rules[token_type];
}

static void parse_precedence(Precedence precedence)
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

static void declare_local(Token identifier, int is_assignable)
{
    if (current->count == LOCAL_MAX_LENGTH)
    {
        error("Already reach the local variable limit");
        return;
    }

    Local *local = &current->locals[current->count++];
    local->name = identifier;
    local->depth = -1;
    local->is_assignable = is_assignable;
}

static void define_local()
{
    Local *local = &current->locals[current->count - 1];
    local->depth = current->depth;
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

static void begin_loop(int *offset)
{
    // Push to stack
    assert(current->loop_count <= LOOP_STACK_MAX_LENGTH && "Already reach max length of loop stack");

    current->loop_stack[current->loop_count++] = offset;
}

static int *peek_loop()
{
    assert(current->loop_count > 0 && "Cannot peek loop stack if empty");
    return current->loop_stack[current->loop_count - 1];
}

static void end_loop()
{
    // Pop the stack
    assert(current->loop_count > 0 && "Cannot pop loop stack if empty");
    current->loop_count--;
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

static void if_statement()
{
    consume(TOKEN_LEFT_PAREN, "Expected '(' before expression");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' before expression");

    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();

    int else_jump = emit_jump(OP_JUMP);
    patch_jump(then_jump);

    emit_byte(OP_POP);
    if (match(TOKEN_ELSE))
    {
        statement();
    }
    patch_jump(else_jump);
}

static void emit_loop(int offset)
{
    emit_byte(OP_LOOP);
    int back_jump = current_chunk()->count - offset + 2;
    if (back_jump > UINT16_MAX)
    {
        error("To many jump statement");
    }

    emit_byte((back_jump >> 8) & 0xff);
    emit_byte(back_jump & 0xff);
}

static void continue_statement()
{
    if (!current->loop_count)
    {
        error("'continue' statement is not inside loop statement");
    }

    // We need offset to be able to move backward
    int *offset = peek_loop();
    emit_loop(*offset);

    consume(TOKEN_SEMICOLON, "Expected ';' after continue");
}

static void while_statement()
{
    int offset = current_chunk()->count;
    begin_loop(&offset);

    consume(TOKEN_LEFT_PAREN, "Expected '(' before expression");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' before expression");

    int then_jump = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    statement();
    emit_loop(offset);

    patch_jump(then_jump);
    emit_byte(OP_POP);
    end_loop();
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

static void for_statement()
{
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "Expected '(' after for");

    if (match(TOKEN_SEMICOLON))
    {
    }
    else if (match(TOKEN_LET))
    {
        var_declaration(1);
    }
    else
    {
        expression_statement();
    }

    int offset = current_chunk()->count;
    begin_loop(&offset);

    int then_jump = -1;
    if (!match(TOKEN_SEMICOLON))
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after expression");

        then_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
    }

    if (!check(TOKEN_RIGHT_PAREN))
    {
        int condition_jump = emit_jump(OP_JUMP);
        int inc_offset = current_chunk()->count;

        expression();
        emit_byte(OP_POP);

        emit_loop(offset);
        offset = inc_offset;

        patch_jump(condition_jump);
    }

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after for");

    statement();
    emit_loop(offset);

    if (then_jump != -1)
    {
        patch_jump(then_jump);
        emit_byte(OP_POP);
    }

    end_scope();
    end_loop();
}

static void default_statement()
{
    consume(TOKEN_COLON, "Expected ':' after default");
    while (!check(TOKEN_RIGHT_BRACE))
    {
        statement();
    }
}

static void case_statement(int jump_idx)
{
    consume(TOKEN_CASE, "Expected 'case' inside switch");
    expression();

    emit_byte(OP_COMPARE);

    int jump_false = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    emit_byte(OP_POP);

    consume(TOKEN_COLON, "Expected ':' after expression");
    while (!check(TOKEN_CASE) && !check(TOKEN_RIGHT_BRACE) && !check(TOKEN_DEFAULT))
    {
        statement();
    }

    int curr_length = current_chunk()->count - (jump_idx - 1);

    emit_byte(OP_SWITCH_JUMP);
    emit_byte(jump_idx);
    emit_byte(curr_length);

    patch_jump(jump_false);

    emit_byte(OP_POP);
    emit_byte(OP_POP);
}

static void switch_statement()
{
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "Expected '(' after switch");

    /* Workaround because we need to match the stack */
    Token switch_identifier = {.start = "switch", .length = 6, .type = TOKEN_IDENTIFIER};
    declare_local(switch_identifier, 0);
    expression();
    define_local();

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression in switch");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before switch body");

    int jump_idx = emit_jump(OP_MARK);
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_DEFAULT))
    {
        case_statement(jump_idx);
    }
    if (match(TOKEN_DEFAULT))
    {
        default_statement();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after switch body");
    patch_jump(jump_idx);
    end_scope();
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
    else if (match(TOKEN_IF))
    {
        if_statement();
    }
    else if (match(TOKEN_WHILE))
    {
        while_statement();
    }
    else if (match(TOKEN_FOR))
    {
        for_statement();
    }
    else if (match(TOKEN_SWITCH))
    {
        switch_statement();
    }
    else if (match(TOKEN_CONTINUE))
    {
        continue_statement();
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
        case TOKEN_LEFT_BRACE:
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

static void define_variable(uint32_t identifier_idx)
{
    emit_byte(OP_GLOBAL_VAR);
    emit_constant_byte(identifier_idx);
}

static int parse_variable(int is_assignable)
{
    consume(TOKEN_IDENTIFIER, "Expected variable name.");

    if (current->depth > 0)
    {
        for (int i = current->count - 1; i >= 0; --i)
        {
            Local *local = &current->locals[i];
            if (local->depth == -1 || local->depth < current->depth)
            {
                break;
            }

            if (compare_token(&parser.previous, &local->name))
            {
                error("Redeclaration of variable");
                return 0;
            }
        }

        declare_local(parser.previous, is_assignable);
        return 0;
    }
    else
    {
        return identifier_constant(&parser.previous);
    }
}

static void var_declaration(int is_assignable)
{
    uint32_t identifier_idx = parse_variable(is_assignable);

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
        define_local();
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
        var_declaration(1);
    }
    else if (match(TOKEN_CONST))
    {
        var_declaration(0);
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
    compiler->loop_count = 0;

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
