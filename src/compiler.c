#include "compiler.h"
#include "object.h"
#include "vm.h"

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
#define LOOP_STACK_MAX_LENGTH 2056
#define JUMP_STACK_MAX_LENGTH 2056

typedef struct
{
    Token name;
    int depth;
    int is_assignable;
    bool is_captured;
} Local;

typedef struct Compiler
{
    // TODO : make this to be a hashmap
    Local locals[LOCAL_MAX_LENGTH];
    int count;
    int depth;

    /*continue statement*/
    int loop_count;
    Loop loop_stack[LOOP_STACK_MAX_LENGTH];

    /*break statement*/
    int jump_count;
    Jump jump_stack[JUMP_STACK_MAX_LENGTH];

    FunctionType type;
    ObjectFunction *function;

    struct Compiler *enclosing;

    int upvalue_count;
    UpValue upvalue[UPVALUE_MAX];

} Compiler;

typedef struct ClassCompiler
{
    struct ClassCompiler *enclosing;

} ClassCompiler;

static void expression();
static void parse_precedence(Precedence presedence);
static ParseRule *get_rule(TokenType token_type);
static void declaration();
static void statement();
static uint32_t identifier_constant(const Token *token);
static bool match(TokenType type);
static bool check(TokenType type);
static void var_declaration(int is_assignable);

Parser parser;
Chunk *compiling_chunk;
ClassCompiler *current_class = NULL;
Compiler *current = NULL;

void init_compiler(Compiler *compiler, FunctionType type)
{
    compiler->count = 0;
    compiler->depth = 0;
    compiler->loop_count = 0;
    compiler->jump_count = 0;
    compiler->upvalue_count = 0;

    compiler->function = new_function();
    compiler->type = type;

    compiler->enclosing = current;

    current = compiler;

    if (type != TYPE_SCRIPT)
    {
        compiler->function->name = copy_string(parser.previous.start, parser.previous.length);
    }

    Local *local = &current->locals[current->count++];
    local->depth = 0;
    local->is_assignable = 0;
    if (type == TYPE_METHOD || type == TYPE_INIT)
    {
        local->name.start = "this";
        local->name.length = 4;
        local->name.type = TOKEN_THIS;
    }
    else
    {
        local->name.start = "";
        local->name.length = 0;
    }
}

Chunk *current_chunk()
{
    return &current->function->chunk;
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

void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line_number);
}

void emit_int(int d)
{
    for (size_t i = 0; i < 4; ++i)
    {
        uint8_t byte = (d >> (8 * (3 - i)));
        emit_byte(byte);
    }
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
    if (current->type == TYPE_INIT)
    {
        emit_byte(OP_GET_LOCAL);
        emit_constant_byte(0);
    }
    else
    {
        emit_byte(OP_NIL);
    }
    emit_byte(OP_RETURN);
}

