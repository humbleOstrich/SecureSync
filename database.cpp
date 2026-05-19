#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "database.hpp"

static void
strip_quotes(char *s)
{
	int len;
	if (!s) return;
	len = strlen(s);
	if (len >= 2 && s[0] == '"' && s[len-1] == '"') {
		memmove(s, s+1, len-1);
		s[len-2] = '\0';
	}
}

static Table *
table_new(const char *name)
{
	Table *t = (Table *)calloc(1, sizeof(Table));
	if (t && name) t->name = strdup(name);
	return t;
}

void
table_free(Table *t)
{
	int i, j;
	if (!t) return;
	free(t->name);
	for (i = 0; i < t->ncols; i++) {
		free(t->colnames[i]);
		free(t->coltypes[i]);
	}
	free(t->colnames);
	free(t->coltypes);
	for (i = 0; i < t->nrows; i++) {
		for (j = 0; j < t->ncols; j++)
			free(t->rows[i][j]);
		free(t->rows[i]);
	}
	free(t->rows);
	free(t);
}

typedef struct {
	char **tokens;
	int    n;
} Tokens;

static void
tokens_free(Tokens *tok)
{
	int i;
	for (i = 0; i < tok->n; i++)
		free(tok->tokens[i]);
	free(tok->tokens);
	tok->tokens = NULL;
	tok->n = 0;
}

static int
ispunctchar(int c)
{
	return c == '(' || c == ')' || c == ',' || c == ';' ||
	       c == '=' || c == '>' || c == '<';
}

static int
tokenize(const char *s, Tokens *out)
{
	const char *p = s;
	char buf[4096];
	out->tokens = NULL;
	out->n = 0;
	if (!s) return 0;

	while (*p) {
		while (*p && isspace(*p)) p++;
		if (*p == '\0') break;

		if (*p == '\'') {
			p++;
			int i = 0;
			while (*p && *p != '\'') {
				if (i < (int)sizeof(buf)-1) buf[i++] = *p;
				p++;
			}
			buf[i] = '\0';
			if (*p == '\'') p++;
			out->tokens = (char **)realloc(out->tokens, (out->n+1)*sizeof(char *));
			out->tokens[out->n++] = strdup(buf);
		}
		else if (ispunctchar(*p)) {
			char tmp[2] = { *p, '\0' };
			out->tokens = (char **)realloc(out->tokens, (out->n+1)*sizeof(char *));
			out->tokens[out->n++] = strdup(tmp);
			p++;
		}
		else {
			int i = 0;
			while (*p && !isspace(*p) && !ispunctchar(*p)) {
				if (i < (int)sizeof(buf)-1) buf[i++] = *p;
				p++;
			}
			buf[i] = '\0';
			out->tokens = (char **)realloc(out->tokens, (out->n+1)*sizeof(char *));
			out->tokens[out->n++] = strdup(buf);
		}
	}
	return 0;
}

static int
col_index(const Table *t, const char *name)
{
	int i;
	for (i = 0; i < t->ncols; i++)
		if (strcmp(t->colnames[i], name) == 0)
			return i;
	return -1;
}

