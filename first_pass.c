#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "instructions.h"
#include "utils.h"
#include "globals.h"
#include "linked_list.h"


/*
 * Boolean flags
 */
typedef struct Flags{
    unsigned int has_label : 1;
    unsigned int is_instruction : 1;
} Flags;


/*
 * Return True if a string contains a label
 */
bool contain_label(char *s){
    while (*s){
        if (*s++ == ':')
            return true;
    }
    return false;
}

/*
 * Return True if a line is relevant, meaning it's not a comment or empty
 */
bool relevant_line(char *s){

    s = trim_whitespaces(s);

    if (*s == '\n')
        return false;

    if (*s == COMMENT_CHAR)
        return false;

    return true;
}

/*
 * Perform first pass on a file
 */
void first_pass(char *fname){
    /* TODO CREATE labels_table */
    Flags flags; /*  */
    int ic, dc; /* Instruction counter, Data counter */
	FILE *fp; /* File pointer */
	char *line_ptr; /* Line holder */
	size_t line_len; /* Max Length of a line in a file */
	ssize_t read_cnt; /* Number of character retrieved on a line */
    char *label, *instruction, *var_name; /* Store temporary strings */

	struct Node *labels_table_ptr;

	/* Init variables */
    ic = 100;
    dc = 0;
    line_ptr = (char *) calloc(LINE_MAX_SIZE, sizeof(char));
	line_len = LINE_MAX_SIZE;

	/* Open file */
	fp = fopen(fname, "r");

	/* Check if the file is valid */
	if (!fp){
		printf("Bad file: %s\n", fname);
		return;
	}

	/* Loop - Read lines and process them */
	while ((read_cnt = getline(&line_ptr, &line_len, fp)) != -1) {
        printf(line_ptr);

        /* Reinitialize flags */
        flags.has_label = flags.is_instruction = 0;

        /* Remove every leading whitespaces */
        line_ptr = trim_whitespaces(line_ptr);

        /* Ignore every irrelevant line (comments/empty...) */
        if (!relevant_line(line_ptr))
            continue;

        /* Handle labelled command */
        if (contain_label(line_ptr)){
            flags.has_label = 1;
            label = get_label(line_ptr); /* TODO */
        }

        /* Handle instruction (data storage) command */
        if (is_instruction(line_ptr)){
            flags.is_instruction = 1;
            instruction = get_instruction(line_ptr);

            /* If the line contains a label, add it to the labels table */
            if (flags.has_label){
                label_data_instruction(labels_table, dc, label); /* TODO */
            }

            /* Update DC */
            dc += get_required_cells(instruction);

            /* Parse next line */
            continue;
        }


        /* If it's a .entry instruction, stop first pass here */
        if (is_entry_instruction(line_ptr)) /* TODO */
            continue;

        /* Handle extern instruction */
        if (is_extern_instruction(line_ptr)){

            /* Extract the variable name from the line */
            var_name = parse_external_var_name(line_ptr); /* TODO */

            /* Add this instruction as an external label */
            add_external_variable(labels_table, var_name); /* TODO */
            continue;
        }

        /* If we get here, it's an code instruction */
        if (flags.has_label) /* If there is a label, add it */
            label_code_instruction(labels_table, ic, label); /* TODO */

        ic += 4;
    }

    /* Because we want to put every data definition at the end
     * of the binary output file, add IC to every labelled data
     */
    add_data_offset(labels_table, ic); /* TODO */

	/* Close the file */
	fclose(fp);
}