ObjectFunction *end_compiler()
{
    emit_return();

    ObjectFunction *function = current->function;
    function->upvalue_count = current->upvalue_count;

#ifdef DEBUG_PRINT
    if (!parser.is_error)
    {
        disassemble_chunk(current_chunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;

    if (parser.is_error)
        return NULL;

    return function;
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

static uint8_t parse_args()
{
    uint8_t arity = 0;
    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            expression();
            arity += 1;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expected closing parentheses ')'");
    return arity;
}

static void dot(int can_assign)
{
    consume(TOKEN_IDENTIFIER, "Expected identifer");

    uint32_t name_attr = identifier_constant(&parser.previous);

    if (match(TOKEN_LEFT_PAREN))
    {
        uint8_t arity = parse_args();

        emit_bytes(OP_INVOKE, arity);
        emit_constant_byte(name_attr);
    }
    else if (can_assign && match(TOKEN_EQUAL))
    {
        expression();
        emit_byte(OP_DOT_SET);
        emit_constant_byte(name_attr);
    }
    else
    {
        emit_byte(OP_DOT_GET);
        emit_constant_byte(name_attr);
    }
}

static void sqrbracket(int can_assign)
{
    expression();
    consume(TOKEN_RIGHT_SQR_BRACKET, "Expecting closing ']'");

    if (can_assign && match(TOKEN_EQUAL))
    {
        expression();
        // container, key, new
        // key, new, container
        emit_byte(OP_SQR_BRACKET_SET);
    }
    else
    {
        emit_byte(OP_SQR_BRACKET_GET);
    }
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

bool compare_token(Token *t1, Token *t2)
{
    if (t1->type != t2->type)
        return false;

    if (t1->length != t2->length)
        return false;

    return (memcmp(t1->start, t2->start, t1->length) == 0);
}

int find_local(Compiler *current, Token token)
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

int resolve_upvalue(Compiler *current, Token token)
{
    if (current->enclosing == NULL)
        return -1;

    int res = find_local(current->enclosing, token);

    if (res >= 0)
    {
        current->enclosing->locals[res].is_captured = true;

        current->upvalue[current->upvalue_count].index = res;
        current->upvalue[current->upvalue_count].is_local = true;
        return current->upvalue_count++;
    }

    res = resolve_upvalue(current->enclosing, token);
    if (res >= 0)
    {
        current->upvalue[current->upvalue_count].index = res;
        current->upvalue[current->upvalue_count].is_local = false;
        return current->upvalue_count++;
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

    int find_idx = find_local(current, parser.previous);

    if (current->depth > 0 && find_idx >= 0)
    {
        OP_GET = OP_GET_LOCAL;
        OP_SET = OP_SET_LOCAL;
        identifier_idx = find_idx;
    }
    else if ((find_idx = resolve_upvalue(current, parser.previous)) >= 0)
    {
        OP_GET = OP_GET_UPVALUE;
        OP_SET = OP_SET_UPVALUE;
        identifier_idx = find_idx;
    }
    else
    {
        OP_GET = OP_GET_GLOBAL;
        OP_SET = OP_SET_GLOBAL;
        identifier_idx = identifier_constant(&parser.previous);
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

static void _this()
{
    if (current_class == NULL)
    {
        error("'this' expression can only appear inside of class");
        return;
    };
    variable(false);
}

static void string(int can_assign)
{
    if (can_assign)
    {
    }
    emit_constant(current_chunk(), VALUE_OBJ(copy_string(parser.previous.start + 1, parser.previous.length - 2)),
                  parser.previous.line_number);
}

static void _number(int can_assign)
{
    if (can_assign)
    {
    }
    Value value = VALUE_NUMBER(strtod(parser.previous.start, NULL));
    emit_constant(current_chunk(), value, parser.previous.line_number);
    // uint8_t idx = AddConstant(current_chunk(), value);
    // emit_bytes(OP_CONSTANT, idx);
}

static void table(int can_assign)
{
    if (can_assign)
    {
    }

    uint32_t table_count = 0;
    while (!check(TOKEN_RIGHT_BRACE))
    {
        if (!check(TOKEN_STRING))
        {
            advance();
            error("Key must be a type string and wrapped inside quotes");
            return;
        }

        expression();
        consume(TOKEN_COLON, "Expected colon ':' after key");
        expression();
        ++table_count;

        if (check(TOKEN_COMMA))
            advance();
        else
            break;
    }
    consume(TOKEN_RIGHT_BRACE, "Expected closing parentheses '}'");

    emit_byte(OP_INIT_TABLE);
    emit_constant_byte(table_count);
}

static void array(int is_assignable)
{
    if (is_assignable)
    {
    }

    uint32_t array_count = 0;
    while (!check(TOKEN_RIGHT_SQR_BRACKET))
    {
        expression();
        ++array_count;
        
        if (!match(TOKEN_COMMA))
            break;
    }
    consume(TOKEN_RIGHT_SQR_BRACKET, "Expected closing ']' in array declaration");

    emit_byte(OP_INIT_ARRAY);
    emit_constant_byte(array_count);
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

static void del_statement()
{
    /*
     * Currently `del` just support notation
     * In the future we can support the sqr_bracket notation
     */
    consume(TOKEN_IDENTIFIER, "Expected identifier after 'del'");
    variable(0);
    consume(TOKEN_DOT, "Expected first identifier");
    do
    {
        consume(TOKEN_IDENTIFIER, "Expected identifier");
        uint32_t name_attr = identifier_constant(&parser.previous);

        if (check(TOKEN_DOT))
        {
            emit_byte(OP_DOT_GET);
            emit_constant_byte(name_attr);
        }
        else
        {
            emit_byte(OP_CONSTANT_LONG);
            emit_constant_byte(name_attr);
        }

    } while (match(TOKEN_DOT));

    emit_byte(OP_DEL);
    consume(TOKEN_SEMICOLON, "Expected ';' after identifier");
}

static void grouping(int can_assign)
{
    if (can_assign)
    {
    }
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected closing parentheses");
}

static void call(int can_assign)
{
    if (can_assign)
    {
    }
    uint8_t arity = parse_args();
    emit_bytes(OP_CALL, arity);
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
        emit_bytes(OP_LESS, OP_BANG);
        break;
    };
    case TOKEN_LESS: {
        emit_byte(OP_LESS);
        break;
    };
    case TOKEN_LESS_EQUAL: {
        emit_bytes(OP_GREATER, OP_BANG);
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
    [TOKEN_NUMBER] = {_number, NULL, PREC_PRIMARY},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_THIS] = {_this, NULL, PREC_NONE},

    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {table, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_SQR_BRACKET] = {array, sqrbracket, PREC_CALL},
    [TOKEN_RIGHT_SQR_BRACKET] = {NULL, NULL, PREC_NONE},

    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_QUESTION_MARK] = {NULL, ternary, PREC_TERNARY},

    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_DOT] = {NULL, dot, PREC_CALL},

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

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (parser.current.type == type)
    {
        advance();
        return 1;
    };

    return false;
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
    local->is_captured = false;
}

static void declare(bool is_assignable)
{
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
                return;
            }
        }

        declare_local(parser.previous, is_assignable);
        return;
    }
}

static void define_local()
{
    if (current->depth == 0)
        return;

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

static void begin_loop(int offset, int depth)
{
    // Push to stack
    assert(current->loop_count <= LOOP_STACK_MAX_LENGTH && "Already reach max length of loop stack");

    Loop *loop = &current->loop_stack[current->loop_count++];
    loop->offset = offset;
    loop->depth = depth;
}

static Loop *peek_loop()
{
    assert(current->loop_count > 0 && "Cannot peek loop stack if empty");
    return &current->loop_stack[current->loop_count - 1];
}

static void end_loop()
{
    // Pop the stack
    assert(current->loop_count > 0 && "Cannot pop loop stack if empty");
    current->loop_count--;
}

static void begin_jump(int offset, int depth)
{
    // Push to stack
    assert(current->jump_count <= JUMP_STACK_MAX_LENGTH && "Already reach max length of jump stack");

    Jump *jump = &current->jump_stack[current->jump_count++];
    jump->idx = offset;
    jump->depth = depth;
}

static Jump *peek_jump()
{
    assert(current->jump_count > 0 && "Cannot peek jump stack if empty");
    return &current->jump_stack[current->jump_count - 1];
}

static void end_jump()
{
    // Pop the stack
    assert(current->jump_count > 0 && "Cannot pop jump stack if empty");
    current->jump_count--;
}

static void end_scope()
{
    current->depth--;

    for (int i = current->count - 1; i >= 0; --i)
    {
        Local local = current->locals[i];
        if (local.depth > current->depth)
        {
            if (local.is_captured)
            {
                emit_byte(OP_CLOSE_UPVALUE);
            }
            else
            {
                emit_byte(OP_POP);
            }

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
        return;
    }

    Loop *current_loop = peek_loop();

    for (int i = current->count - 1; i >= 0; --i)
    {
        Local local = current->locals[i];
        if (local.depth > current_loop->depth)
        {
            emit_byte(OP_POP);
        }
        else
        {
            break;
        }
    }

    // We need offset to be able to move backward
    int offset = peek_loop()->offset;
    emit_loop(offset);

    consume(TOKEN_SEMICOLON, "Expected ';' after continue");
}

static void break_statement()
{
    if (!current->loop_count && !current->jump_count)
    {
        error("'break' statement is not inside loop statement");
        return;
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after break");

    Jump *current_jump = peek_jump();

    for (int i = current->count - 1; i >= 0; --i)
    {
        Local local = current->locals[i];
        if (local.depth > current_jump->depth)
        {
            emit_byte(OP_POP);
        }
        else
        {
            break;
        }
    }

    int jump_idx = current_jump->idx;
    int offset = current_chunk()->count - (jump_idx - 1);

    emit_byte(OP_SWITCH_JUMP);
    emit_byte(jump_idx);
    emit_byte(offset);
}

static void begin_while(int *while_jump, int *offset)
{
    *while_jump = emit_jump(OP_MARK_JUMP);
    begin_jump(*while_jump, current->depth);

    *offset = current_chunk()->count;
    begin_loop(*offset, current->depth);
}

static void end_while(int while_jump)
{
    emit_byte(OP_POP);
    patch_jump(while_jump);
    end_jump();
    end_loop();
}

static void while_statement()
{
    int while_jump, offset;
    begin_while(&while_jump, &offset);

    consume(TOKEN_LEFT_PAREN, "Expected '(' before expression");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' before expression");

    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    emit_loop(offset);
    patch_jump(then_jump);

    end_while(while_jump);
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
        // this
        emit_byte(OP_POP);
    }
}

static void begin_for(int *for_jump)
{
    begin_scope();
    *for_jump = emit_jump(OP_MARK_JUMP);
    begin_jump(*for_jump, current->depth);
}

static void end_for(int for_jump, int then_jump)
{
    if (then_jump != -1)
    {
        patch_jump(then_jump);
        emit_byte(OP_POP);
    }

    patch_jump(for_jump);

    end_jump();
    end_loop();
    end_scope();
}

static void for_statement()
{
    int for_jump;
    int then_jump = -1;
    begin_for(&for_jump);

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

    begin_loop(offset, current->depth);
    statement();
    emit_loop(offset);

    end_for(for_jump, then_jump);
}

static void default_statement()
{
    consume(TOKEN_COLON, "Expected ':' after default");
    while (!check(TOKEN_RIGHT_BRACE))
    {
        statement();
    }
}

static void case_statement()
{
    consume(TOKEN_CASE, "Expected 'case' inside switch");

    int case_jump = emit_jump(OP_JUMP_IF_TRUE);
    expression();
    emit_byte(OP_CASE_COMPARE);
    patch_jump(case_jump);

    int jump_false = emit_jump(OP_JUMP_IF_FALSE);

    consume(TOKEN_COLON, "Expected ':' after expression");
    while (!check(TOKEN_CASE) && !check(TOKEN_RIGHT_BRACE) && !check(TOKEN_DEFAULT))
    {
        statement();
    }

    patch_jump(jump_false);
}

static void emit_switch()
{
    /* We need this to match what inside the stack
     * One for the expression in switch
     * And one for the boolean
     * */
    Token switch_identifier = {.start = "switch", .length = 6, .type = TOKEN_IDENTIFIER};
    declare_local(switch_identifier, 0);
    define_local();
    declare_local(switch_identifier, 0);
    define_local();

    emit_byte(OP_SWITCH);
}

static int begin_switch()
{
    // current->jump_count++;
    int switch_jump = emit_jump(OP_MARK_JUMP);

    begin_scope();
    begin_jump(switch_jump, current->depth);

    return switch_jump;
}

static void end_switch(int switch_jump)
{
    // current->jump_count--;
    patch_jump(switch_jump);

    end_jump();
    end_scope();
}

static void switch_statement()
{
    int switch_jump = begin_switch();
    consume(TOKEN_LEFT_PAREN, "Expected '(' after switch");

    expression();

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression in switch");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before switch body");

    emit_switch();

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_DEFAULT))
    {
        case_statement();
    }
    if (match(TOKEN_DEFAULT))
    {
        default_statement();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after switch body");
    end_switch(switch_jump);
}

static void return_statement()
{
    if (!check(TOKEN_SEMICOLON))
    {
        expression();
    }
    else
    {
        emit_byte(OP_NIL);
    }

    if (current->type == TYPE_INIT)
    {
        emit_byte(OP_POP);
        emit_byte(OP_GET_LOCAL);
        emit_constant_byte(0);
    }

    emit_byte(OP_RETURN);

    consume(TOKEN_SEMICOLON, "Expected ';' after statement");
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
    else if (match(TOKEN_RETURN))
    {
        return return_statement();
    }
    else if (match(TOKEN_CONTINUE))
    {
        continue_statement();
    }
    else if (match(TOKEN_BREAK))
    {
        break_statement();
    }
    else if (match(TOKEN_DEL))
    {
        del_statement();
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
    push(VALUE_OBJ(string));
    uint32_t res = add_long_constant(current_chunk(), VALUE_OBJ(string));
    pop();
    return res;
}

static void define_variable(uint32_t identifier_idx)
{
    if (current->depth > 0)
    {
        Local *local = &current->locals[current->count - 1];
        local->depth = current->depth;
    }
    else
    {
        emit_byte(OP_GLOBAL_VAR);
        emit_constant_byte(identifier_idx);
    }
}

static int parse_variable(int is_assignable)
{
    consume(TOKEN_IDENTIFIER, "Expected variable name");

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

    if (check(TOKEN_EQUAL))
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

static void function(FunctionType type)
{
    define_local();

    Compiler compiler;
    init_compiler(&compiler, type);

    begin_scope();

    consume(TOKEN_LEFT_PAREN, "Expected '(' after function name");

    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            ++current->function->arity;
            uint32_t constant = parse_variable(1);
            define_variable(constant);
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after function parameter");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before function body");

    block_statement();

    // Here
    ObjectFunction *function = end_compiler();

    push(VALUE_OBJ(function));
    emit_byte(OP_CLOSURE);
    make_constant(current_chunk(), VALUE_OBJ(function), parser.previous.line_number);
    pop();

    for (int i = 0; i < compiler.upvalue_count; ++i)
    {
        emit_byte(compiler.upvalue[i].is_local);
        emit_int(compiler.upvalue[i].index);
    }
}

static void function_declaration()
{
    uint32_t identifier_idx = parse_variable(false);
    function(TYPE_FUNCTION);
    define_variable(identifier_idx);
}

static void class_declaration()
{
    consume(TOKEN_IDENTIFIER, "Expected variable name.");
    int klass_name = identifier_constant(&parser.previous);
    declare(false);

    emit_byte(OP_CLASS);
    emit_constant_byte(klass_name);

    define_variable(klass_name);

    variable(false);

    ClassCompiler c;
    c.enclosing = current_class;
    current_class = &c;

    consume(TOKEN_LEFT_BRACE, "Expected '{' after class name");
    begin_scope();

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        consume(TOKEN_IDENTIFIER, "Expected identifier");

        uint32_t name_method = identifier_constant(&parser.previous);
        // Check if this is constructor
        FunctionType func_type = TYPE_METHOD;
        if (memcmp(vm.init_string->chars, parser.previous.start, vm.init_string->length) == 0)
        {
            func_type = TYPE_INIT;
        }

        function(func_type);
        define_variable(name_method);

        emit_byte(OP_METHOD);
        emit_constant_byte(name_method);
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after class declaration");
    emit_byte(OP_POP);

    current->depth--;

    current_class = c.enclosing;
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
    else if (match(TOKEN_FUN))
    {
        function_declaration();
    }
    else if (match(TOKEN_CLASS))
    {
        class_declaration();
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

ObjectFunction *compile(const char *source)
{

    parser.is_error = 0;
    parser.is_panic = 0;

    init_scanner(source);

    Compiler compiler;
    init_compiler(&compiler, TYPE_SCRIPT);

    advance();
    while (!match(TOKEN_EOF))
    {
        declaration();
    }

    return end_compiler();
}

void mark_compiler()
{
    Compiler *c = current;
    while (c != NULL)
    {
        mark_obj((Obj *)c->function);
        c = c->enclosing;
    }
}

//'0b0111111111111100000000000000000000000000000000000000000000000000'