MultiTable *
multitable_load(const char *filename)
{
	FILE *f = fopen(filename, "r");
	char buf[4096], *p, *tok, *typ;
	int len, blank_line = 1;
	MultiTable *mt;
	Table *cur = NULL;

	/* file does not exist → return an empty database */
	if (!f) {
		mt = (MultiTable *)calloc(1, sizeof(MultiTable));
		return mt;
	}

	mt = (MultiTable *)calloc(1, sizeof(MultiTable));
	if (!mt) { fclose(f); return NULL; }

	while (fgets(buf, sizeof(buf), f)) {
		len = strlen(buf);
		while (len > 0 && (buf[len-1]=='\n' || buf[len-1]=='\r'))
			buf[--len] = '\0';
		p = buf; while (*p && isspace(*p)) p++;
		if (*p == '\0') { blank_line = 1; continue; }

		if (*p == '"' && !strchr(p, ',') && strchr(p+1, '"') &&
		    (!cur || blank_line)) {
			if (cur) {
				mt->tables = (Table **)realloc(mt->tables, (mt->n+1)*sizeof(Table *));
				mt->tables[mt->n++] = cur;
			}
			strip_quotes(p);
			cur = table_new(p);
			blank_line = 0;
		} else if (cur && cur->ncols == 0) {
			tok = strtok(p, ",\n");
			while (tok) {
				strip_quotes(tok);
				typ = strchr(tok, ':');
				if (typ) *typ++ = '\0';
				cur->colnames = (char **)realloc(cur->colnames, (cur->ncols+1)*sizeof(char *));
				cur->coltypes = (char **)realloc(cur->coltypes, (cur->ncols+1)*sizeof(char *));
				cur->colnames[cur->ncols] = strdup(tok);
				cur->coltypes[cur->ncols] = strdup(typ ? typ : "TEXT");
				cur->ncols++;
				tok = strtok(NULL, ",\n");
			}
		} else if (cur) {
			char **row = (char **)malloc(cur->ncols * sizeof(char *));
			int j = 0;
			tok = strtok(p, ",\n");
			while (tok && j < cur->ncols) {
				strip_quotes(tok);
				row[j++] = strdup(tok);
				tok = strtok(NULL, ",\n");
			}
			while (j < cur->ncols) row[j++] = strdup("");
			cur->rows = (char ***)realloc(cur->rows, (cur->nrows+1)*sizeof(char **));
			cur->rows[cur->nrows++] = row;
		}
	}
	fclose(f);
	if (cur) {
		mt->tables = (Table **)realloc(mt->tables, (mt->n+1)*sizeof(Table *));
		mt->tables[mt->n++] = cur;
	}
	return mt;
}

int
multitable_save(const MultiTable *mt, const char *filename)
{
	FILE *f = fopen(filename, "w");
	int i, j, k;
	if (!f) return -1;
	for (i = 0; i < mt->n; i++) {
		Table *t = mt->tables[i];
		fprintf(f, "\"%s\"\n", t->name);
		for (j = 0; j < t->ncols; j++) {
			if (j > 0) fputc(',', f);
			fprintf(f, "\"%s:%s\"", t->colnames[j], t->coltypes[j]);
		}
		fputc('\n', f);
		for (j = 0; j < t->nrows; j++) {
			for (k = 0; k < t->ncols; k++) {
				if (k > 0) fputc(',', f);
				fprintf(f, "\"%s\"", t->rows[j][k]);
			}
			fputc('\n', f);
		}
		if (i < mt->n - 1) fputc('\n', f);
	}
	fclose(f);
	return 0;
}

void
multitable_free(MultiTable *mt)
{
	int i;
	if (!mt) return;
	for (i = 0; i < mt->n; i++)
		table_free(mt->tables[i]);
	free(mt->tables);
	free(mt);
}

static Table *
multitable_find(const MultiTable *mt, const char *name)
{
	int i;
	for (i = 0; i < mt->n; i++)
		if (mt->tables[i]->name && strcmp(mt->tables[i]->name, name) == 0)
			return mt->tables[i];
	return NULL;
}

static int
eval_condition(const Table *t, int row, const Tokens *tok, int *pos)
{
	int col, op;
	char *val;
	long a, b;
	if (*pos >= tok->n) return -1;

	col = col_index(t, tok->tokens[*pos]);
	if (col < 0) return -1;
	(*pos)++;
	if (*pos >= tok->n) return -1;

	if      (strcmp(tok->tokens[*pos], "=") == 0) op = 0;
	else if (strcmp(tok->tokens[*pos], ">") == 0) op = 1;
	else if (strcmp(tok->tokens[*pos], "<") == 0) op = 2;
	else return -1;
	(*pos)++;
	if (*pos >= tok->n) return -1;
	val = tok->tokens[(*pos)++];

	if (strcasecmp(t->coltypes[col], "INT") == 0) {
		a = atol(t->rows[row][col]);
		b = atol(val);
	} else {
		a = (long)strcmp(t->rows[row][col], val);
		b = 0;
	}
	switch (op) {
		case 0: return (a == b);
		case 1: return (a > b);
		case 2: return (a < b);
	}
	return 0;
}

