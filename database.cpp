#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include "database.hpp"

static void strip_quotes(char *s) {
    int len;
    if (!s) return;
    len = strlen(s);
    if (len >= 2 && s[0] == '"' && s[len-1] == '"') {
        memmove(s, s+1, len-1);
        s[len-2] = '\0';
    }
}

static Table *table_new(const char *name) {
    Table *t = (Table *)calloc(1, sizeof(Table));
    if (t && name) t->name = strdup(name);
    return t;
}

void table_free(Table *t) {
    int i, j;
    if (!t) return;
    free(t->name);
    for (i = 0; i < t->ncols; i++) {
        free(t->colnames[i]);
    }
    free(t->colnames);
    free(t->coltypes);
    for (i = 0; i < t->nrows; i++) {
        for (j = 0; j < t->ncols; j++) free(t->rows[i][j]);
        free(t->rows[i]);
    }
    free(t->rows);
    free(t);
}

typedef struct {
    char **tokens;
    int    n;
} Tokens;

static void tokens_free(Tokens *tok) {
    for (int i = 0; i < tok->n; i++) free(tok->tokens[i]);
    free(tok->tokens);
    tok->tokens = NULL;
    tok->n = 0;
}

static int ispunctchar(int c) {
    return c == '(' || c == ')' || c == ',' || c == ';' ||
           c == '=' || c == '>' || c == '<' || c == '!';
}

