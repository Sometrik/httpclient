#ifndef _WINHTTPCLIENT_H_
#define _WINHTTPCLIENT_H_

#include "HTTPClient.h"

class WinHTTPClientFactory : public HTTPClientFactory {
 public:
  WinHTTPClientFactory() { }
  WinHTTPClientFactory(const char * _default_user_agent) : HTTPClientFactory(_default_user_agent) { }

  std::unique_ptr<HTTPClient> createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) override;

#if 0
  static void globalInit();
  static void globalCleanup();
#endif
};

#endif
