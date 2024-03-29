#ifndef _BASE64_H_
#define _BASE64_H_

#include <string>

namespace httpclient {
  static inline const int MAX_LINE_LENGTH = 76;

  class Base64 {
  public:
    static std::string encode(const std::string & input, const char * indent = 0, int max_line_length = MAX_LINE_LENGTH) { return encode((const unsigned char *)input.data(), input.size(), indent, max_line_length); }
    static std::string encode(const unsigned char * input, size_t length, const char * indent = 0, int max_line_length = MAX_LINE_LENGTH);
    static unsigned long long decode64BitId(const std::string & input);
    static std::string encode64BitId(unsigned long long a);
  };
};

#endif