static int
eval_conditions(const Table *t, int row, const Tokens *tok, int *pos)
{
	int res;
	res = eval_condition(t, row, tok, pos);
	if (res < 0) return -1;
	while (*pos < tok->n && strcasecmp(tok->tokens[*pos], "AND") == 0) {
		(*pos)++;
		int r = eval_condition(t, row, tok, pos);
		if (r < 0) return -1;
		res = res && r;
	}
	return res;
}

static int
do_select(MultiTable *mt, const Tokens *tok, int *pos, FILE *out)
{
	int star = 0, nsel = 0, i, j, *sel = NULL;
	Table *t;
	char **colnames = NULL;

	if (*pos >= tok->n) { fprintf(stderr, "Error: missing columns\n"); return -1; }
	if (strcmp(tok->tokens[*pos], "*") == 0) {
		star = 1;
		(*pos)++;
	} else {
		while (*pos < tok->n && strcasecmp(tok->tokens[*pos], "FROM") != 0) {
			if (strcmp(tok->tokens[*pos], ",") == 0) { (*pos)++; continue; }
			colnames = (char **)realloc(colnames, (nsel+1)*sizeof(char *));
			colnames[nsel++] = strdup(tok->tokens[*pos]);
			(*pos)++;
			if (*pos < tok->n && strcmp(tok->tokens[*pos], ",") == 0) (*pos)++;
		}
	}

	if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "FROM") != 0) {
		fprintf(stderr, "Error: expected FROM\n");
		if (colnames) { for (i = 0; i < nsel; i++) free(colnames[i]); free(colnames); }
		free(sel);
		return -1;
	}
	(*pos)++;
	if (*pos >= tok->n) {
		fprintf(stderr, "Error: missing table name\n");
		if (colnames) { for (i = 0; i < nsel; i++) free(colnames[i]); free(colnames); }
		free(sel);
		return -1;
	}
	t = multitable_find(mt, tok->tokens[*pos]);
	if (!t) {
		fprintf(stderr, "Error: table '%s' not found\n", tok->tokens[*pos]);
		if (colnames) { for (i = 0; i < nsel; i++) free(colnames[i]); free(colnames); }
		free(sel);
		return -1;
	}
	(*pos)++;

	if (star) {
		nsel = t->ncols;
		sel = (int *)malloc(nsel * sizeof(int));
		for (i = 0; i < nsel; i++) sel[i] = i;
	} else {
		sel = (int *)malloc(nsel * sizeof(int));
		for (i = 0; i < nsel; i++) {
			int idx = col_index(t, colnames[i]);
			if (idx < 0) {
				fprintf(stderr, "Error: unknown column '%s'\n", colnames[i]);
				for (j = 0; j < nsel; j++) free(colnames[j]);
				free(colnames);
				free(sel);
				return -1;
			}
			sel[i] = idx;
		}
		for (i = 0; i < nsel; i++) free(colnames[i]);
		free(colnames);
	}

	int *matches = NULL;
	if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "WHERE") == 0) {
		(*pos)++;
		matches = (int *)malloc(t->nrows * sizeof(int));
		int nmatch = 0;
		for (i = 0; i < t->nrows; i++) {
			int pos2 = *pos;
			int r = eval_conditions(t, i, tok, &pos2);
			if (r < 0) {
				fprintf(stderr, "Error: invalid WHERE condition\n");
				free(matches);
				free(sel);
				return -1;
			}
			if (r) matches[nmatch++] = i;
		}
		for (i = 0; i < nsel; i++) {
			if (i > 0) fputc(',', out);
			fprintf(out, "%s", t->colnames[sel[i]]);
		}
		fputc('\n', out);
		for (i = 0; i < nmatch; i++) {
			for (j = 0; j < nsel; j++) {
				if (j > 0) fputc(',', out);
				fprintf(out, "%s", t->rows[matches[i]][sel[j]]);
			}
			fputc('\n', out);
		}
		free(matches);
	} else {
		for (i = 0; i < nsel; i++) {
			if (i > 0) fputc(',', out);
			fprintf(out, "%s", t->colnames[sel[i]]);
		}
		fputc('\n', out);
		for (i = 0; i < t->nrows; i++) {
			for (j = 0; j < nsel; j++) {
				if (j > 0) fputc(',', out);
				fprintf(out, "%s", t->rows[i][sel[j]]);
			}
			fputc('\n', out);
		}
	}
	free(sel);
	while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
	return 0;
}

