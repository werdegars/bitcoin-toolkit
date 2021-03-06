/*
 * Copyright (c) 2017 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <string.h>
#include <stdint.h>
#include <gcrypt.h>
#include <assert.h>
#include "crypto.h"
#include "error.h"

static int crypto_init(void)
{
	static int isInit = 0;

	if (!isInit)
	{
		if (!gcry_check_version(GCRYPT_VERSION))
		{
			error_log("Libgcrypt version mismatch.");
			return -1;
		}
		gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
		gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);
		gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
		gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

		isInit = 1;
	}

	return 1;
}

int crypto_get_sha256(unsigned char *output, unsigned char *input, size_t input_len)
{
	gcry_md_hd_t gc;
	
	assert(output);
	assert(input);
	assert(input_len);

	if (crypto_init() < 0)
	{
		error_log("Could not initialize encryption library.");
		return -1;
	}
	
	gcry_md_open(&gc, GCRY_MD_SHA256, 0);
	
	gcry_md_write(gc, input, input_len);
	
	memcpy(output, gcry_md_read(gc, 0), 32);
	
	gcry_md_close(gc);
	
	return 1;
}

int crypto_get_rmd160(unsigned char *output, unsigned char *input, size_t input_len)
{
	gcry_md_hd_t gc;
	
	assert(output);
	assert(input);
	assert(input_len);
	
	if (crypto_init() < 0)
	{
		error_log("Could not initialize encryption library.");
		return -1;
	}

	gcry_md_open(&gc, GCRY_MD_RMD160, 0);

	gcry_md_write(gc, input, input_len);

	memcpy(output, gcry_md_read(gc, 0), 20);

	gcry_md_close(gc);

	return 1;
}

int crypto_get_checksum(uint32_t *output, unsigned char *data, size_t len)
{
	int r;
	unsigned char *sha1, *sha2;

	assert(output);
	assert(data);
	assert(len);

	sha1 = malloc(32);
	if (sha1 == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}
	sha2 = malloc(32);
	if (sha2 == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	r = crypto_get_sha256(sha1, data, len);
	if (r < 0)
	{
		error_log("Could not generate SHA256 hash for input.");
		return -1;
	}
	r = crypto_get_sha256(sha2, sha1, 32);
	if (r < 0)
	{
		error_log("Could not generate SHA256 hash for input.");
		return -1;
	}

	*output <<= 8;
	*output += sha2[0];
	*output <<= 8;
	*output += sha2[1];
	*output <<= 8;
	*output += sha2[2];
	*output <<= 8;
	*output += sha2[3];
	
	free(sha1);
	free(sha2);
	
	return 1;
}
