#include <string.h>
#include "serialize.h"
#include "assert.h"

unsigned char *serialize_uint32(unsigned char *dest, uint32_t src, int endian) {
	
	assert(dest);
	assert(endian == SERIALIZE_ENDIAN_BIG || endian == SERIALIZE_ENDIAN_LIT);

	if (endian == SERIALIZE_ENDIAN_LIT) {
		*dest++ = (unsigned char)(src & 0x000000FF);
		*dest++ = (unsigned char)((src & 0x0000FF00) >> 8);
		*dest++ = (unsigned char)((src & 0x00FF0000) >> 16);
		*dest++ = (unsigned char)((src & 0xFF000000) >> 24);
	} else {
		*dest++ = (unsigned char)((src & 0xFF000000) >> 24);
		*dest++ = (unsigned char)((src & 0x00FF0000) >> 16);
		*dest++ = (unsigned char)((src & 0x0000FF00) >> 8);
		*dest++ = (unsigned char)(src & 0x000000FF);
	}
	
	return dest;
}

unsigned char *serialize_uchar(unsigned char *dest, unsigned char *src, int len) {
	
	assert(dest);
	assert(src);
	assert(len);
	
	memcpy(dest, src, len);
	dest += len;
	
	return dest;
}

unsigned char *deserialize_uint32(uint32_t *dest, unsigned char *src, int endian) {
	
	assert(dest);
	assert(src);
	assert(endian == SERIALIZE_ENDIAN_BIG || endian == SERIALIZE_ENDIAN_LIT);
	
	*dest = 0;
	
	if (endian == SERIALIZE_ENDIAN_LIT) {
		*dest += *src++;
		*dest += (*src++ << 8);
		*dest += (*src++ << 16);
		*dest += (*src++ << 24);
	} else {
		*dest += (*src++ << 24);
		*dest += (*src++ << 16);
		*dest += (*src++ << 8);
		*dest += *src++;
	}
	
	return src;
}
