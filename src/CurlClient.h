#ifndef _CURLCLIENT_H_
#define _CURLCLIENT_H_

#include "HTTPClient.h"

#include <curl/curl.h>

class CurlClient : public HTTPClient {
 public:
  // CurlClient();
  CurlClient(const std::string & _interface, const std::string & _user_agent, bool enable_cookies = true, bool enable_keepalive = true);
  CurlClient(const CurlClient & other);
  ~CurlClient();
  CurlClient & operator=(const CurlClient & other);

  bool Post(const std::string & uri, const std::string & data, const Authorization & auth) override;
  bool Get(const std::string & uri, const Authorization & auth, bool follow_location = true, int timeout = 0) override;

  void clearCookies() override;

  static void globalInit();
  static void globalCleanup();
  
 protected:
  bool initialize() override;

 private:
  bool addToData(size_t len, const char * data);
  bool handleProgress(double dltotal, double dlnow, double ultotal, double ulnow);

  static size_t write_data_func(void * buffer, size_t size, size_t nmemb, void *userp);
  static int progress_func(void *  clientp, double dltotal, double dlnow, double ultotal, double ulnow);

  CURL * curl = 0;
};

#endif