static int tokenize(const char *s, Tokens *out) {
    const char *p = s;
    char buf[4096];
    out->tokens = NULL;
    out->n = 0;
    if (!s) return 0;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
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
        } else if (ispunctchar(*p)) {
            char tmp[3] = { *p, '\0' };
            if ((*p == '!' || *p == '<' || *p == '>') && *(p+1) == '=') {
                tmp[1] = '=';
                tmp[2] = '\0';
                p++;
            }
            out->tokens = (char **)realloc(out->tokens, (out->n+1)*sizeof(char *));
            out->tokens[out->n++] = strdup(tmp);
            p++;
        } else {
            int i = 0;
            while (*p && !isspace((unsigned char)*p) && !ispunctchar(*p)) {
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

static int col_index(const Table *t, const char *name) {
    for (int i = 0; i < t->ncols; i++)
        if (strcmp(t->colnames[i], name) == 0) return i;
    return -1;
}

MultiTable *multitable_load(const char *filename) {
    FILE *f = fopen(filename, "r");
    char buf[4096], *p, *tok, *typ;
    int len, blank_line = 1;
    MultiTable *mt;
    Table *cur = NULL;

    if (!f) return (MultiTable *)calloc(1, sizeof(MultiTable));
    mt = (MultiTable *)calloc(1, sizeof(MultiTable));
    if (!mt) { fclose(f); return NULL; }

    while (fgets(buf, sizeof(buf), f)) {
        len = strlen(buf);
        while (len > 0 && (buf[len-1]=='\n' || buf[len-1]=='\r')) buf[--len] = '\0';
        p = buf; while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') { blank_line = 1; continue; }

        if (*p == '"' && !strchr(p, ',') && strchr(p+1, '"') && (!cur || blank_line)) {
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
                cur->coltypes = (int *)realloc(cur->coltypes, (cur->ncols+1)*sizeof(int));
                cur->colnames[cur->ncols] = strdup(tok);
                const char *type_str = typ ? typ : "TEXT";
                int ctype = TYPE_TEXT;
                if (strcasecmp(type_str, "INT") == 0) ctype = TYPE_INT;
                else if (strcasecmp(type_str, "FLOAT") == 0) ctype = TYPE_FLOAT;
                else if (strcasecmp(type_str, "BOOL") == 0) ctype = TYPE_BOOL;
                else if (strncasecmp(type_str, "VARCHAR", 7) == 0) ctype = TYPE_VARCHAR;
                cur->coltypes[cur->ncols] = ctype;
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

static const char *type_to_string(int type) {
    switch (type) {
        case TYPE_INT:    return "INT";
        case TYPE_FLOAT:  return "FLOAT";
        case TYPE_BOOL:   return "BOOL";
        case TYPE_VARCHAR:return "TEXT";
        default:          return "TEXT";
    }
}

int multitable_save(const MultiTable *mt, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    for (int i = 0; i < mt->n; i++) {
        Table *t = mt->tables[i];
        fprintf(f, "\"%s\"\n", t->name);
        for (int j = 0; j < t->ncols; j++) {
            if (j > 0) fputc(',', f);
            fprintf(f, "\"%s:%s\"", t->colnames[j], type_to_string(t->coltypes[j]));
        }
        fputc('\n', f);
        for (int j = 0; j < t->nrows; j++) {
            for (int k = 0; k < t->ncols; k++) {
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

void multitable_free(MultiTable *mt) {
    if (!mt) return;
    for (int i = 0; i < mt->n; i++) table_free(mt->tables[i]);
    free(mt->tables);
    free(mt);
}

static Table *multitable_find(const MultiTable *mt, const char *name) {
    for (int i = 0; i < mt->n; i++)
        if (mt->tables[i]->name && strcmp(mt->tables[i]->name, name) == 0)
            return mt->tables[i];
    return NULL;
}

static int validate_value(const char *val, int type) {
    if (!val) return 1;
    char *endptr;
    switch (type) {
        case TYPE_INT:
            strtol(val, &endptr, 10);
            return *endptr == '\0';
        case TYPE_FLOAT:
            strtod(val, &endptr);
            return *endptr == '\0';
        case TYPE_BOOL:
            return (strcasecmp(val, "true") == 0 || strcasecmp(val, "false") == 0 ||
                    strcmp(val, "0") == 0 || strcmp(val, "1") == 0);
        default:
            return 1;
    }
}

static int compare_values(const char *val1, int type1, const char *val2, int type2) {
    if (type1 == TYPE_INT && type2 == TYPE_INT) {
        long a = atol(val1), b = atol(val2);
        return (a < b) ? -1 : (a > b) ? 1 : 0;
    }
    if ((type1 == TYPE_INT || type1 == TYPE_FLOAT) && (type2 == TYPE_INT || type2 == TYPE_FLOAT)) {
        double a = atof(val1), b = atof(val2);
        return (a < b) ? -1 : (a > b) ? 1 : 0;
    }
    if (type1 == TYPE_BOOL && type2 == TYPE_BOOL) {
        int a = (strcasecmp(val1, "true") == 0 || strcmp(val1, "1") == 0);
        int b = (strcasecmp(val2, "true") == 0 || strcmp(val2, "1") == 0);
        return (a < b) ? -1 : (a > b) ? 1 : 0;
    }
    return strcmp(val1, val2);
}

static int eval_primary(const Table *t, int row, const Tokens *tok, int *pos);
static int eval_expr(const Table *t, int row, const Tokens *tok, int *pos) {
    int left = eval_primary(t, row, tok, pos);
    if (left < 0) return -1;
    while (*pos < tok->n) {
        const char *op = tok->tokens[*pos];
        if (strcasecmp(op, "AND") == 0) {
            (*pos)++;
            int right = eval_primary(t, row, tok, pos);
            if (right < 0) return -1;
            left = left && right;
        } else if (strcasecmp(op, "OR") == 0) {
            (*pos)++;
            int right = eval_primary(t, row, tok, pos);
            if (right < 0) return -1;
            left = left || right;
        } else break;
    }
    return left;
}

static int eval_primary(const Table *t, int row, const Tokens *tok, int *pos) {
    if (*pos >= tok->n) return -1;
    const char *tok_str = tok->tokens[*pos];
    if (strcasecmp(tok_str, "NOT") == 0) {
        (*pos)++;
        int r = eval_primary(t, row, tok, pos);
        if (r < 0) return -1;
        return !r;
    }
    if (strcmp(tok_str, "(") == 0) {
        (*pos)++;
        int r = eval_expr(t, row, tok, pos);
        if (r < 0) return -1;
        if (*pos >= tok->n || strcmp(tok->tokens[*pos], ")") != 0) return -1;
        (*pos)++;
        return r;
    }
    int col = col_index(t, tok_str);
    if (col < 0) return -1;
    (*pos)++;
    if (*pos >= tok->n) return -1;
    const char *op = tok->tokens[*pos];
    int opcode;
    if (strcmp(op, "=") == 0) opcode = 0;
    else if (strcmp(op, "!=") == 0) opcode = 1;
    else if (strcmp(op, ">") == 0) opcode = 2;
    else if (strcmp(op, "<") == 0) opcode = 3;
    else if (strcmp(op, ">=") == 0) opcode = 4;
    else if (strcmp(op, "<=") == 0) opcode = 5;
    else return -1;
    (*pos)++;
    if (*pos >= tok->n) return -1;
    const char *val = tok->tokens[(*pos)++];
    int cmp = compare_values(t->rows[row][col], t->coltypes[col], val, TYPE_TEXT);
    switch (opcode) {
        case 0: return cmp == 0;
        case 1: return cmp != 0;
        case 2: return cmp > 0;
        case 3: return cmp < 0;
        case 4: return cmp >= 0;
        case 5: return cmp <= 0;
    }
    return -1;
}

static int eval_conditions(const Table *t, int row, const Tokens *tok, int *pos) {
    return eval_expr(t, row, tok, pos);
}

static int do_select(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    int star = 0, nsel = 0;
    Table *t;
    char **colnames = NULL;
    int *sel = NULL;

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
        for (int i = 0; i < nsel; i++) free(colnames[i]);
        free(colnames); free(sel); return -1;
    }
    (*pos)++;
    if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
    t = multitable_find(mt, tok->tokens[(*pos)++]);
    if (!t) { fprintf(stderr, "Error: table not found\n"); return -1; }

    if (star) {
        nsel = t->ncols;
        sel = (int *)malloc(nsel * sizeof(int));
        for (int i = 0; i < nsel; i++) sel[i] = i;
    } else {
        sel = (int *)malloc(nsel * sizeof(int));
        for (int i = 0; i < nsel; i++) {
            int idx = col_index(t, colnames[i]);
            if (idx < 0) {
                fprintf(stderr, "Error: unknown column '%s'\n", colnames[i]);
                for (int j = 0; j < nsel; j++) free(colnames[j]);
                free(colnames); free(sel); return -1;
            }
            sel[i] = idx;
        }
        for (int i = 0; i < nsel; i++) free(colnames[i]);
        free(colnames);
    }

    int *matches = NULL;
    if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "WHERE") == 0) {
        (*pos)++;
        matches = (int *)malloc(t->nrows * sizeof(int));
        int nmatch = 0;
        for (int i = 0; i < t->nrows; i++) {
            int pos2 = *pos;
            int r = eval_conditions(t, i, tok, &pos2);
            if (r < 0) { free(matches); free(sel); return -1; }
            if (r) matches[nmatch++] = i;
        }
        for (int i = 0; i < nsel; i++) {
            if (i > 0) fputc(',', out);
            fprintf(out, "%s", t->colnames[sel[i]]);
        }
        fputc('\n', out);
        for (int i = 0; i < nmatch; i++) {
            for (int j = 0; j < nsel; j++) {
                if (j > 0) fputc(',', out);
                fprintf(out, "%s", t->rows[matches[i]][sel[j]]);
            }
            fputc('\n', out);
        }
        free(matches);
    } else {
        for (int i = 0; i < nsel; i++) {
            if (i > 0) fputc(',', out);
            fprintf(out, "%s", t->colnames[sel[i]]);
        }
        fputc('\n', out);
        for (int i = 0; i < t->nrows; i++) {
            for (int j = 0; j < nsel; j++) {
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

static int do_create(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    (void)out;
    int if_not_exists = 0;
    if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "TABLE") != 0) {
        fprintf(stderr, "Error: expected TABLE\n"); return -1;
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
    if (multitable_find(mt, tname)) {
        if (if_not_exists) {
            while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") != 0) (*pos)++;
            if (*pos < tok->n) (*pos)++;
            return 0;
        }
        fprintf(stderr, "Error: table '%s' already exists\n", tname);
        return -1;
    }
    if (*pos >= tok->n || strcmp(tok->tokens[*pos], "(") != 0) {
        fprintf(stderr, "Error: expected (\n"); return -1;
    }
    (*pos)++;
    Table *nt = table_new(tname);
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ")") != 0) {
        if (strcmp(tok->tokens[*pos], ",") == 0) { (*pos)++; continue; }
        char *colname = tok->tokens[(*pos)++];
        char type_buf[128] = {0};
        int type_len = 0;
        while (*pos < tok->n && strcmp(tok->tokens[*pos], ",") != 0 && strcmp(tok->tokens[*pos], ")") != 0) {
            const char *part = tok->tokens[*pos];
            if (type_len > 0) {
                if (type_len + 1 >= (int)sizeof(type_buf)) break;
                type_buf[type_len++] = ' ';
            }
            int part_len = strlen(part);
            if (type_len + part_len >= (int)sizeof(type_buf)) break;
            memcpy(type_buf + type_len, part, part_len + 1);
            type_len += part_len;
            (*pos)++;
        }
        if (type_buf[0] == '\0') {
            fprintf(stderr, "Error: missing column type\n");
            table_free(nt); return -1;
        }
        int actual_type = TYPE_TEXT;
        if (strncasecmp(type_buf, "VARCHAR", 7) == 0) actual_type = TYPE_VARCHAR;
        else if (strcasecmp(type_buf, "INT") == 0) actual_type = TYPE_INT;
        else if (strcasecmp(type_buf, "FLOAT") == 0) actual_type = TYPE_FLOAT;
        else if (strcasecmp(type_buf, "BOOL") == 0) actual_type = TYPE_BOOL;
        nt->colnames = (char **)realloc(nt->colnames, (nt->ncols+1)*sizeof(char *));
        nt->coltypes = (int *)realloc(nt->coltypes, (nt->ncols+1)*sizeof(int));
        nt->colnames[nt->ncols] = strdup(colname);
        nt->coltypes[nt->ncols] = actual_type;
        nt->ncols++;
    }
    if (*pos >= tok->n || strcmp(tok->tokens[*pos], ")") != 0) {
        fprintf(stderr, "Error: expected )\n");
        table_free(nt); return -1;
    }
    (*pos)++;
    if (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
    mt->tables = (Table **)realloc(mt->tables, (mt->n+1)*sizeof(Table *));
    mt->tables[mt->n++] = nt;
    return 0;
}

static int do_drop(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    (void)out;
    int if_exists = 0;
    if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "TABLE") != 0) {
        fprintf(stderr, "Error: expected TABLE\n"); return -1;
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
    int found = 0;
    for (int i = 0; i < mt->n; i++) if (mt->tables[i] == t) { found = i; break; }
    table_free(t);
    memmove(&mt->tables[found], &mt->tables[found+1], (mt->n - found - 1)*sizeof(Table *));
    mt->n--;
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
    return 0;
}

static int do_insert(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    (void)out;
    if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "INTO") != 0) {
        fprintf(stderr, "Error: expected INTO\n"); return -1;
    }
    (*pos)++;
    if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
    Table *t = multitable_find(mt, tok->tokens[(*pos)++]);
    if (!t) { fprintf(stderr, "Error: table not found\n"); return -1; }

    int *col_map = NULL;
    int n_col_map = 0;
    if (*pos < tok->n && strcmp(tok->tokens[*pos], "(") == 0) {
        (*pos)++;
        while (*pos < tok->n && strcmp(tok->tokens[*pos], ")") != 0) {
            if (strcmp(tok->tokens[*pos], ",") == 0) { (*pos)++; continue; }
            int idx = col_index(t, tok->tokens[*pos]);
            if (idx < 0) {
                fprintf(stderr, "Error: unknown column '%s'\n", tok->tokens[*pos]);
                free(col_map); return -1;
            }
            col_map = (int *)realloc(col_map, (n_col_map+1)*sizeof(int));
            col_map[n_col_map++] = idx;
            (*pos)++;
        }
        if (*pos < tok->n && strcmp(tok->tokens[*pos], ")") == 0) (*pos)++;
    }

    if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "VALUES") != 0) {
        fprintf(stderr, "Error: expected VALUES\n");
        free(col_map); return -1;
    }
    (*pos)++;

    int affected = 0;
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") != 0) {
        if (strcmp(tok->tokens[*pos], ",") == 0) { (*pos)++; continue; }
        if (*pos >= tok->n || strcmp(tok->tokens[*pos], "(") != 0) break;
        (*pos)++;

        char **row = (char **)calloc(t->ncols, sizeof(char *));
        int val_idx = 0;
        while (*pos < tok->n && strcmp(tok->tokens[*pos], ")") != 0 && val_idx < (n_col_map ? n_col_map : t->ncols)) {
            char *val = tok->tokens[(*pos)++];
            int col = n_col_map ? col_map[val_idx] : val_idx;
            if (!validate_value(val, t->coltypes[col])) {
                fprintf(stderr, "Error: value '%s' invalid for column type\n", val);
                for (int i = 0; i < t->ncols; i++) free(row[i]);
                free(row); free(col_map); return -1;
            }
            row[col] = strdup(val);
            val_idx++;
            if (*pos < tok->n && strcmp(tok->tokens[*pos], ",") == 0) (*pos)++;
        }
        for (int i = 0; i < t->ncols; i++) if (!row[i]) row[i] = strdup("");
        if (*pos < tok->n && strcmp(tok->tokens[*pos], ")") == 0) (*pos)++;

        t->rows = (char ***)realloc(t->rows, (t->nrows+1)*sizeof(char **));
        t->rows[t->nrows++] = row;
        affected++;
    }
    free(col_map);
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
    return affected;
}

static int do_update(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    (void)out;
    if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "TABLE") == 0) (*pos)++;
    if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
    Table *t = multitable_find(mt, tok->tokens[(*pos)++]);
    if (!t) { fprintf(stderr, "Error: table not found\n"); return -1; }

    if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "SET") != 0) {
        fprintf(stderr, "Error: expected SET\n"); return -1;
    }
    (*pos)++;

    int n_assign = 0;
    int *assign_cols = NULL;
    char **assign_vals = NULL;
    while (*pos < tok->n && strcasecmp(tok->tokens[*pos], "WHERE") != 0 && strcmp(tok->tokens[*pos], ";") != 0) {
        if (strcmp(tok->tokens[*pos], ",") == 0) { (*pos)++; continue; }
        int col = col_index(t, tok->tokens[*pos]);
        if (col < 0) {
            fprintf(stderr, "Error: unknown column '%s'\n", tok->tokens[*pos]);
            free(assign_cols); free(assign_vals); return -1;
        }
        (*pos)++;
        if (*pos >= tok->n || strcmp(tok->tokens[*pos], "=") != 0) {
            fprintf(stderr, "Error: expected '='\n");
            free(assign_cols); free(assign_vals); return -1;
        }
        (*pos)++;
        if (*pos >= tok->n) {
            fprintf(stderr, "Error: missing value\n");
            free(assign_cols); free(assign_vals); return -1;
        }
        char *val = tok->tokens[(*pos)++];
        if (!validate_value(val, t->coltypes[col])) {
            fprintf(stderr, "Error: value '%s' invalid for column type\n", val);
            free(assign_cols); free(assign_vals); return -1;
        }
        assign_cols = (int *)realloc(assign_cols, (n_assign+1)*sizeof(int));
        assign_vals = (char **)realloc(assign_vals, (n_assign+1)*sizeof(char *));
        assign_cols[n_assign] = col;
        assign_vals[n_assign] = strdup(val);
        n_assign++;
    }

    if (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;

    int *matches = NULL;
    int nmatch = 0;
    if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "WHERE") == 0) {
        (*pos)++;
        matches = (int *)malloc(t->nrows * sizeof(int));
        for (int i = 0; i < t->nrows; i++) {
            int pos2 = *pos;
            int r = eval_conditions(t, i, tok, &pos2);
            if (r < 0) {
                free(matches); free(assign_cols); free(assign_vals); return -1;
            }
            if (r) matches[nmatch++] = i;
        }
    } else {
        matches = (int *)malloc(t->nrows * sizeof(int));
        for (int i = 0; i < t->nrows; i++) matches[nmatch++] = i;
    }

    for (int i = 0; i < nmatch; i++) {
        int row = matches[i];
        for (int a = 0; a < n_assign; a++) {
            free(t->rows[row][assign_cols[a]]);
            t->rows[row][assign_cols[a]] = strdup(assign_vals[a]);
        }
    }
    int affected = nmatch;
    free(matches);
    for (int a = 0; a < n_assign; a++) free(assign_vals[a]);
    free(assign_cols);
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
    return affected;
}

