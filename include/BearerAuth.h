#ifndef _BEARERAUTH_H_
#define _BEARERAUTH_H_

#include "Authorization.h"

namespace httpclient {
  class BearerAuth : public Authorization {
  public:
    BearerAuth(const std::string & _data) : data(_data) { }
    
    std::string createHeader() const { 
      std::string s = "Bearer ";
      s += data;
      return s;
    }
    
  private:
    std::string data;
  };
};

#endif
