
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define assert(x)	\
do {	\
	if (!(x)) {	\
		printf("assert failed in %s line %d: %s\n", __FILE__, __LINE__, #x);	\
		exit(1);	\
	}	\
} while (0)

void *xmalloc(size_t n)
{
	void *p = malloc(n);
	if (!p) {
		perror("malloc() failed");
		exit(1);
	}
	return p;
}

void *xrealloc(void *p, size_t n)
{
	void *q = realloc(p, n);
	if (!q) {
		perror("realloc() failed");
		exit(1);
	}
	return q;
}

// dynamic arrays

typedef struct arrhdr {
	size_t m;
	size_t n;
	char e[];
} arrhdr;

#define arrhdr_(a) ((arrhdr *)((char *)(a) - offsetof(arrhdr, e)))
#define arrfit_(a, n) (arrlen(a) + (n) <= arrcap(a) ? (a) : ((a) = arrgrow_((a), arrlen(a) + (n), sizeof(*(a)))))

#define arrlen(a) ((a) ? arrhdr_(a)->n : 0)
#define arrcap(a) ((a) ? arrhdr_(a)->m : 0)
#define arrpush(a, ...) (arrfit_((a), 1), (a)[arrlen(a)] = (__VA_ARGS__), arrhdr_(a)->n++)
#define arrfree(a) (arrfree_(a), (a) = 0)

void arrfree_(void *a)
{
	if (a) {
		free(arrhdr_(a));
	}
}

void *arrgrow_(void *a, size_t n, size_t esize)
{
	arrhdr *h;

	if (!a) {
		h = xmalloc(sizeof(arrhdr) + n * esize);
		h->m = n;
		h->n = 0;
	}
	else {
		size_t m = n > arrcap(a) * 2 ? n : arrcap(a) * 2;
		h = xrealloc(arrhdr_(a), sizeof(arrhdr) + m * esize);
		h->m = m;
	}

	return h->e;
}

void arrtest(void)
{
	int *ints = NULL;
	enum { N = 1024 };

	assert(arrcap(ints) == 0);
	assert(arrlen(ints) == 0);

	for (int i = 0; i < N; ++i) {
		arrpush(ints, i);
	}

	assert(arrlen(ints) == N);
	assert(arrcap(ints) >= N);
	for (int i = 0; i < N; ++i) {
		assert(ints[i] == i);
	}

	arrfree(ints);
	assert(ints == 0);
}

// string interning

typedef struct internstr {
	size_t n;
	const char *s;
} internstr;

internstr *interns;

const char *strintern(const char *b, const char *e)
{
	size_t n = e - b;
	for (size_t i = 0; i < arrlen(interns); ++i) {
		if (interns[i].n == n && strncmp(interns[i].s, b, n) == 0) {
			return interns[i].s;
		}
	}

	char *t = xmalloc(n + 1);
	memcpy(t, b, n);
	t[n] = 0;
	arrpush(interns, (internstr){ n, t });
	return t;
}

const char *cstrintern(const char *s)
{
	return strintern(s, s + strlen(s));
}

void strinterntest(void)
{
	char *s = "hello";

	const char *t = cstrintern(s);
	assert(strcmp(t, s) == 0);
	assert(t != s);

	const char *p = cstrintern("hello");
	assert(p != s);
	assert(p == t);

	const char *q = cstrintern("hello");
	assert(q != s);
	assert(q == t);

	assert(cstrintern("hell") != t);
	assert(cstrintern("hello!") != t);

	assert(arrlen(interns) == 3);
	arrfree(interns);
}

// lexer

typedef enum tokenkind {
	TOKEN_EOF = 0,
	TOKEN_INT = 128,
	TOKEN_NAME,
	TOKEN_COLON,
	TOKEN_COLON_ASSIGN,
	TOKEN_ADD,
	TOKEN_SUB,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_ADD_ASSIGN,
	TOKEN_SUB_ASSIGN,
	TOKEN_MUL_ASSIGN,
	TOKEN_DIV_ASSIGN,
	TOKEN_EQ,
	TOKEN_ASSIGN,
	TOKEN_NOT,
	TOKEN_NEQ,
	TOKEN_XOR,
	TOKEN_XOR_ASSIGN,
	TOKEN_MOD,
	TOKEN_MOD_ASSIGN,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LT,		// <
	TOKEN_LTE,		// <=
	TOKEN_LSHIFT,		// <<
	TOKEN_LSHIFT_ASSIGN,	// <<=
	TOKEN_GT,		// >
	TOKEN_GTE,		// >=
	TOKEN_RSHIFT,		// >>
	TOKEN_RSHIFT_ASSIGN,	// >>=
	TOKEN_BITNOT,
	TOKEN_COMMA,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,
	TOKEN_LBRACE,
	TOKEN_RBRACE
} tokenkind;

typedef struct token {
	tokenkind kind;
	const char *start;
	const char *end;
	union {
		uint64_t intval;
		const char *name;
	};
} token;

const char *input;
token tok;

void nexttoken(void)
{
retry:
	tok.start = input;
	switch (*input) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			tok.kind = TOKEN_INT;
			uint64_t val = *input++ - '0';
			while (isdigit(*input)) {
				val = val * 10 + *input - '0';
				++input;
			}
			tok.intval = val;
		} break;

		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j':	case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D':	case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J':	case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case '_': {
			tok.kind = TOKEN_NAME;
			++input;
			while (isalnum(*input) || *input == '_') {
				++input;
			}
			tok.name = strintern(tok.start, input);
		} break;

