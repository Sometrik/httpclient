#ifndef _IOSCLIENT_H_
#define _IOSCLIENT_H_

#include "HTTPClient.h"

namespace httpclient {
  class iOSClientFactory : public HTTPClientFactory {
  public:
    iOSClientFactory() { }
    iOSClientFactory(const char * _default_user_agent) : HTTPClientFactory(_default_user_agent) { }
    
    std::unique_ptr<HTTPClient> createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive);  
  };
};

#endif
