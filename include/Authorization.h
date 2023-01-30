#ifndef _AUTHORIZATION_H_
#define _AUTHORIZATION_H_

#include <string>

namespace httpclient {
  class Authorization {
  public:
    Authorization() { }
    virtual ~Authorization() = default;
    virtual const char * getHeaderName() const { return "Authorization"; }
    virtual std::string createHeader() const { return ""; }
    virtual std::string getToken() const { return ""; }
  };
};

#endif