static int do_delete(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    (void)out;
    if (*pos >= tok->n || strcasecmp(tok->tokens[*pos], "FROM") != 0) {
        fprintf(stderr, "Error: expected FROM\n"); return -1;
    }
    (*pos)++;
    if (*pos >= tok->n) { fprintf(stderr, "Error: missing table name\n"); return -1; }
    Table *t = multitable_find(mt, tok->tokens[(*pos)++]);
    if (!t) { fprintf(stderr, "Error: table not found\n"); return -1; }

    int affected = 0;
    if (*pos < tok->n && strcasecmp(tok->tokens[*pos], "WHERE") == 0) {
        (*pos)++;
        int keep = 0;
        for (int i = 0; i < t->nrows; i++) {
            int pos2 = *pos;
            int r = eval_conditions(t, i, tok, &pos2);
            if (r < 0) return -1;
            if (!r) {
                if (keep != i) t->rows[keep] = t->rows[i];
                keep++;
            } else {
                for (int j = 0; j < t->ncols; j++) free(t->rows[i][j]);
                free(t->rows[i]);
                affected++;
            }
        }
        t->nrows = keep;
    } else {
        affected = t->nrows;
        for (int i = 0; i < t->nrows; i++) {
            for (int j = 0; j < t->ncols; j++) free(t->rows[i][j]);
            free(t->rows[i]);
        }
        free(t->rows);
        t->rows = NULL;
        t->nrows = 0;
    }
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
    return affected;
}

