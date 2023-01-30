#ifndef _BASE64URL_H_
#define _BASE64URL_H_

#include <string>

namespace httpclient {
  class Base64Url {
  public:
    static bool decode(const char *input, size_t inputLen, void *output, size_t *outputLen);
    static std::string encode(const void *input, size_t inputLen);
    
    static std::string encode(const std::string & input) { return encode(input.data(), input.size()); }
    static long long decode_id(const std::string & s);
    static std::string encode_id(long long id);
    static std::string encode_id_bigendian(long long id);
  };
};

#endif
