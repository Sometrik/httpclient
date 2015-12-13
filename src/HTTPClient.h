#ifndef _HTTPCLIENT_H_
#define _HTTPCLIENT_H_

#include "Authorization.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPClientInterface.h"

#include <map>
#include <string>

class HTTPClient {
 public:
  HTTPClient(const std::string & _interface, const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : interface_name(_interface),
    user_agent(_user_agent),
    enable_cookies(_enable_cookies),
    enable_keepalive(_enable_keepalive)
    { }  
  HTTPClient(const HTTPClient & other)
    : interface_name(other.interface_name),
    user_agent(other.user_agent),
    cookie_jar(other.cookie_jar),
    enable_cookies(other.enable_cookies),
    enable_keepalive(other.enable_keepalive)
      {
      }
  virtual ~HTTPClient() { }
  
  HTTPClient & operator=(const HTTPClient & other) {
    if (this != &other) {
      interface_name = other.interface_name;
      user_agent = other.user_agent;
      cookie_jar = other.cookie_jar;
      enable_cookies = other.enable_cookies;
      enable_keepalive = other.enable_keepalive;
    }
    return *this;
  }
  
  bool Post(const std::string & uri, const std::string & data) {
    return Post(uri, data, Authorization::noAuth);
  }

  bool Get(const std::string & uri, const std::string & data, const Authorization & auth, bool follow_location = true, int timeout = 0) {
    std::string uri2 = uri;
    uri2 += '?';
    uri2 += data;
    return Get(uri2, auth, follow_location, timeout);
  }
  
  bool Get(const std::string & uri, bool follow_location = true, int timeout = 0) {
    return Get(uri, Authorization::noAuth, follow_location, timeout);
  }

  virtual bool Post(const std::string & uri, const std::string & data, const Authorization & auth) = 0;
  virtual bool Get(const std::string & uri, const Authorization & auth, bool follow_location = true, int timeout = 0) = 0;

  void addHeader(const char * name, const char * value) {
    header_map[name] = value;
  }

  bool doRequest(const HTTPRequest & req) {
    if (req.getType() == HTTPRequest::GET) {
      return Get(req.getURI());
    } else {
      return Post(req.getURI(), req.getData());
    }
  }

  const char * getResultData() { return data_in.c_str(); }
  const std::string & getResultString() const { return data_in; }

  const char * getErrorText() { return errortext.c_str(); }
  int getResultCode() const { return result_code; }
  size_t getResultDataLength() const { return data_in.size(); }

  void setCookieJar(const std::string & filename) { cookie_jar = filename; }
  virtual void clearCookies() = 0;

  void setCallback(HTTPClientInterface * _callback) { callback = _callback; }
  
  const std::string & getRedirectUrl() const { return redirect_url; }
  
 protected:
  virtual bool initialize() {
    result_code = 0;
    redirect_url.clear();
    return true;
  }

  std::map<std::string, std::string> header_map;
  std::string data_in, data_out;
  HTTPClientInterface * callback = 0;
  std::string redirect_url;
  std::string interface_name;
  std::string user_agent;
  std::string cookie_jar;
  bool enable_cookies, enable_keepalive;
  int result_code = 0;
  std::string errortext;
};

#endif