static int
do_create(MultiTable *mt, const Tokens *tok, int *pos, FILE *out)
{
	(void)out;
	int if_not_exists = 0;
	if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "TABLE") != 0) {
		fprintf(stderr, "Error: expected TABLE\n");
		return -1;
	}
	(*pos)++;
	if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "IF") == 0) {
		(*pos)++;
		if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "NOT") == 0) (*pos)++;
		if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "EXISTS") == 0) {
			if_not_exists = 1;
			(*pos)++;
		}
	}
	if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
	char *tname = tok->tokens[(*pos)++];
	Table *existing = multitable_find(mt, tname);
	if (existing) {
		if (if_not_exists) {
			while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") != 0) (*pos)++;
			if (*pos < tok->n) (*pos)++;
			return 0;
		}
		fprintf(stderr, "Error: table '%s' already exists\n", tname);
		return -1;
	}
	if (*pos >= tok->n || strcmp(tok->tokens[*pos], "(") != 0) {
		fprintf(stderr, "Error: expected (\n");
		return -1;
	}
	(*pos)++;
	Table *nt = table_new(tname);
	while (*pos < tok->n && strcmp(tok->tokens[*pos], ")") != 0) {
		if (strcmp(tok->tokens[*pos], ",") == 0) { (*pos)++; continue; }
		char *colname = tok->tokens[(*pos)++];
		if (*pos >= tok->n || strcmp(tok->tokens[*pos], ",") == 0 || strcmp(tok->tokens[*pos], ")") == 0) {
			fprintf(stderr, "Error: missing column type\n");
			table_free(nt);
			return -1;
		}
		char *coltype = tok->tokens[(*pos)++];
		nt->colnames = (char **)realloc(nt->colnames, (nt->ncols+1)*sizeof(char *));
		nt->coltypes = (char **)realloc(nt->coltypes, (nt->ncols+1)*sizeof(char *));
		nt->colnames[nt->ncols] = strdup(colname);
		nt->coltypes[nt->ncols] = strdup(coltype);
		nt->ncols++;
		if (*pos < tok->n && strcmp(tok->tokens[*pos], ",") == 0) (*pos)++;
	}
	if (*pos >= tok->n || strcmp(tok->tokens[*pos], ")") != 0) {
		fprintf(stderr, "Error: expected )\n");
		table_free(nt);
		return -1;
	}
	(*pos)++;
	if (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
	mt->tables = (Table **)realloc(mt->tables, (mt->n+1)*sizeof(Table *));
	mt->tables[mt->n++] = nt;
	return 0;
}

static int
do_drop(MultiTable *mt, const Tokens *tok, int *pos, FILE *out)
{
	(void)out;
	int if_exists = 0;
	if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "TABLE") != 0) {
		fprintf(stderr, "Error: expected TABLE\n");
		return -1;
	}
	(*pos)++;
	if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "IF") == 0) {
		(*pos)++;
		if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "EXISTS") == 0) {
			if_exists = 1;
			(*pos)++;
		}
	}
	if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
	char *tname = tok->tokens[(*pos)++];
	Table *t = multitable_find(mt, tname);
	if (!t) {
		if (if_exists) {
			while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") != 0) (*pos)++;
			if (*pos < tok->n) (*pos)++;
			return 0;
		}
		fprintf(stderr, "Error: table '%s' not found\n", tname);
		return -1;
	}
	int i, found = 0;
	for (i = 0; i < mt->n; i++) {
		if (mt->tables[i] == t) { found = i; break; }
	}
	table_free(t);
	memmove(&mt->tables[found], &mt->tables[found+1], (mt->n - found - 1)*sizeof(Table *));
	mt->n--;
	while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
	return 0;
}

