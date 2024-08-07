#include <iostream>
#include <fmt/core.h>

#include <cstdlib>
#include <cstdarg>

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

    Token(TokenKind p_kind, char *start, int p_len): kind(p_kind), loc(start), len(p_len) {}

    Token() {}
};

// Reports an error and exit.
static void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

// Consumes the current token if it matches `s`.
static bool equal(Token *tok, const char *op) {
    return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `s`,
static Token *skip(Token *tok, const char *s) {
    if (!equal(tok, s)) {
        error("expected '%s'", s);
    }
    return tok->next;
}

// Ensure that the current token is TK_NUM.
static int get_number(Token *tok) {
    if (tok->kind != TK_NUM) {
        error("expected a number");
    }
    return tok->val;
}

// Tokenize `p` and returns new tokens.
static Token *tokenize(char *p) {
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

        // Punctuator
        if (*p == '+' || *p == '-') {
            cur = cur->next = new Token(TK_PUNCT, p, 1);
            p++;
            continue;
        }

        error("invalid token");
    }

    cur = cur->next = new Token(TK_EOF, p, 0);
    return head.next;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << argv[0] << ": invalid number of arguments" << std::endl;
    return 1;
  }

  Token *tok = tokenize(argv[1]);

  fmt::print("  .global main\n");
  fmt::print("main:\n");

  // The first token must be a number
  fmt::print("  mov ${}, %rax\n", get_number(tok));
  tok = tok->next;

  // ... followed by either `+ <number>` or `- <number>`.
  while (tok->kind != TK_EOF) {
      if (equal(tok, "+")) {
          fmt::print("  add ${}, %rax\n", get_number(tok->next));
          tok = tok->next->next;
          continue;
      }

      tok = skip(tok, "-");
      fmt::print("  sub ${}, %rax\n", get_number(tok));
      tok = tok->next;
  }

  fmt::print("  ret\n");

  return 0;
}
