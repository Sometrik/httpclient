#ifndef _IOSCLIENT_H_
#define _IOSCLIENT_H_

#include "HTTPClient.h"

class iOSClient;

class iOSClientFactory : public HTTPClientFactory {
 public:
  iOSClientFactory() { }

  std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive);  
};

#endif
