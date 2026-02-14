#include "interp.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

static int cursor = 0;
static const char *current_expr;
static Token current_token;
static jmp_buf out_error;

void num_to_tok() {
	int num_begin = cursor;
	while ((current_expr[cursor] >= '0' &&
		current_expr[cursor] <= '9') ||
	       (current_expr[cursor] == '.' ||
		current_expr[cursor] == '-')) {
		cursor++;
	}
	char *end = (char *)&current_expr[cursor - 1];
	current_token.type = TOK_CONSTANT;
	current_token.value = SDL_strtod(&current_expr[num_begin], &end);
}

void ident_to_tok() {
	int ident_len = 0;
	while ((current_expr[cursor] >= 'a' &&
		current_expr[cursor] <= 'z') ||
	       (current_expr[cursor] >= 'A' &&
		current_expr[cursor] <= 'Z')) {

		current_token.ident[ident_len] = current_expr[cursor];
		ident_len++;
		if (ident_len >= 32)
			longjmp(out_error, 1);
		cursor++;
	}
	current_token.type = TOK_IDENT;
	current_token.ident[ident_len] = 0;
}

void next_token() {
again:
	char c = current_expr[cursor];
	switch (c) {
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		cursor++;
		goto again;
	case 'a' ... 'z':
	case 'A' ... 'Z':
		ident_to_tok();
		break;
	case '-':
		if (current_expr[cursor + 1] >= '0' &&
		    current_expr[cursor + 1] <= '9') {
			num_to_tok();
			break;
		}
	case '+':
	case '/':
	case '*':
	case '^':
	case '>': // Min
	case '<': // Max
	case '(':
	case ')':
		current_token = (Token) {
			.type = c,
		};
		cursor++;
		break;
	case '\0':
		current_token = (Token) {
			.type = TOK_EOF,
		};
		break;
	case '0' ... '9':
		num_to_tok();
		break;
	default:
		current_token = (Token) {
			.type = TOK_INVALID,
		};
		longjmp(out_error, 1);
		break;
	}
}

void expect(Token_Type t) {
	if (current_token.type != t) {
		longjmp(out_error, 1);
	} else {
		next_token();
	}
}

Expr_Node *new_node() {
	Expr_Node *n = SDL_malloc(sizeof(Expr_Node));
	if (!n) {
		printf("OOM!\n");
		exit(1);
	}

	return n;
}

Expr_Node *new_constant(double value) {
	Expr_Node *n = new_node();
	n->type = NODE_CONSTANT;
	n->value = value;
	return n;
}

Expr_Node *new_binop(Token_Type op, Expr_Node *left, Expr_Node *right) {
	Expr_Node *n = new_node();
	n->type = NODE_BINOP;
	n->binop.op = op;
	n->binop.left = left;
	n->binop.right = right;
	return n;
}

Expr_Node *new_ident(Token *tok) {
	Expr_Node *n = new_node();
	n->type = NODE_IDENT;
	memcpy(n->ident, tok->ident, 32);
	return n;
}

Expr_Node *parse_subexpr(int min_prec);

Expr_Node *parse_primary() {
	Expr_Node *result = 0;
	if (current_token.type == TOK_CONSTANT) {
		result = new_constant(current_token.value);
		next_token();
	} else if (current_token.type == TOK_IDENT) {
		result = new_ident(&current_token);
		next_token();
	} else if (current_token.type == '(') {
		next_token();
		result = parse_subexpr(1);
		expect(')');
	} else {
		longjmp(out_error, 1);
	}
	return result;
}

const char binops[128] = {
	['+'] = 1, ['-'] = 1, ['^'] = 1,
	['*'] = 1, ['/'] = 1, ['<'] = 1,
	['>'] = 1,
};

int get_prec() {
	switch ((int) current_token.type) {
	case '*':
	case '/':
		return 3;
	case '^':
		return 2;
	default:
		return 1;
	}
}

Expr_Node *parse_subexpr(int min_prec) {
	Expr_Node *primary = parse_primary();

	while (binops[current_token.type]) {
		int prec = get_prec();
		int next_prec = min_prec + 1;

		if (prec >= min_prec) {
		} else {
			break;
		}

		Token optok = current_token;
		next_token();

		Expr_Node *right = parse_subexpr(next_prec);
		primary = new_binop(optok.type, primary, right);
	}
	return primary;
}

Expr_Node *parse_expression(const char *text) {
	current_expr = text;
	if (setjmp(out_error) == 0) {
		// ok, setup
	} else {
		printf("error! %s\n", current_expr);
		printf("      %*c^\n", cursor + 1, ' ');
		return 0;
	}

	next_token();

	return parse_subexpr(1);
}

typedef struct {
	const char *ident;
	double value;
} Constant;

static Constant constants[] = {
	{ "e", 2.71828182 },
	{ "pi", 3.14159265 },
	{ "tau", 6.28318530 },
};
static const int constant_count = sizeof(constants) / sizeof(Constant);


static double current_x;

double eval_expr_real(Expr_Node *expr) {
	if (expr->type == NODE_CONSTANT) {
		return expr->value;
	} else if (expr->type == NODE_IDENT) {
		if (!SDL_strcmp("x", expr->ident)) {
			return current_x;
		} else {
			for (int i = 0; i < constant_count; i++) {
				Constant c = constants[i];
				if (!SDL_strcmp(c.ident, expr->ident)) {
					return c.value;
				}
			}
		}
	} else if (expr->type == NODE_BINOP) {
		switch ((int) expr->binop.op) {
		case '+':
			return eval_expr_real(expr->binop.left) + eval_expr_real(expr->binop.right);
		case '-':
			return eval_expr_real(expr->binop.left) - eval_expr_real(expr->binop.right);
		case '/':
			return eval_expr_real(expr->binop.left) / eval_expr_real(expr->binop.right);
		case '*':
			return eval_expr_real(expr->binop.left) * eval_expr_real(expr->binop.right);
		case '^':
			return SDL_pow(eval_expr_real(expr->binop.left), eval_expr_real(expr->binop.right));
		case '>':
			return SDL_min(eval_expr_real(expr->binop.left), eval_expr_real(expr->binop.right));
		case '<':
			return SDL_max(eval_expr_real(expr->binop.left), eval_expr_real(expr->binop.right));
		default:
			return 0;
		}
	}
	return 0;
}

double eval_expr(Expr_Node *expr, double x) {
	current_x = x;
	return eval_expr_real(expr);
}