static int
do_insert(MultiTable *mt, const Tokens *tok, int *pos, FILE *out)
{
	(void)out;
	if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "INTO") != 0) {
		fprintf(stderr, "Error: expected INTO\n");
		return -1;
	}
	(*pos)++;
	if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
	Table *t = multitable_find(mt, tok->tokens[(*pos)++]);
	if (!t) { fprintf(stderr, "Error: table not found\n"); return -1; }
	if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "VALUES") != 0) {
		fprintf(stderr, "Error: expected VALUES\n");
		return -1;
	}
	(*pos)++;
	if (*pos >= tok->n || strcmp(tok->tokens[*pos], "(") != 0) {
		fprintf(stderr, "Error: expected (\n");
		return -1;
	}
	(*pos)++;
	char **row = (char **)malloc(t->ncols * sizeof(char *));
	int j = 0;
	while (*pos < tok->n && strcmp(tok->tokens[*pos], ")") != 0 && j < t->ncols) {
		row[j++] = strdup(tok->tokens[(*pos)++]);
		if (*pos < tok->n && strcmp(tok->tokens[*pos], ",") == 0) (*pos)++;
	}
	while (j < t->ncols) row[j++] = strdup("");
	if (*pos < tok->n && strcmp(tok->tokens[*pos], ")") == 0) (*pos)++;
	t->rows = (char ***)realloc(t->rows, (t->nrows+1)*sizeof(char **));
	t->rows[t->nrows++] = row;
	while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
	return 0;
}

static int
do_delete(MultiTable *mt, const Tokens *tok, int *pos, FILE *out)
{
	(void)out;
	if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "FROM") != 0) {
		fprintf(stderr, "Error: expected FROM\n");
		return -1;
	}
	(*pos)++;
	if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
	Table *t = multitable_find(mt, tok->tokens[(*pos)++]);
	if (!t) { fprintf(stderr, "Error: table not found\n"); return -1; }

	if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "WHERE") == 0) {
		(*pos)++;
		int keep = 0, i;
		for (i = 0; i < t->nrows; i++) {
			int pos2 = *pos;
			int r = eval_conditions(t, i, tok, &pos2);
			if (r < 0) { fprintf(stderr, "Error: invalid WHERE condition\n"); return -1; }
			if (!r) {
				if (keep != i) t->rows[keep] = t->rows[i];
				keep++;
			} else {
				int j;
				for (j = 0; j < t->ncols; j++) free(t->rows[i][j]);
				free(t->rows[i]);
			}
		}
		t->nrows = keep;
	} else {
		int i, j;
		for (i = 0; i < t->nrows; i++) {
			for (j = 0; j < t->ncols; j++) free(t->rows[i][j]);
			free(t->rows[i]);
		}
		free(t->rows);
		t->rows = NULL;
		t->nrows = 0;
	}
	while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
	return 0;
}

void
execute_sql(MultiTable *mt, const char *sql, FILE *out)
{
	Tokens tok;
	int pos = 0;
	if (tokenize(sql, &tok) != 0) {
		fprintf(stderr, "Error: unmatched quote\n");
		return;
	}
	if (tok.n == 0) { tokens_free(&tok); return; }

	if (strcasecmp(tok.tokens[0], "SELECT") == 0)
		{ pos = 1; do_select(mt, &tok, &pos, out); }
	else if (strcasecmp(tok.tokens[0], "CREATE") == 0)
		{ pos = 1; do_create(mt, &tok, &pos, out); }
	else if (strcasecmp(tok.tokens[0], "DROP") == 0)
		{ pos = 1; do_drop(mt, &tok, &pos, out); }
	else if (strcasecmp(tok.tokens[0], "INSERT") == 0)
		{ pos = 1; do_insert(mt, &tok, &pos, out); }
	else if (strcasecmp(tok.tokens[0], "DELETE") == 0)
		{ pos = 1; do_delete(mt, &tok, &pos, out); }
	else
		fprintf(stderr, "Error: unknown statement\n");

	tokens_free(&tok);
}