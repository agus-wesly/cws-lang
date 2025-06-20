#ifndef CWS_TOKEN_H
#define CWS_TOKEN_H
typedef enum
{
    /* Single Character */
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_SQR_BRACKET,
    TOKEN_RIGHT_SQR_BRACKET,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_COLON,
    TOKEN_QUESTION_MARK,
    TOKEN_BACKTICK,

    /* One or two character */
    TOKEN_BANG,
	TOKEN_BANG_EQUAL,
	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER,
	TOKEN_GREATER_EQUAL,
	TOKEN_LESS,
	TOKEN_LESS_EQUAL,
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS_EQUAL,

    /* Literal */
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_NUMBER,
    TOKEN_CHAR,

    /* Keywords */
	TOKEN_AND,
	TOKEN_CLASS,
	TOKEN_ELSE,
	TOKEN_FALSE,
	TOKEN_FUN,
	TOKEN_FOR,
	TOKEN_IF,
	TOKEN_SWITCH,
    TOKEN_CASE,
	TOKEN_NIL,
	TOKEN_OR,
	TOKEN_PRINT,
	TOKEN_RETURN,
	TOKEN_SUPER,
	TOKEN_THIS,
	TOKEN_TRUE,
	TOKEN_LET,
	TOKEN_CONST,
	TOKEN_WHILE,
	TOKEN_BREAK,
    TOKEN_CONTINUE,
	TOKEN_DEFAULT,
	TOKEN_DEL,
	TOKEN_LEN,
	TOKEN_EOF,

    TOKEN_COMMENT,
	TOKEN_ERROR,

} TokenType;

typedef struct
{
    const char *start;
    int line_number;
    int length;
    TokenType type;
} Token;


#endif // !CWS_TOKEN_H
