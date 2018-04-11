/*
 * Copyright (c) 2017 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the MIT License. See LICENSE for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <gmp.h>
#include <sys/ioctl.h>
#include "mods/privkey.h"
#include "mods/network.h"
#include "mods/base58.h"
#include "mods/crypto.h"
#include "mods/hex.h"
#include "mods/base58check.h"
#include "mods/mem.h"
#include "mods/assert.h"

#define BUFFER_SIZE             1000
#define INPUT_NEW               1
#define INPUT_WIF               2
#define INPUT_HEX               3
#define INPUT_RAW               4
#define INPUT_STR               5
#define INPUT_DEC               6
#define INPUT_BLOB              7
#define INPUT_GUESS             8
#define OUTPUT_WIF              1
#define OUTPUT_HEX              2
#define OUTPUT_RAW              3
#define OUTPUT_COMPRESS         1
#define OUTPUT_UNCOMPRESS       2
#define TRUE                    1
#define FALSE                   0

static size_t btk_privkey_get_input(unsigned char** output);

int btk_privkey_main(int argc, char *argv[], unsigned char *input, size_t input_len) {
	int o;
	size_t i, c;
	PrivKey key = NULL;
	unsigned char *t;
	
	// Default flags
	int input_format       = INPUT_GUESS;
	int output_format      = OUTPUT_WIF;
	int output_compression = FALSE;
	int output_newline     = FALSE;
	int output_testnet     = FALSE;
	
	// Process arguments
	while ((o = getopt(argc, argv, "nwhrsdbWHRCUNT")) != -1) {
		switch (o) {
			// Input format
			case 'n':
				input_format = INPUT_NEW;
				break;
			case 'w':
				input_format = INPUT_WIF;
				break;
			case 'h':
				input_format = INPUT_HEX;
				break;
			case 'r':
				input_format = INPUT_RAW;
				break;
			case 's':
				input_format = INPUT_STR;
				break;
			case 'd':
				input_format = INPUT_DEC;
				break;
			case 'b':
				input_format = INPUT_BLOB;
				break;

			// Output format
			case 'W':
				output_format = OUTPUT_WIF;
				break;
			case 'H':
				output_format = OUTPUT_HEX;
				break;
			case 'R':
				output_format = OUTPUT_RAW;
				break;
			
			// Output Compression
			case 'C':
				output_compression = OUTPUT_COMPRESS;
				break;
			case 'U':
				output_compression = OUTPUT_UNCOMPRESS;
				break;

			// Other options
			case 'N':
				output_newline = TRUE;
				break;

			// Network Options
			case 'T':
				output_testnet = TRUE;
				break;

			// Unknown option
			case '?':
				if (isprint(optopt))
					fprintf (stderr, "Unknown option '-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character '\\x%x'.\n", optopt);
				return EXIT_FAILURE;
		}
	}

	// Process testnet option
	switch (output_testnet) {
		case FALSE:
			break;
		case TRUE:
			network_set_test();
	}
	
	// Process Input
	switch (input_format) {
		case INPUT_NEW:
			key = privkey_new();
			break;
		case INPUT_WIF:
			if (input == NULL)
				input_len = btk_privkey_get_input(&input);

			while (isspace((int)input[input_len - 1]))
				--input_len;

			if (input_len < PRIVKEY_WIF_LENGTH_MIN)
				{
				fprintf(stderr, "Error: Invalid input.\n");
				return EXIT_FAILURE;
				}

			for (i = 0; i < input_len; ++i)
				if (!base58_ischar(input[i]))
					{
					fprintf(stderr, "Error: Invalid input.\n");
					return EXIT_FAILURE;
					}

			if (!base58check_valid_checksum((char *)input, input_len))
				{
				fprintf(stderr, "Error: Invalid input.\n");
				return EXIT_FAILURE;
				}

			RESIZE(input, input_len + 1);
			input[input_len] = '\0';

			key = privkey_from_wif((char *)input);
			break;
		case INPUT_HEX:
			if (input == NULL)
				input_len = btk_privkey_get_input(&input);

			while (isspace((int)input[input_len - 1]))
				--input_len;

			if (input_len != PRIVKEY_LENGTH * 2 && input_len != (PRIVKEY_LENGTH + 1) * 2)
				{
				fprintf(stderr, "Error: Invalid input.\n");
				return EXIT_FAILURE;
				}

			for (i = 0; i < input_len; ++i)
				if (!hex_ischar(input[i]))
					{
					fprintf(stderr, "Error: Invalid input.\n");
					return EXIT_FAILURE;
					}

			RESIZE(input, input_len + 1);
			input[input_len] = '\0';

			key = privkey_from_hex((char *)input);
			break;
		case INPUT_RAW:
			if (input == NULL)
				{
				fprintf(stderr, "Error: Input required.\n");
				return EXIT_FAILURE;
				}

			if (input_len != PRIVKEY_LENGTH && input_len != PRIVKEY_LENGTH + 1)
				{
				fprintf(stderr, "Error: Invalid input.\n");
				return EXIT_FAILURE;
				}

			key = privkey_from_raw(input, input_len);
			break;
		case INPUT_STR:
			if (input == NULL)
				input_len = btk_privkey_get_input(&input);

			if((int)input[input_len - 1] == '\n')
				--input_len;

			RESIZE(input, input_len + 1);
			input[input_len] = '\0';

			key = privkey_from_str((char *)input);
			break;
		case INPUT_DEC:
			if (input == NULL)
				input_len = btk_privkey_get_input(&input);

			while (isspace((int)input[input_len - 1]))
				--input_len;

			for (i = 0; i < input_len; ++i)
				if (input[i] < '0' || input[i] > '9')
					{
					fprintf(stderr, "Error: Invalid input.\n");
					return EXIT_FAILURE;
					}

			RESIZE(input, input_len + 1);
			input[input_len] = '\0';

			key = privkey_from_dec((char *)input);
			break;
		case INPUT_BLOB:
			if (input == NULL)
				{
				fprintf(stderr, "Error: Input required.\n");
				return EXIT_FAILURE;
				}
			key = privkey_from_blob(input, input_len);
			break;
		case INPUT_GUESS:
			if (input != NULL)
				{
				key = privkey_from_guess(input, input_len);
				if (key == NULL)
					{
					fprintf(stderr, "Error: Unable to interpret input automatically. Use an input switch to specify how this input should be interpreted.\n");
					return EXIT_FAILURE;
					}
				}
			else
				key = privkey_new();

			break;
	}
	
	// Make sure we have a key
	assert(key);

	// Don't allow private keys with a zero value
	if (privkey_is_zero(key)) {
		fprintf(stderr, "Error: Private key can not be zero.\n");
		return EXIT_FAILURE;
	}
	
	// Set output compression only if the option is set. Otherwise,
	// compression is based on input.
	switch (output_compression) {
		case FALSE:
			break;
		case OUTPUT_COMPRESS:
			key = privkey_compress(key);
			break;
		case OUTPUT_UNCOMPRESS:
			key = privkey_uncompress(key);
			break;
	}
	
	// Write output
	switch (output_format) {
		case OUTPUT_WIF:
			printf("%s", privkey_to_wif(key));
			break;
		case OUTPUT_HEX:
			printf("%s", privkey_to_hex(key));
			break;
		case OUTPUT_RAW:
			t = privkey_to_raw(key, &c);
			for (i = 0; i < c; ++i) {
				putchar(t[i]);
			}
			free(t);
			break;
	}
	// Process format flags
	switch (output_newline) {
		case TRUE:
			printf("\n");
			break;
	}

	// Free key
	privkey_free(key);

	// Return
	return EXIT_SUCCESS;
}

static size_t btk_privkey_get_input(unsigned char** output) {
	size_t i, size, increment = 100;
	int o;

	size = increment;

	*output = ALLOC(size);

	for (i = 0; (o = getchar()) != '\n'; ++i)
		{
		if (i == size)
			{
			size += increment;
			RESIZE(*output, size);
			}
		(*output)[i] = (unsigned char)o;
		}

	return i;
}