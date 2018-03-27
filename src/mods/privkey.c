#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gmp.h>
#include "privkey.h"
#include "network.h"
#include "random.h"
#include "hex.h"
#include "base58.h"
#include "base58check.h"
#include "crypto.h"
#include "mem.h"
#include "assert.h"

#define MAINNET_PREFIX      0x80
#define TESTNET_PREFIX      0xEF

// Private keys can not be larger than (1.158 * 10^77) - 1
#define PRIVKEY_MAX                "100047a327efc14f7fe934ae56989375080f11619ff7157ffffffffffffffffff"
#define PRIVKEY_COMPRESSED_FLAG    0x01
#define PRIVKEY_UNCOMPRESSED_FLAG  0x00

struct PrivKey {
	unsigned char data[PRIVKEY_LENGTH];
	int cflag;
};

PrivKey privkey_new(void) {
	int i;
	PrivKey k;
	mpz_t cur_key, max_key;
	
	NEW(k);

	// Init and set max key size
	mpz_init(max_key);
	mpz_init(cur_key);
	mpz_set_str(max_key, PRIVKEY_MAX, 16);
	
	// Creating private key as hex string
	while (mpz_cmp_ui(cur_key, 1) <= 0 || mpz_cmp(cur_key, max_key) >= 0) {

		for (i = 0; i < PRIVKEY_LENGTH; ++i) {
			k->data[i] = random_get();
		}
		mpz_import(cur_key, PRIVKEY_LENGTH, 1, 1, 1, 0, k->data);
	}
	
	mpz_clear(max_key);
	mpz_clear(cur_key);
	
	k->cflag = PRIVKEY_COMPRESSED_FLAG;
	
	return k;
}

PrivKey privkey_compress(PrivKey k) {
	assert(k);
	k->cflag = PRIVKEY_COMPRESSED_FLAG;
	return k;
}

PrivKey privkey_uncompress(PrivKey k) {
	assert(k);
	k->cflag = PRIVKEY_UNCOMPRESSED_FLAG;
	return k;
}

PrivKey privkey_new_compressed(void) {
	return privkey_compress(privkey_new());;
}

char *privkey_to_hex(PrivKey k) {
	int i;
	char *r;
	
	assert(k);
	
	r = ALLOC(((PRIVKEY_LENGTH + 1) * 2) + 1);
	
	memset(r, 0, ((PRIVKEY_LENGTH + 1) * 2) + 1);
	
	for (i = 0; i < PRIVKEY_LENGTH; ++i) {
		sprintf(r + (i * 2), "%02x", k->data[i]);
	}
	if (k->cflag == PRIVKEY_COMPRESSED_FLAG) {
		sprintf(r + (i * 2), "%02x", k->cflag);
	}
	
	return r;
}

unsigned char *privkey_to_raw(PrivKey k, size_t *l) {
	unsigned char *r;
	assert(k);

	r = ALLOC(PRIVKEY_LENGTH + 1);
	memcpy(r, k->data, PRIVKEY_LENGTH);
	
	if (privkey_is_compressed(k)) {
		r[PRIVKEY_LENGTH] = PRIVKEY_COMPRESSED_FLAG;
		*l = PRIVKEY_LENGTH + 1;
	} else {
		*l = PRIVKEY_LENGTH;
	}

	return r;
}

char *privkey_to_wif(PrivKey k) {
	int l;
	unsigned char p[PRIVKEY_LENGTH + 2];

	if (network_is_main()) {
		p[0] = MAINNET_PREFIX;
	} else if (network_is_test()) {
		p[0] = TESTNET_PREFIX;
	}
	memcpy(p+1, k->data, PRIVKEY_LENGTH);
	if (privkey_is_compressed(k)) {
		p[PRIVKEY_LENGTH+1] = PRIVKEY_COMPRESSED_FLAG;
		l = PRIVKEY_LENGTH + 2;
	} else {
		l = PRIVKEY_LENGTH + 1;
	}
	
	return base58check_encode(p, l);
}

PrivKey privkey_from_wif(char *wif) {
	unsigned char *p;
	PrivKey k;
	size_t l;
	
	NEW(k);

	p = base58check_decode(wif, strlen(wif), &l);
	
	assert(p[0] == MAINNET_PREFIX || p[0] == TESTNET_PREFIX);
	assert(l >= PRIVKEY_LENGTH + 1 && l <= PRIVKEY_LENGTH + 2);

	switch (p[0]) {
		case MAINNET_PREFIX:
			network_set_main();
			break;
		case TESTNET_PREFIX:
			network_set_test();
			break;
	}
	
	memcpy(k->data, p+1, PRIVKEY_LENGTH);
	
	if (l == PRIVKEY_LENGTH + 2) {
		privkey_compress(k);
	} else {
		privkey_uncompress(k);
	}
	
	return k;
}

PrivKey privkey_from_hex(char *hex) {
	size_t i;
	PrivKey k;
	mpz_t cur_key, max_key;
	
	// Validate hex string
	assert(hex);
	assert(strlen(hex) % 2 == 0);
	assert(strlen(hex) >= PRIVKEY_LENGTH * 2);
	for (i = 0; i < strlen(hex); ++i) {
		assert(hex_ischar(hex[i]));
	}
	
	// allocate memory
	NEW(k);

	// load hex string as private key
	for (i = 0; i < PRIVKEY_LENGTH * 2; i += 2) {
		k->data[i/2] = hex_to_dec(hex[i], hex[i+1]);
	}
	if (hex[i] &&  hex[i+1] && hex_to_dec(hex[i], hex[i+1]) == PRIVKEY_COMPRESSED_FLAG) {
		k->cflag = PRIVKEY_COMPRESSED_FLAG;
	} else {
		k->cflag = PRIVKEY_UNCOMPRESSED_FLAG;
	}

	// Make sure key is not above PRIVKEY_MAX
	mpz_init(max_key);
	mpz_init(cur_key);
	mpz_set_str(max_key, PRIVKEY_MAX, 16);
	mpz_import(cur_key, PRIVKEY_LENGTH, 1, 1, 1, 0, k->data);
	assert(mpz_cmp(cur_key, max_key) < 0);
	mpz_clear(max_key);
	mpz_clear(cur_key);
	
	return k;
}

