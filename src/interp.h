#ifndef __INTERP_H__
#define __INTERP_H__

typedef enum {
	TOK_EOF = 0,
	// 1 ... 127 reserved for chars
	TOK_IDENT = 128,
	TOK_CONSTANT,
	TOK_INVALID,
} Token_Type;

typedef struct {
	Token_Type type;
	union {
		double value;
		char ident[32];
	};
} Token;

typedef enum {
	NODE_BINOP,
	NODE_CONSTANT,
	NODE_IDENT,
	NODE_FUNCALL,
} Node_Type;

typedef struct Expr_Node {
	Node_Type type;
	union {
		struct {
			Token_Type op;
			struct Expr_Node *left;
			struct Expr_Node *right;
		} binop;
		struct {
			struct Expr_Node *args[12];
			int arg_count;
			char ident[32];
		} funcall;
		double value;
		char ident[32];
	};
} Expr_Node;

Expr_Node *parse_expression(const char *text);
double eval_expr(Expr_Node *expr, double x);

#endif
