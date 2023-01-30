#ifndef _WINHTTPCLIENT_H_
#define _WINHTTPCLIENT_H_

#include "HTTPClient.h"

namespace httpclient {
  class WinHTTPClientFactory : public HTTPClientFactory {
  public:
    WinHTTPClientFactory() { }
    WinHTTPClientFactory(const char * default_user_agent) : HTTPClientFactory(default_user_agent) { }
    
    std::unique_ptr<HTTPClient> createClient2(std::string user_agent, bool enable_cookies, bool enable_keepalive) override;
    
  };
};

#endif
