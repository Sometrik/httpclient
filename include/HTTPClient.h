#ifndef _HTTPCLIENT_H_
#define _HTTPCLIENT_H_

#include "Authorization.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPClientInterface.h"

#include <map>
#include <string>
#include <memory>

namespace httpclient {
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
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      HTTPResponse response;
      request(req, noAuth, response);
      return response;
    }

    HTTPResponse Post(const std::string & uri, const std::string & data, const Authorization & auth) {
      HTTPRequest req(HTTPRequest::POST, uri);
      req.setContent(data);
      req.setContentType("application/x-www-form-urlencoded;charset=UTF-8");
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      HTTPResponse response;
      request(req, auth, response);
      return response;
    }
  
    void Post(const std::string & uri, const std::string & data, const Authorization & auth, HTTPClientInterface & callback) {
      HTTPRequest req(HTTPRequest::POST, uri);
      req.setContent(data);
      req.setContentType("application/x-www-form-urlencoded;charset=UTF-8");
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      request(req, auth, callback);
    }

    void Get(const std::string & uri, const std::string & data, const Authorization & auth, HTTPClientInterface & callback) {
      std::string uri2 = uri;
      uri2 += '?';
      uri2 += data;
      HTTPRequest req(HTTPRequest::GET, uri2);
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      request(req, auth, callback);
    }

    HTTPResponse Get(const std::string & uri, const std::string & data, const Authorization & auth, bool follow_location = true) {
      std::string uri2 = uri;
      uri2 += '?';
      uri2 += data;
      HTTPRequest req(HTTPRequest::GET, uri2);
      req.setFollowLocation(follow_location);
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      HTTPResponse response;
      request(req, auth, response);
      return response;
    }

    HTTPResponse Get(const std::string & uri, const std::string & data, bool follow_location = true) {
      std::string uri2 = uri;
      uri2 += '?';
      uri2 += data;
      HTTPRequest req(HTTPRequest::GET, uri2);
      req.setFollowLocation(follow_location);
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      HTTPResponse response;
      request(req, noAuth, response);
      return response;
    }

    void Get(const std::string & uri, const std::string & data, HTTPClientInterface & callback, bool follow_location = true) {
      std::string uri2 = uri;
      uri2 += '?';
      uri2 += data;
      HTTPRequest req(HTTPRequest::GET, uri2);
      req.setFollowLocation(follow_location);
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      request(req, noAuth, callback);
    }

    HTTPResponse Get(const std::string & uri, bool follow_location = true) {
      HTTPRequest req(HTTPRequest::GET, uri);
      req.setFollowLocation(follow_location);
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      HTTPResponse response;
      request(req, noAuth, response);
      return response;
    }
  
    HTTPResponse Get(const std::string & uri, const Authorization & auth, bool follow_location = true) {
      HTTPRequest req(HTTPRequest::GET, uri);
      req.setFollowLocation(follow_location);
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      HTTPResponse response;
      request(req, auth, response);
      return response;
    }

    void Get(const std::string & uri, HTTPClientInterface & callback, bool follow_location = true) {
      HTTPRequest req(HTTPRequest::GET, uri);
      req.setFollowLocation(follow_location);
      req.setConnectTimeout(getConnectTimeout());
      req.setReadTimeout(getReadTimeout());
      req.setConnectionTimeout(getConnectionTimeout());
      request(req, noAuth, callback);
    }

    void setCookieJar(const std::string & filename) { cookie_jar = filename; }

    void addDefaultHeader(const std::string & name, const std::string & value) {
      default_headers[name] = value;
    }

    virtual void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) = 0;
    virtual void clearCookies() = 0;

    void setConnectTimeout(int t) { connect_timeout = t; }
    void setReadTimeout(int t) { read_timeout = t; }
    void setConnectionTimeout(int t) { connection_timeout = t; }
  
    int getConnectTimeout() const { return connect_timeout; }
    int getReadTimeout() const { return read_timeout; }
    int getConnectionTimeout() const { return connection_timeout; }

    Authorization noAuth;

  protected:
    const std::string & getUserAgent() const { return user_agent; }

    std::string user_agent;
    std::string cookie_jar;
    bool enable_cookies, enable_keepalive;
    std::map<std::string, std::string> default_headers;
    int connect_timeout = 0, read_timeout = 0, connection_timeout = 0;
  };

  class HTTPClientFactory {
  public:
    HTTPClientFactory(const char * default_user_agent = "httpclient") : default_user_agent_(default_user_agent) { }
    virtual ~HTTPClientFactory() { }

    std::unique_ptr<HTTPClient> createClient(std::string user_agent, bool enable_cookies = true, bool enable_keepalive = true) {
      return createClient2(user_agent, enable_cookies, enable_keepalive);
    }

    std::unique_ptr<HTTPClient> createClient(bool enable_cookies = true, bool enable_keepalive = true) {
      return createClient2(getDefaultUserAgent(), enable_cookies, enable_keepalive);
    }

    const std::string & getDefaultUserAgent() const { return default_user_agent_; }

  protected:
    virtual std::unique_ptr<HTTPClient> createClient2(std::string user_agent, bool enable_cookies = true, bool enable_keepalive = true) = 0;

  private:
    std::string default_user_agent_;
  };
};

#endif
