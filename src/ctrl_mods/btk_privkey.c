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
#include "mods/input.h"
#include "mods/error.h"

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

int btk_privkey_main(int argc, char *argv[])
{
	int o, r;
	size_t i;
	PrivKey key = NULL;
	unsigned char *input;
	size_t input_len;
	char *output;
	unsigned char *uc_output;
	size_t output_len;
	
	// Default flags
	int input_format       = INPUT_GUESS;
	int output_format      = OUTPUT_WIF;
	int output_compression = FALSE;
	int output_newline     = TRUE;
	int output_testnet     = FALSE;
	
	// Process arguments
	while ((o = getopt(argc, argv, "nwhrsdbWHRCUNT")) != -1)
	{
		switch (o)
		{
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
				output_newline = FALSE;
				break;

			// Network Options
			case 'T':
				output_testnet = TRUE;
				break;

			// Unknown option
			case '?':
				if (isprint(optopt))
				{
					fprintf (stderr, "Unknown option '-%c'.\n", optopt);
				}
				else
				{
					fprintf (stderr, "Unknown option character '\\x%x'.\n", optopt);
				}
				return -1;
		}
	}

	// Process testnet option
	if (output_testnet)
	{
		network_set_test();
	}
	
	key = malloc(privkey_sizeof());
	if (key == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	// Process Input
	switch (input_format)
	{
		case INPUT_NEW:
			r = privkey_new(key);
			if (r < 0)
			{
				error_log("Error while generating new private key.");
				return -1;
			}
			break;
		case INPUT_WIF:
			input_len = input_get(&input);

			while (isspace((int)input[input_len - 1]))
			{
				--input_len;
			}

			input = realloc(input, input_len + 1);
			if (input == NULL)
			{
				error_log("Memory allocation error.");
				return -1;
			}

			input[input_len] = '\0';

			r = privkey_from_wif(key, (char *)input);
			if (r < 0)
			{
				error_log("Error while handling input.");
				return -1;
			}

			break;
		case INPUT_HEX:
			input_len = input_get(&input);

			while (isspace((int)input[input_len - 1]))
			{
				--input_len;
			}

			input = realloc(input, input_len + 1);
			if (input == NULL)
			{
				error_log("Memory allocation error.");
				return -1;
			}

			input[input_len] = '\0';

			r = privkey_from_hex(key, (char *)input);
			if (r < 0)
			{
				error_log("Error while handling input.");
				return -1;
			}
			break;
		case INPUT_RAW:
			input_len = input_get_from_pipe(&input);
			if (input == NULL)
			{
				error_log("Piped or redirected input required for raw data.");
				return -1;
			}

			r = privkey_from_raw(key, input, input_len);
			if (r < 0)
			{
				error_log("Error while handling input.");
				return -1;
			}
			break;
		case INPUT_STR:
			input_len = input_get(&input);

			if((int)input[input_len - 1] == '\n')
			{
				--input_len;
			}

			input = realloc(input, input_len + 1);
			if (input == NULL)
			{
				error_log("Memory allocation error.");
				return -1;
			}

			input[input_len] = '\0';

			r = privkey_from_str(key, (char *)input);
			if (r < 0)
			{
				error_log("Error while handling input.");
				return -1;
			}

			break;
		case INPUT_DEC:
			input_len = input_get(&input);

			while (isspace((int)input[input_len - 1]))
			{
				--input_len;
			}

			input = realloc(input, input_len + 1);
			if (input == NULL)
			{
				error_log("Memory allocation error.");
				return -1;
			}

			input[input_len] = '\0';

			r = privkey_from_dec(key, (char *)input);
			if (r < 0)
			{
				error_log("Error while handling input.");
				return -1;
			}
			break;
		case INPUT_BLOB:
			input_len = input_get_from_pipe(&input);
			if (input == NULL)
			{
				error_log("Piped or redirected input required for blob data.");
				return -1;
			}

			r = privkey_from_blob(key, input, input_len);
			if (r < 0)
			{
				error_log("Error while handling input.");
				return -1;
			}
			break;
		case INPUT_GUESS:
			input_len = input_get(&input);

			r = privkey_from_guess(key, input, input_len);
			if (r < 0)
			{
				error_log("Unable to determine input format automatically. Use a command option to specify input format.");
				return -1;
			}

			break;
	}
	
	// Make sure we have a key
	if (!key)
	{
		error_log("Unable to generate key from input.");
		return -1;
	}

	// Don't allow private keys with a zero value
	if (privkey_is_zero(key))
	{
		error_log("Invalid private key. Key value cannot be zero.");
		return -1;
	}

	// Set output compression only if the option is set. Otherwise,
	// compression is based on input.
	switch (output_compression)
	{
		case FALSE:
			break;
		case OUTPUT_COMPRESS:
			privkey_compress(key);
			break;
		case OUTPUT_UNCOMPRESS:
			privkey_uncompress(key);
			break;
	}

	// Write output
	switch (output_format)
	{
		case OUTPUT_WIF:
			output = malloc(PRIVKEY_WIF_LENGTH_MAX + 1);
			if (output == NULL)
			{
				error_log("Memory allocation error.");
				return -1;
			}
			r = privkey_to_wif(output, key);
			if (r < 0)
			{
				error_log("Error while converting key to WIF format.");
				return -1;
			}
			printf("%s", output);
			free(output);
			break;
		case OUTPUT_HEX:
			output = malloc(((PRIVKEY_LENGTH + 1) * 2) + 1);
			if (output == NULL)
			{
				error_log("Memory allocation error.");
				return -1;
			}
			r = privkey_to_hex(output, key);
			if (r < 0)
			{
				error_log("Error while converting key to hex format.");
				return -1;
			}
			printf("%s", output);
			free(output);
			break;
		case OUTPUT_RAW:
			uc_output = malloc(PRIVKEY_LENGTH + 1);
			if (uc_output == NULL)
			{
				error_log("Memory allocation error.");
				return -1;
			}
			r = privkey_to_raw(uc_output, key);
			if (r < 0)
			{
				error_log("Error while converting key to raw format.");
				return -1;
			}
			output_len = (size_t)r;
			for (i = 0; i < output_len; ++i)
			{
				putchar(uc_output[i]);
			}
			free(uc_output);
			break;
	}

	switch (output_newline)
	{
		case TRUE:
			printf("\n");
			break;
	}

	free(key);

	return 1;
}