#define CASE(c1, k1)	\
		case c1: {	\
			tok.kind = k1;	\
			++input;	\
		} break;

		CASE('\0', TOKEN_EOF)
		CASE('(', TOKEN_LPAREN)
		CASE(')', TOKEN_RPAREN)
		CASE('~', TOKEN_BITNOT)
		CASE(',', TOKEN_COMMA)
		CASE('[', TOKEN_LBRACKET)
		CASE(']', TOKEN_RBRACKET)
		CASE('{', TOKEN_LBRACE)
		CASE('}', TOKEN_RBRACE)
#undef CASE

#define CASE(c1, k1, c2, k2)			\
		case c1: {			\
			++input;		\
			if (*input == c2) {	\
				tok.kind = k2;	\
				++input;	\
			}			\
			else {			\
				tok.kind = k1;	\
			}			\
		} break;

		CASE(':', TOKEN_COLON, '=', TOKEN_COLON_ASSIGN)
		CASE('+', TOKEN_ADD, '=', TOKEN_ADD_ASSIGN)
		CASE('-', TOKEN_SUB, '=', TOKEN_SUB_ASSIGN)
		CASE('*', TOKEN_MUL, '=', TOKEN_MUL_ASSIGN)
		CASE('/', TOKEN_DIV, '=', TOKEN_DIV_ASSIGN)
		CASE('=', TOKEN_ASSIGN, '=', TOKEN_EQ)
		CASE('!', TOKEN_NOT, '=', TOKEN_NEQ)
		CASE('^', TOKEN_XOR, '=', TOKEN_XOR_ASSIGN)
		CASE('%', TOKEN_MOD, '=', TOKEN_MOD_ASSIGN)
#undef CASE

#define CASE(c1, k1, c2, k2, c3, k3, c4, k4)		\
		case c1: {				\
			++input;			\
			if (*input == c2) {		\
				tok.kind = k2;		\
				++input;		\
			}				\
			else if (*input == c3) {	\
				++input;		\
				if (*input == c4) {	\
					tok.kind = k4;	\
					++input;	\
				}			\
				else {			\
					tok.kind = k3;	\
				}			\
			}				\
			else {				\
				tok.kind = k1;		\
			}				\
		} break;

		CASE('<', TOKEN_LT, '=', TOKEN_LTE, '<', TOKEN_LSHIFT, '=', TOKEN_LSHIFT_ASSIGN)
		CASE('>', TOKEN_GT, '=', TOKEN_GTE, '>', TOKEN_RSHIFT, '=', TOKEN_RSHIFT_ASSIGN)
