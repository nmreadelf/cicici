#include <iostream>
#include <assert.h>
#include <ctype.h>
#include <fmt/core.h>

#include <cstdlib>
#include <cstdarg>


//
// Tokenizer
//

typedef enum {
    TK_PUNCT,   // Punctuators,
    TK_NUM,     // Numeric literals
    TK_EOF,     // End-of-file markers
} TokenKind;

// Token Type
typedef struct Token Token;

struct Token {
    TokenKind kind;     // Token kind
    Token *next;        // Next Token
    int val;            // If kind is TK_NUM, its value
    char *loc;          // Token location
    int len;            // Token length

    Token(TokenKind p_kind, char *start, int p_len) : kind(p_kind), loc(start), len(p_len) {}

    Token() {}
};

// Input string
static char *current_input;

// Reports an error and exit.
static void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

// Reports an error location and exit.
static void verror_at(const char *loc, const char *fmt, va_list ap) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static void error_at(const char *loc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

// Consumes the current token if it matches `s`.
static bool equal(Token *tok, const char *op) {
    return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `s`,
static Token *skip(Token *tok, const char *s) {
    if (!equal(tok, s)) {
        error_tok(tok, "expected '%s'", s);
    }
    return tok->next;
}

// Ensure that the current token is TK_NUM.
static int get_number(Token *tok) {
    if (tok->kind != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return tok->val;
}

static bool startswitch(const char *p, const char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

// Read a punctuator token from p and returns its length.
static int read_punct(const char *p) {
    if (startswitch(p, "==") || startswitch(p, "!=") ||
        startswitch(p, "<=") || startswitch(p, ">=")) {
        return 2;
    }

    return ispunct(*p) ? 1 : 0;
}

// Tokenize `current_input` and returns new tokens.
static Token *tokenize(void) {
    char *p = current_input;
    Token head = {};
    Token *cur = &head;

    while (*p) {
        // Skip whitespace characters.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Numeric literal
        if (isdigit(*p)) {
            cur = cur->next = new Token(TK_NUM, p, 0);
            char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        // Punctuators
        int punct_len = read_punct(p);
        if (punct_len) {
            cur = cur->next = new Token(TK_PUNCT, p, punct_len);
            p += cur->len;
            continue;
        }

        error_at(p, "invalid token");
    }

    cur = cur->next = new Token(TK_EOF, p, 0);
    return head.next;
}

//
// Parser
//

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NEG, // unary -
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;

struct Node {
    NodeKind kind;  // Node kind
    Node *lhs;      // Left-hand side
    Node *rhs;      // Right-hand side
    int val;        // Used if kind == ND_NUM

    Node(NodeKind p_kind, int p_val) : kind(p_kind), lhs(nullptr), rhs(nullptr), val(p_val) {}

    Node(NodeKind p_kind, Node *p_lhs, Node *p_rhs) : kind(p_kind), lhs(p_lhs), rhs(p_rhs), val(0) {}
};

static Node *expr(Token **rest, Token *tok);

static Node *equality(Token **rest, Token *tok);

static Node *relational(Token **rest, Token *tok);

static Node *add(Token **rest, Token *tok);

static Node *mul(Token **rest, Token *tok);

static Node *unary(Token **rest, Token *tok);

static Node *primary(Token **rest, Token *tok);

// expr = equality
static Node *expr(Token **rest, Token *tok) {
    return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            node = new Node(ND_EQ, node, relational(&tok, tok->next));
            continue;
        }
        if (equal(tok, "!=")) {
            node = new Node(ND_NE, node, relational(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);

    for (;;) {
        if (equal(tok, "<")) {
            node = new Node(ND_LT, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, "<=")) {
            node = new Node(ND_LE, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, ">")) {
            node = new Node(ND_LT, add(&tok, tok->next), node);
            continue;
        }
        if (equal(tok, ">=")) {
            node = new Node(ND_LE, add(&tok, tok->next), node);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);

    for (;;) {
        if (equal(tok, "+")) {
            node = new Node(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }
        if (equal(tok, "-")) {
            node = new Node(ND_SUB, node, mul(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);

    for (;;) {
        if (equal(tok, "*")) {
            node = new Node(ND_MUL, node, unary(&tok, tok->next));
            continue;
        }

        if (equal(tok, "/")) {
            node = new Node(ND_DIV, node, unary(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// unary = ("+" | "-") unary
//       | primary
static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }

    if (equal(tok, "-")) {
        return new Node(ND_NEG, unary(rest, tok->next), nullptr);
    }

    return primary(rest, tok);
}

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    if (tok->kind == TK_NUM) {
        Node *node = new Node(ND_NUM, tok->val);
        *rest = tok->next;
        return node;
    }

    error_tok(tok, "expected an expression");
}

//
// Code generator
//

static int depth;

static void push(void) {
    fmt::print("  push %rax\n");
    depth++;
}

static void pop(const char *arg) {
    fmt::print("  pop {}\n", arg);
    depth--;
}

static void gen_expr(Node *node) {
    switch (node->kind) {
        case ND_NUM:
            fmt::print("  mov ${}, %rax\n", node->val);
            return;
        case ND_NEG:
            gen_expr(node->lhs);
            fmt::print("  neg %rax\n");
            return;
    }

    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");

    switch (node->kind) {
        case ND_ADD:
            fmt::print("  add %rdi, %rax\n");
            return;
        case ND_SUB:
            fmt::print("  sub %rdi, %rax\n");
            return;
        case ND_MUL:
            fmt::print("  imul %rdi, %rax\n");
            return;
        case ND_DIV:
            fmt::print("  cqo\n");
            fmt::print("  idiv %rdi\n");
            return;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            fmt::print("  cmp %rdi, %rax\n");

            if (node->kind == ND_EQ) {
                fmt::print("  sete %al\n");
            } else if (node->kind == ND_NE) {
                fmt::print("  setne %al\n");
            } else if (node->kind == ND_LT) {
                fmt::print("  setl %al\n");
            } else if (node->kind == ND_LE) {
                fmt::print("  setle %al\n");
            }
            fmt::print("  movzb %al, %rax\n");
            return;
    }

    error("invalid expression");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s : invalid number of arguments", argv[0]);
    }

    // Tokenize and parse.
    current_input = argv[1];
    Token *tok = tokenize();
    Node *node = expr(&tok, tok);

    if (tok->kind != TK_EOF) {
        fmt::print("{}\n", (int) tok->kind);
        error_tok(tok, "extra token");
    }

    fmt::print("  .global main\n");
    fmt::print("main:\n");

    // Traverse the AST to emit assembly.
    gen_expr(node);
    fmt::print("  ret\n");

    assert(depth == 0);
    return 0;
}
