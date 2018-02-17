#ifndef _IOSCFCLIENT_H_
#define _IOSCFCLIENT_H_

#include "HTTPClient.h"

class iOSCFClientFactory : public HTTPClientFactory {
 public:
  iOSCFClientFactory() { }
  iOSCFClientFactory(const char * _default_user_agent) : HTTPClientFactory(_default_user_agent) { }

  std::unique_ptr<HTTPClient> createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive);  
};

#endif
