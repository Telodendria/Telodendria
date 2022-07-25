#ifndef TELODENDRIA_BASE64_H
#define TELODENDRIA_BASE64_H

#include <stddef.h>

extern size_t
 Base64EncodedSize(size_t inputSize);

extern size_t
 Base64DecodedSize(const char *base64, size_t len);

extern char *
 Base64Encode(const char *input, size_t len);

extern char *
 Base64Decode(const char *input, size_t len);

extern void
 Base64Unpad(char *base64, size_t length);

extern int
 Base64Pad(char **base64Ptr, size_t length);

#endif
