#ifndef DATABASE_H
#define DATABASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct {
	char *name;
	char **colnames;
	char **coltypes;
	int  ncols;
	char ***rows;
	int  nrows;
} Table;

typedef struct {
	Table **tables;
	int    n;
} MultiTable;

MultiTable *multitable_load(const char *filename);
int         multitable_save(const MultiTable *mt, const char *filename);
void        multitable_free(MultiTable *mt);
void        execute_sql(MultiTable *mt, const char *sql, FILE *out);

#ifdef __cplusplus
}
#endif
#endif