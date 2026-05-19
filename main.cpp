#include <stdlib.h>
#include <stdio.h>
#include "database.hpp"

int
main(int argc, char *argv[])
{
	MultiTable *mt;
	const char *infile, *outfile = NULL;

	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: %s <input_csv> <sql> [output_csv]\n", argv[0]);
		return 1;
	}
	infile = argv[1];
	mt = multitable_load(infile);
	if (!mt) {
		fprintf(stderr, "Failed to load '%s'\n", infile);
		return 1;
	}
	execute_sql(mt, argv[2], stdout);
	if (argc == 4) {
		outfile = argv[3];
		if (multitable_save(mt, outfile) != 0) {
			fprintf(stderr, "Failed to save to '%s'\n", outfile);
			multitable_free(mt);
			return 1;
		}
	}
	multitable_free(mt);
	return 0;
}