#include "interp.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

static int cursor = 0;
static const char *current_expr;
static Token current_token;
static jmp_buf out_error;

double num_to_tok() {
	int num_begin = cursor;
	while ((current_expr[cursor] >= '0' &&
		current_expr[cursor] <= '9') ||
	       (current_expr[cursor] == '.' ||
		current_expr[cursor] == '-')) {
		cursor++;
	}
	char *end = (char *)&current_expr[cursor - 1];
	return SDL_strtod(&current_expr[num_begin], &end);
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
	case 'x':
		current_token = (Token) {
			.type = TOK_IDENT,
			.ident[0] = c,
			.ident[1] = 0,
		};
		cursor++;
		break;
	case '-':
		if (current_expr[cursor + 1] >= '0' &&
		    current_expr[cursor + 1] <= '9') {
			current_token = (Token) {
				.type = TOK_CONSTANT,
				.value = num_to_tok(),
			};
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
		current_token = (Token) {
			.type = TOK_CONSTANT,
			.value = num_to_tok(),
		};
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

double eval_expr(Expr_Node *expr, double x) {
	if (expr->type == NODE_CONSTANT) {
		return expr->value;
	} else if (expr->type == NODE_IDENT) {
		if (!SDL_strcmp("x", expr->ident)) {
			return x;
		}
	} else if (expr->type == NODE_BINOP) {
		switch ((int) expr->binop.op) {
		case '+':
			return eval_expr(expr->binop.left, x) + eval_expr(expr->binop.right, x);
		case '-':
			return eval_expr(expr->binop.left, x) - eval_expr(expr->binop.right, x);
		case '/':
			return eval_expr(expr->binop.left, x) / eval_expr(expr->binop.right, x);
		case '*':
			return eval_expr(expr->binop.left, x) * eval_expr(expr->binop.right, x);
		case '^':
			return SDL_pow(eval_expr(expr->binop.left, x), eval_expr(expr->binop.right, x));
		case '>':
			return SDL_min(eval_expr(expr->binop.left, x), eval_expr(expr->binop.right, x));
		case '<':
			return SDL_max(eval_expr(expr->binop.left, x), eval_expr(expr->binop.right, x));
		default:
			return 0;
		}
	}
	return 0;
}

