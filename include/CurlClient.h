#ifndef _CURLCLIENT_H_
#define _CURLCLIENT_H_

#include "HTTPClient.h"

#include <curl/curl.h>

class CurlClient : public HTTPClient {
 public:
  CurlClient(const std::string & _interface, const std::string & _user_agent, bool enable_cookies = true, bool enable_keepalive = true);
  CurlClient(const CurlClient & other);
  ~CurlClient();
  CurlClient & operator=(const CurlClient & other);

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth) override;

  void clearCookies() override;

  static void globalInit();
  static void globalCleanup();
  
 protected:
  bool initialize();

 private:
  bool addToData(size_t len, const char * data);
  bool handleProgress(double dltotal, double dlnow, double ultotal, double ulnow);

  static size_t write_data_func(void * buffer, size_t size, size_t nmemb, void *userp);
  static int progress_func(void *  clientp, double dltotal, double dlnow, double ultotal, double ulnow);

  std::string interface_name;
  CURL * curl = 0;
};

class CurlClientFactory {
 public:
  CurlClientFactory() { }
  virtual ~CurlClientFactory() { }

  virtual std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) { return std::make_shared<CurlClient>("", _user_agent, _enable_cookies, _enable_keepalive); }
};

#endif
