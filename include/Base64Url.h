#ifndef _BASE64URL_H_
#define _BASE64URL_H_

#include <string>

class Base64Url {
 public:
  static bool decode(const char *input, size_t inputLen, void *output, size_t *outputLen);
  static void encode(const void *input, size_t inputLen, char *output, size_t *outputLen);

  static long long decode_id(const std::string & s);
  static std::string encode_id(long long id);
};

#endif
