#ifndef _HTTPCLIENT_H_
#define _HTTPCLIENT_H_

#include "Authorization.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPClientInterface.h"

#include <map>
#include <string>
#include <memory>

class HTTPClient {
 public:
  HTTPClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : user_agent(_user_agent),
    enable_cookies(_enable_cookies),
    enable_keepalive(_enable_keepalive)
    { }  
  HTTPClient(const HTTPClient & other) = delete;

  virtual ~HTTPClient() = default;
  
  HTTPClient & operator=(const HTTPClient & other) = delete;
    
  HTTPResponse Post(const std::string & uri, const std::string & data) {
    HTTPRequest req(HTTPRequest::POST, uri);
    req.setContent(data);
    req.setContentType("application/x-www-form-urlencoded;charset=UTF-8");
    HTTPResponse response;
    request(req, noAuth, response);
    return response;
  }

  HTTPResponse Post(const std::string & uri, const std::string & data, const Authorization & auth) {
    HTTPRequest req(HTTPRequest::POST, uri);
    req.setContent(data);
    req.setContentType("application/x-www-form-urlencoded;charset=UTF-8");
    HTTPResponse response;
    request(req, auth, response);
    return response;
  }
  
  void Post(const std::string & uri, const std::string & data, const Authorization & auth, HTTPClientInterface & callback) {
    HTTPRequest req(HTTPRequest::POST, uri);
    req.setContent(data);
    req.setContentType("application/x-www-form-urlencoded;charset=UTF-8");
    request(req, auth, callback);
  }

  HTTPResponse Get(const std::string & uri, const std::string & data, const Authorization & auth, bool follow_location = true, int timeout = 0) {
    std::string uri2 = uri;
    uri2 += '?';
    uri2 += data;
    HTTPRequest req(HTTPRequest::GET, uri2);
    req.setFollowLocation(follow_location);
    req.setTimeout(timeout);
    HTTPResponse response;
    request(req, auth, response);
    return response;
  }

  HTTPResponse Get(const std::string & uri, const std::string & data, bool follow_location = true, int timeout = 0) {
    std::string uri2 = uri;
    uri2 += '?';
    uri2 += data;
    HTTPRequest req(HTTPRequest::GET, uri2);
    req.setFollowLocation(follow_location);
    req.setTimeout(timeout);
    HTTPResponse response;
    request(req, noAuth, response);
    return response;
  }

  void Get(const std::string & uri, const std::string & data, HTTPClientInterface & callback, bool follow_location = true, int timeout = 0) {
    std::string uri2 = uri;
    uri2 += '?';
    uri2 += data;
    HTTPRequest req(HTTPRequest::GET, uri2);
    req.setFollowLocation(follow_location);
    req.setTimeout(timeout);
    request(req, noAuth, callback);
  }

  HTTPResponse Get(const std::string & uri, bool follow_location = true, int timeout = 0) {
    HTTPRequest req(HTTPRequest::GET, uri);
    req.setFollowLocation(follow_location);
    req.setTimeout(timeout);
    HTTPResponse response;
    request(req, noAuth, response);
    return response;
  }
  
  HTTPResponse Get(const std::string & uri, const Authorization & auth, bool follow_location = true, int timeout = 0) {
    HTTPRequest req(HTTPRequest::GET, uri);
    req.setFollowLocation(follow_location);
    req.setTimeout(timeout);
    HTTPResponse response;
    request(req, auth, response);
    return response;
  }

  void Get(const std::string & uri, HTTPClientInterface & callback, bool follow_location = true, int timeout = 0) {
    HTTPRequest req(HTTPRequest::GET, uri);
    req.setFollowLocation(follow_location);
    req.setTimeout(timeout);
    request(req, noAuth, callback);
  }

  void setCookieJar(const std::string & filename) { cookie_jar = filename; }

  void addDefaultHeader(const std::string & name, const std::string & value) {
    default_headers[name] = value;
  }

  virtual void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) = 0;
  virtual void clearCookies() = 0;

  Authorization noAuth;

 protected:
  std::string user_agent;
  std::string cookie_jar;
  bool enable_cookies, enable_keepalive;
  std::map<std::string, std::string> default_headers; 
};

class HTTPClientFactory {
 public:
  HTTPClientFactory(const char * _default_user_agent = "Sometrik/httpclient") : default_user_agent(_default_user_agent) { }
  virtual ~HTTPClientFactory() { }

  virtual std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies = true, bool _enable_keepalive = true) = 0;
  std::shared_ptr<HTTPClient> createClient(bool _enable_cookies = true, bool _enable_keepalive = true) { return createClient(default_user_agent, _enable_cookies, _enable_keepalive); }

  const std::string & getDefaultUserAgent() const { return default_user_agent; }
  
 private:
  std::string default_user_agent;
};

#endif