PrivKey privkey_from_str(char *data) {
	PrivKey key;
	unsigned char *tmp;
	
	tmp = crypto_get_sha256((unsigned char*)data, strlen(data));
	key = privkey_from_raw(tmp, 32);
	free(tmp);

	key = privkey_compress(key);
	
	return key;
}

PrivKey privkey_from_dec(char *data) {
	size_t i, c;
	PrivKey key;
	mpz_t d;
	unsigned char *raw;

	mpz_init(d);
	mpz_set_str(d, data, 10);
	i = (mpz_sizeinbase(d, 2) + 7) / 8;
	raw = ALLOC((i < PRIVKEY_LENGTH) ? PRIVKEY_LENGTH : i);
	memset(raw, 0, (i < PRIVKEY_LENGTH) ? PRIVKEY_LENGTH : i);
	mpz_export(raw + PRIVKEY_LENGTH - i, &c, 1, 1, 1, 0, d);
	mpz_clear(d);
	key = privkey_from_raw(raw, PRIVKEY_LENGTH);

	FREE(raw);

	key = privkey_compress(key);
	
	return key;
}

PrivKey privkey_from_raw(unsigned char *raw, size_t l) {
	PrivKey k;
	mpz_t cur_key, max_key;

	// Check params
	assert(raw);
	assert(l >= PRIVKEY_LENGTH);
	
	// allocate memory
	NEW(k);

	// load raw string as private key
	memcpy(k->data, raw, PRIVKEY_LENGTH);
	
	// Set compression flag
	if (l >= PRIVKEY_LENGTH + 1 && raw[PRIVKEY_LENGTH] == PRIVKEY_COMPRESSED_FLAG)
		k->cflag = PRIVKEY_COMPRESSED_FLAG;
	else
		k->cflag = PRIVKEY_UNCOMPRESSED_FLAG;

	// Make sure key is not above PRIVKEY_MAX
	mpz_init(max_key);
	mpz_init(cur_key);
	mpz_set_str(max_key, PRIVKEY_MAX, 16);
	mpz_import(cur_key, PRIVKEY_LENGTH, 1, 1, 1, 0, k->data);
	assert(mpz_cmp(cur_key, max_key) < 0);
	mpz_clear(max_key);
	mpz_clear(cur_key);
	
	return k;
}

PrivKey privkey_from_guess(unsigned char *data, size_t data_len) {
	int i, str_len;
	unsigned char *head = data;
	char *tmp;
	PrivKey key = NULL;

	str_len = (data[data_len-1] == '\n') ? data_len - 1 : data_len;

	// Decimal
	for (data = head, i = 0; i < str_len; ++i)
		if (*data >= '0' && *data <= '9')
			++data;
		else 
			break;
	if (i == str_len) {
		tmp = ALLOC(i + 1);
		memcpy(tmp, head, i);
		tmp[i] = '\0';
		key = privkey_from_dec(tmp);
		FREE(tmp);
		return key;
	}

	// Hex
	if (str_len == PRIVKEY_LENGTH * 2 || str_len == (PRIVKEY_LENGTH + 1) * 2) {
		for (data = head, i = 0; i < str_len; ++i) {
			if (hex_ischar(*data))
				++data;
			else
				break;
		}
		if (i == str_len) {
			tmp = ALLOC(i + 1);
			memcpy(tmp, head, i);
			tmp[i] = '\0';
			key = privkey_from_hex(tmp);
			FREE(tmp);
			return key;
		}
	}

	// WIF
	if (str_len >= PRIVKEY_WIF_LENGTH_MIN && str_len <= PRIVKEY_WIF_LENGTH_MAX) {
		for (data = head, i = 0; i < str_len; ++i) {
			if (base58_ischar(*data))
				++data;
			else
				break;
		}
		if (i == str_len) {
			tmp = ALLOC(i + 1);
			memcpy(tmp, head, i);
			tmp[i] = '\0';
			key = privkey_from_wif(tmp);
			FREE(tmp);
			return key;
		}
	}

	// String
	for (data = head, i = 0; i < str_len; ++i)
		if (*data > 0 && *data < 128)
			++data;
		else
			break;
	if (i == str_len) {
		tmp = ALLOC(i + 1);
		memcpy(tmp, head, i);
		tmp[i] = '\0';
		key = privkey_from_str(tmp);
		FREE(tmp);
		return key;
	}

	// Raw
	data = head;
	if (data_len == PRIVKEY_LENGTH || (data_len == PRIVKEY_LENGTH + 1 && (data[data_len - 1] == PRIVKEY_COMPRESSED_FLAG || data[data_len - 1] == PRIVKEY_UNCOMPRESSED_FLAG))) {
		key = privkey_from_raw(data, data_len);
	}

	return key;
}

int privkey_is_compressed(PrivKey k) {
	return (k->cflag == PRIVKEY_COMPRESSED_FLAG) ? 1 : 0;
}

void privkey_free(PrivKey k) {
	FREE(k);
}