#undef CASE

		case '\r':
		case '\n':
		case '\v':
		case '\t':
		case ' ': {
			++input;
			goto retry;
		} break;

		default: {
			tok.kind = *input;
			tok.start = input;
			++input;
			tok.end = input;
		}
	}
	tok.end = input;
}

void initlex(const char *s)
{
	input = s;
	nexttoken();
}

void printtoken(void)
{
	switch (tok.kind) {

		case TOKEN_INT: {
			printf("INT %lld", tok.intval);
		} break;

		case TOKEN_NAME: printf("NAME intern = %s", tok.name); break;
		case TOKEN_EOF: printf("EOF"); break;
		case TOKEN_COLON: printf("COLON"); break;
		case TOKEN_COLON_ASSIGN: printf("COLON_ASSIGN"); break;
		case TOKEN_ADD: printf("ADD"); break;
		case TOKEN_SUB: printf("SUB"); break;
		case TOKEN_MUL: printf("MUL"); break;
		case TOKEN_DIV: printf("DIV"); break;
		case TOKEN_ADD_ASSIGN: printf("ADD_ASSIGN"); break;
		case TOKEN_SUB_ASSIGN: printf("SUB_ASSIGN"); break;
		case TOKEN_MUL_ASSIGN: printf("MUL_ASSIGN"); break;
		case TOKEN_DIV_ASSIGN: printf("DIV_ASSIGN"); break;
		case TOKEN_LPAREN: printf("LPAREN"); break;
		case TOKEN_RPAREN: printf("RPAREN"); break;
		case TOKEN_LT: printf("LT"); break;
		case TOKEN_LTE: printf("LTE"); break;
		case TOKEN_LSHIFT: printf("LSHIFT"); break;
		case TOKEN_LSHIFT_ASSIGN: printf("LSHIFT_ASSIGN"); break;
		case TOKEN_GT: printf("GT"); break;
		case TOKEN_GTE: printf("GTE"); break;
		case TOKEN_RSHIFT: printf("RSHIFT"); break;
		case TOKEN_RSHIFT_ASSIGN: printf("RSHIFT_ASSIGN"); break;
		case TOKEN_BITNOT: printf("BITNOT"); break;
		case TOKEN_NOT: printf("NOT"); break;
		case TOKEN_NEQ: printf("NEQ"); break;
		case TOKEN_COMMA: printf("COMMA"); break;
		case TOKEN_EQ: printf("EQ"); break;
		case TOKEN_ASSIGN: printf("ASSIGN"); break;
		case TOKEN_LBRACKET: printf("LBRACKET"); break;
		case TOKEN_RBRACKET: printf("RBRACKET"); break;
		case TOKEN_LBRACE: printf("LBRACE"); break;
		case TOKEN_RBRACE: printf("RBRACE"); break;

		default: {
			printf("unknown token = %d", tok.kind);
			exit(1);
		} break;
	}
	printf(" \"%.*s\"\n", (int)(tok.end - tok.start), tok.start);
}

void lextest(void)
{
	printf("\n");
	initlex("num := (123 + 456) * 789");
	while (tok.kind) {
		printtoken();
		nexttoken();
	}

	printf("\n");
	initlex("y += 1");
	while (tok.kind) {
		printtoken();
		nexttoken();
	}

	printf("\n");
	initlex("= == < <= << <<= > >= >> >>= ~ ! != , [] {} ()");
	while (tok.kind) {
		printtoken();
		nexttoken();
	}
}

int main(void)
{
	arrtest();
	strinterntest();
	lextest();

	return 0;
}
