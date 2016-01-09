#ifndef _IOSCLIENT_H_
#define _IOSCLIENT_H_

#include "HTTPClient.h"

class iOSClient;

class iOSClientFactory : public HTTPClientFactory {
 public:
  iOSClientFactory() { }
 iOSClientFactory(const char * _default_user_agent) : HTTPClientFactory(_default_user_agent) { }

  std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive);  
};

#endif
