#ifndef VERSION_H
#define VERSION_H 1

#include <stddef.h>

#define VERSION_COMMAND "version"

typedef struct Version *Version;

int version_new(Version);
int version_serialize(unsigned char *, Version);
int version_new_serialize(unsigned char *);
int version_deserialize(Version, unsigned char *, size_t);
int version_to_json(char *, Version);
size_t version_sizeof(void);

#endif
