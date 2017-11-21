#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include "crypto.h"
#include "message.h"
#include "serialize.h"
#include "mem.h"
#include "assert.h"
#include "messages/version.h"

#define MESSAGE_MAINNET        0xD9B4BEF9
#define MESSAGE_TESTNET        0x0709110B
#define MESSAGE_COMMAND_MAXLEN 12
#define MESSAGE_PAYLOAD_MAXLEN 1024

struct Message {
	uint32_t       magic;
	unsigned char  command[MESSAGE_COMMAND_MAXLEN];
	uint32_t       length;
	uint32_t       checksum;
	unsigned char *payload;
};

Message message_new(const char *c) {
	Message m;
	unsigned char *sha1, *sha2;

	assert(c);

	NEW0(m);

	m->magic = MESSAGE_MAINNET;
	memcpy(m->command, c, strlen(c));

	if (strcmp(c, "version") == 0) {
		m->length = (uint32_t)version_serialize(version_new(), &(m->payload));
	} else {
		m->length = 0;
		m->payload = NULL;
	}
	
	// Getting checksum of payload
	sha1 = crypto_get_sha256(m->payload, m->length);
	sha2 = crypto_get_sha256(sha1, 32);
	m->checksum <<= 8;
	m->checksum += sha2[0];
	m->checksum <<= 8;
	m->checksum += sha2[1];
	m->checksum <<= 8;
	m->checksum += sha2[2];
	m->checksum <<= 8;
	m->checksum += sha2[3];	
	FREE(sha1);
	FREE(sha2);

	return m;
}

// TODO - handle serialization of individual data types in a separate module
size_t message_serialize(Message m, unsigned char **s) {
	unsigned char *head, *ptr;
	
	ptr = head = ALLOC(sizeof(struct Message) + MESSAGE_PAYLOAD_MAXLEN);
	
	// Serializing Message
	ptr = serialize_uint32(ptr, m->magic, SERIALIZE_ENDIAN_LIT);
	ptr = serialize_uchar(ptr, m->command, MESSAGE_COMMAND_MAXLEN);
	ptr = serialize_uint32(ptr, m->length, SERIALIZE_ENDIAN_LIT);
	ptr = serialize_uint32(ptr, m->checksum, SERIALIZE_ENDIAN_BIG);
	ptr = serialize_uchar(ptr, m->payload, m->length);
	
	*s = head;
	
	return 12 + MESSAGE_COMMAND_MAXLEN + m->length;
}

// TODO - handle deserialization of individual data types in a separate module
Message message_from_raw(unsigned char *data, int l) {
	size_t i;
	Message m;

	assert(data);
	assert(l);

	NEW0(m);
	
	// De-Serializing Magic (little endian)
	data = deserialize_uint32(&(m->magic), data, SERIALIZE_ENDIAN_LIT);
	
	// De-Serializing command
	for (i = 0; i < MESSAGE_COMMAND_MAXLEN; ++i) {
		if (data[i])
			m->command[i] = data[i];
		else
			m->command[i] = 0x00;
	}
	data += MESSAGE_COMMAND_MAXLEN;
	
	// De-Serializing Length (little endian)
	data = deserialize_uint32(&(m->length), data, SERIALIZE_ENDIAN_LIT);
	
	// De-Serializing checksum (big endian)
	data = deserialize_uint32(&(m->checksum), data, SERIALIZE_ENDIAN_BIG);
	
	// Getting payload
	m->payload = ALLOC(m->length);
	for (i = 0; i < m->length; ++i) {
		m->payload[i] = *data++;
	}
	
	return m;
}
