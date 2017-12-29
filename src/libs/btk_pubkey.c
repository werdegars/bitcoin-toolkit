/*
 * Copyright (c) 2017 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the MIT License. See LICENSE for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "mods/privkey.h"
#include "mods/pubkey.h"
#include "mods/mem.h"
#include "mods/assert.h"

#define INPUT_BUFFER_SIZE    (PUBKEY_UNCOMPRESSED_LENGTH * 2) + 2

/*
 * Static Function Declarations
 */
static int btk_pubkey_read_input(void);

/*
 * Static Variable Declarations
 */
static unsigned char input_buffer[INPUT_BUFFER_SIZE];
static int flag_input_raw = 0;
static int flag_format_newline = 0;

int btk_pubkey_main(int argc, char *argv[]) {
	int o;
	PubKey key = NULL;
	
	// Check arguments
	while ((o = getopt(argc, argv, "rN")) != -1) {
		switch (o) {
			// Input flags
			case 'r':
				flag_input_raw = 1;
				break;

			// Output flags
			

			// Format flags
			case 'N':
				flag_format_newline = 1;
				break;

			case '?':
				if (isprint(optopt))
					fprintf (stderr, "Unknown option '-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character '\\x%x'.\n", optopt);
				return EXIT_FAILURE;
		}
	}
	
	// Process inptu flags
	if (flag_input_raw) {
		int cnt;
		PrivKey priv;
		cnt = btk_pubkey_read_input();
		if (cnt < PRIVKEY_LENGTH) {
			fprintf(stderr, "Error: Invalid input.\n");
			return EXIT_FAILURE;
		}
		priv = privkey_from_raw(input_buffer);
		key = pubkey_get(priv);
		privkey_free(priv);
	}

	// Process format flags
	if (flag_format_newline)
			printf("\n");

	if (key) {
		pubkey_free(key);
	}

	return EXIT_SUCCESS;
}

// TODO - should this be merged with the function in btk_privkey, perhaps it's own module?
static int btk_pubkey_read_input(void) {
	int i, c;

	for (i = 0; i < INPUT_BUFFER_SIZE && (c = getchar()) != EOF; ++i)
		input_buffer[i] = c;

	return i;
}
