#include "Base64.h"

#include <cassert>

static char alphabet1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char alphabet2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
 
using namespace std;

string
Base64::encode(const unsigned char * input, size_t length, const char * indent, int max_line_length) {
  string out;
  int line_len = 0;
  if (indent) out += indent;
  size_t pos = 0, rem = length;
  
  for (; rem >= 3; pos += 3, rem -= 3) {
    unsigned int tmp = (input[pos] << 16) | (input[pos + 1] << 8) | input[pos + 2];
    out += alphabet1[(tmp >> 18)];
    out += alphabet1[(tmp >> 12) & 0x3f];
    out += alphabet1[(tmp >> 6) & 0x3f];
    out += alphabet1[tmp & 0x3f ];
    line_len += 4;
    if (max_line_length && line_len >= max_line_length) {
      out += "\n";
      if (indent) out += indent;
      line_len = 0;
    }
  }
  if (rem == 2) {
    unsigned int tmp = (input[pos] << 16) | (input[pos + 1] << 8);
    out += alphabet1[(tmp >> 18)];
    out += alphabet1[(tmp >> 12) & 0x3f];
    out += alphabet1[(tmp >> 6) & 0x3f];
    out += '=';
  } else if (rem == 1) {
    unsigned int tmp = (input[pos] << 16);
    out += alphabet1[(tmp >> 18)];
    out += alphabet1[(tmp >> 12) & 0x3f];
    out += '=';
    out += '=';
  }
  return out;
}

static inline unsigned char base64_decode_digit(char c) {
  switch (c) {
  case '+': case '-': return 62;
  case '/': case '_': return 63;
  default:
    if (isdigit(c)) return (unsigned char)(c - '0' + 26 + 26);
    else if (islower(c)) return (unsigned char)(c - 'a' + 26);
    else if (isupper(c)) return (unsigned char)(c - 'A');
    else assert(0);
  }
  return 0xff;
}

unsigned long long
Base64::decode64BitId(const std::string & input) {
  unsigned long long n = 0;
  for (unsigned int i = 0; i < input.size(); i++) {
    int c = base64_decode_digit(input[i]);
    n = 64 * n + c;
  }
    
  return n;
}

string
Base64::encode64BitId(unsigned long long a) {
  string s;
  while (a) {
    int b = a & 63;
    a = a >> 6;
    s = alphabet2[b] + s;
  }
  return s;
}
