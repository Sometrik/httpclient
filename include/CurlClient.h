#ifndef _CURLCLIENT_H_
#define _CURLCLIENT_H_

#include "HTTPClient.h"

class CurlClientFactory : public HTTPClientFactory {
 public:
  CurlClientFactory() { }
  CurlClientFactory(const char * default_user_agent) : HTTPClientFactory(default_user_agent) { }

  std::unique_ptr<HTTPClient> createClient2(std::string user_agent, bool enable_cookies, bool enable_keepalive) override;

  static void globalInit();
  static void globalCleanup();
};

#endif
