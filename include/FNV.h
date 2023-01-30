#ifndef _FNV_H_
#define _FNV_H_

#include <string>

namespace httpclient {
  class FNV {
  public:
    static long long calcFNV1a_64(const void * ptr, size_t len);
    static long long calcFNV1a_64(const std::string & s) { return calcFNV1a_64(s.data(), s.size()); }
    
    static int calcFNV1a_32(const void * ptr, size_t len);
    static int calcFNV1a_32(const std::string & s) { return calcFNV1a_32(s.data(), s.size()); }
  };
};

#endif
