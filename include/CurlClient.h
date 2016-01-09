#ifndef _CURLCLIENT_H_
#define _CURLCLIENT_H_

#include "HTTPClient.h"

class CurlClientFactory : public HTTPClientFactory {
 public:
  CurlClientFactory() { }
  CurlClientFactory(const char * _default_user_agent) : HTTPClientFactory(_default_user_agent) { }

  std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies = true, bool _enable_keepalive = true);

  static void globalInit();
  static void globalCleanup();
};

#endif
