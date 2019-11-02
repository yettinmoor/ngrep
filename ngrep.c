#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1000
#define MAX(A, B) ((A) < (B)) ? (B) : (A)

#define MAX_COL 32

/*
 * TODO:
 * Implement col as array of *RE
 * Meta/escape type? \d, \s
 */
enum { NULLR, CHAR, QUANT, COL, SPEC, };

struct RE {
	int type;
	int val;
	char *col;
	int invcol;
};

struct RE *RE_convert(char *re_string);

int match(struct RE *re, char *text);
int match_here(struct RE *re, char *text);
int match_star(struct RE *re, char *text);
int match_char(struct RE *re, char c);

/* Apply match to successive lines. Nothing to see here */
int main(int argc, char **argv)
{
	if (argc == 1) {
		printf("Too few arguments, exiting.\n");
		exit(1);
	}

	struct RE *re = RE_convert(*++argv);
	FILE *fp = (argc == 2) ? stdin : fopen(*++argv, "r");

	char line[MAXLINE + 1];

	while (fgets(line, MAXLINE, fp) != NULL) {
		line[MAX(0, strlen(line) - 1)] = '\0';
		if (match(re, line))
			printf("%s\n", line);
	}

	free(re);
}

/* Convert a regex string to a struct RE array.
 * Array is terminated by a struct RE of type NULLR. */
struct RE *
RE_convert(char *re_string)
{
	struct RE *re, *r;
	re = r = malloc((strlen(re_string) + 1) * sizeof(*re));

	while (*re_string != '\0') {
		switch (r->val = *re_string) {
		case '\\':
			r->val = *++re_string;
			r->type = CHAR;
			break;
		case '*': /* QUANT */
		case '?': /* FALLTHROUGH */
		case '+':
			if (re == r) {
				perror("ERROR: Quantifier not allowed at the beginning of the regex.\n");
				exit(1);
			}
			r->type = QUANT;
			break;
		case '[':
			r->type = COL;
			char *col = r->col = malloc(MAX_COL * sizeof(*col));
			r->invcol = (*++re_string == '^');
			if (r->invcol) ++re_string;
			while (*re_string != ']') {
				if (*re_string == '\0' || col - r->col >= MAX_COL - 1) {
					perror("ERROR: Unterminated '['.\n");
					exit(1);
				}
				if (col - r->col >= MAX_COL - 1) {
					perror("ERROR: Bracket statement '[...]' too large.\n");
					exit(1);
				}
				if (*re_string == '\\') ++re_string;
				*col++ = *re_string++;
			}
			*col = '\0';
			break;
		case '^': /* SPEC */
		case '$': /* FALLTHROUGH */
		case '.':
			r->type = SPEC;
			break;
		default:
			r->type = CHAR;
		}
		++r;
		++re_string;
	}

	r->type = NULLR;
	return re;
}

int
match(struct RE *re, char *text)
{
	if (re->type == SPEC && re->val == '^')
		return match_here(re + 1, text);
	do {
		if (match_here(re, text))
			return 1;
	} while (*text++ != '\0');
	return 0;
}

int
match_here(struct RE *re, char *text)
{
	if (re->type == NULLR)
		return 1;
	if (re->type == SPEC && re->val == '$')
		return *text == '\0';
	if ((re + 1)->type == QUANT)
		return match_star(re, text);
	if (*text != '\0' && match_char(re, *text))
		return match_here(re + 1, text + 1);
	return 0;
}

int
match_star(struct RE *re, char *text)
{
	struct RE *cont = re + 2;
	switch ((re + 1)->val) {
	case '*':
		do {
			if (match_here(cont, text)) return 1;
		} while (*text != '\0' && match_char(re, *text++));
		break;
	case '+':
		while (*text != '\0' && match_char(re, *text++))
			if (match_here(cont, text)) return 1;
		break;
	case '?':
		return match_here(cont, text) || match_here(cont, text + 1);
	}
	return 0;
}

int
match_char(struct RE *re, char c)
{
	char *s;
	switch (re->type) {
	case CHAR:
		return re->val == c;
	case SPEC:
		return re->val == '.';
	case COL:
		s = re->col;
		while (*s != '\0' && *s != c)
			++s;
		return (*s == c) ^ re->invcol;
	}
	return 0;
}
