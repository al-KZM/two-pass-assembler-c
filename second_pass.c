/*
 * Second pass of the assembling, encode every line to the object (.ob) file.
 * Every code instruction (normal command) line is directly dumped into the object file in the right format.
 * Every .entry instruction is directly dumped into an entries file (.ent)
 * Every use of an external label is dumped into a temporary externals file (.ext) that is renamed after
 * all the lines are parsed.
 * Data instruction are first encoded to a temporary file as raw bits. After reading the whole input file,
 * this temporary file will be parsed and converted to the right format in order to merge it to the object
 * file.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "encoder.h"
#include "second_pass.h"
#include "instructions.h"
#include "labels.h"
#include "utils.h"
#include "globals.h"

void second_pass(char *fname, LabelsTable *labels_table_ptr, int ic_size, int dc_size){
	FILE *fp, *obj_fp;
    char *line_ptr; /* Hold the line strin */
    size_t line_len; /* Maximum length of an instruction line */
    size_t read_cnt; /* Counter of characters read from the file */

    char *label; /* Hold the label name */

    int ic; /* Instruction counter & Data counter */

	char *file_basename; /* Base name of the processed file */
    char *main_of; /* main output file */
    char *entries_of; /* entries output file */
    char *external_of; /* externals output file */

    BITMAP_32 *bitmap; /* 32-Bits array */

	int dc_offset = ic_size;

    /* Init variables */
    ic = 100;
	line_len = LINE_MAX_SIZE;
    line_ptr = (char *) calloc(line_len, sizeof(char));

	/* Check if the file is valid */
	fp = fopen(fname, "r");
	if (!fp){
		printf("Bad file: %s\n", fname);
        return;
    }

    /* Create output files */
	file_basename = get_basename(fname);

    /* Main output file is file basename with .ob at the end */
    main_of = (char *) calloc(strlen(file_basename)+4, sizeof(char));
    strcpy(main_of, file_basename);
    strcat(main_of, ".ob");

    /* Entries output file is file basename with .ent at the end */
    entries_of = (char *) calloc(strlen(file_basename)+5, sizeof(char));
    strcpy(entries_of, file_basename);
    strcat(entries_of, ".ent");

    /* External output file is file basename with .ext at the end */
    external_of = (char *) calloc(strlen(file_basename)+5, sizeof(char));
    strcpy(external_of, file_basename);
    strcat(external_of, ".ext");

    /* Create the files */
	obj_fp = fopen(main_of, "w"); /* Object file */
	fprintf(obj_fp, "%d %d\n", ic_size, dc_size); /* Write title to the object file */
	fclose(obj_fp);
    fopen(entries_of, "w"); /* Entries file */
    fopen(external_of, "w"); /* Externals file */
    create_tmp_files(); /* Temporary files */

	while ((read_cnt = get_line_wout_spaces(&line_ptr, &line_len, fp)) != -1) {

		/* We assume no one modified the input file between the first and the second pass */
		/* Thus we're not checking syntax errors again */

        /* Clean the line (remove unwanted characters) */
        line_ptr = clean_str(line_ptr);

        /* Ignore every irrelevant line (comments/empty...) */
        if (!relevant_line(line_ptr))
            continue;

		/* If it's an external instruction, ignore it */
		if (is_external_instruction(line_ptr))
			continue;

        /* If it's an entry instruction - mark the symbol as entry */
        if (is_entry_instruction(line_ptr)){
            label = get_entry_label(line_ptr);
            mark_label_as_entry(labels_table_ptr, label);
            continue;
        }

        /* If the line is a labelled line, skip the label */
        if (contain_label(line_ptr)){
            /* Skip the label itself */
            while (*line_ptr++ != ':'){}
            /* Skip the whitespaces after the label */
            while ( isspace(*line_ptr) )
                line_ptr++;
        }

        /* If it's a data instruction, add the data to the memory */
        if (is_data_instruction(line_ptr)){
            tmp_dump_data_instruction(line_ptr);
            continue;
        }

        /* === If we got here, then it's a code instruction === */

        /* Encode the line to binary */
        bitmap = encode_instruction_line(line_ptr, labels_table_ptr, ic);

        /* Add the bitmap to the file */
        dump_bitmap(bitmap, main_of, ic, 4);

        /* Increment instruction counter */
        ic += 4;
    }

    /* Merge the temporary data file to the output file */
    merge_tmp_data_file(main_of, dc_offset);

    /* Create entries file */
    dump_entry_labels(labels_table_ptr, entries_of);

    /* Create externals file */
    rename_externals_file(external_of);

    /* Close input file and delete temporary ones */
	fclose(fp);
    delete_tmp_files();
}