static int do_create_database(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    (void)mt; (void)out;
    if (*pos >= tok->n) { fprintf(stderr, "Error: missing database name\n"); return -1; }
    const char *dbname = tok->tokens[(*pos)++];
    char filename[512];
    snprintf(filename, sizeof(filename), "%s.db", dbname);
    FILE *f = fopen(filename, "r");
    if (f) { fclose(f); fprintf(stderr, "Error: database '%s' already exists\n", dbname); return -1; }
    f = fopen(filename, "w");
    if (!f) { fprintf(stderr, "Error: cannot create database\n"); return -1; }
    fclose(f);
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
    return 0;
}

static int do_drop_database(MultiTable *mt, const Tokens *tok, int *pos, FILE *out) {
    (void)mt; (void)out;
    if (*pos >= tok->n) { fprintf(stderr, "Error: missing database name\n"); return -1; }
    const char *dbname = tok->tokens[(*pos)++];
    char filename[512];
    snprintf(filename, sizeof(filename), "%s.db", dbname);
    if (remove(filename) != 0) {
        fprintf(stderr, "Error: cannot drop database\n"); return -1;
    }
    while (*pos < tok->n && strcmp(tok->tokens[*pos], ";") == 0) (*pos)++;
    return 0;
}

int execute_sql(MultiTable *mt, const char *sql, FILE *out) {
    Tokens tok;
    int pos = 0;
    if (tokenize(sql, &tok) != 0) {
        fprintf(stderr, "Error: unmatched quote\n");
        return -1;
    }
    if (tok.n == 0) { tokens_free(&tok); return 0; }

    int result = -1;
    const char *first = tok.tokens[0];
    if (strcasecmp(first, "SELECT") == 0) {
        pos = 1;
        result = do_select(mt, &tok, &pos, out);
    } else if (strcasecmp(first, "CREATE") == 0) {
        if (tok.n > 1 && strcasecmp(tok.tokens[1], "DATABASE") == 0) {
            pos = 2;
            result = do_create_database(mt, &tok, &pos, out);
        } else if (tok.n > 1 && strcasecmp(tok.tokens[1], "TABLE") == 0) {
            pos = 1;
            result = do_create(mt, &tok, &pos, out);
        } else {
            fprintf(stderr, "Error: unknown CREATE statement\n");
            result = -1;
        }
    } else if (strcasecmp(first, "DROP") == 0) {
        if (tok.n > 1 && strcasecmp(tok.tokens[1], "DATABASE") == 0) {
            pos = 2;
            result = do_drop_database(mt, &tok, &pos, out);
        } else if (tok.n > 1 && strcasecmp(tok.tokens[1], "TABLE") == 0) {
            pos = 1;
            result = do_drop(mt, &tok, &pos, out);
        } else {
            fprintf(stderr, "Error: unknown DROP statement\n");
            result = -1;
        }
    } else if (strcasecmp(first, "INSERT") == 0) {
        pos = 1;
        result = do_insert(mt, &tok, &pos, out);
    } else if (strcasecmp(first, "DELETE") == 0) {
        pos = 1;
        result = do_delete(mt, &tok, &pos, out);
    } else if (strcasecmp(first, "UPDATE") == 0) {
        pos = 1;
        result = do_update(mt, &tok, &pos, out);
    } else {
        fprintf(stderr, "Error: unknown statement\n");
        result = -1;
    }
    tokens_free(&tok);
    return result;
}
