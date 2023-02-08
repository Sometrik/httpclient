#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <string>
#include <map>

namespace httpclient {
  enum class Method {
    GET = 1,
    POST,
    // PUT,
    // DELETE,
    OPTIONS
  };

  // string values may not be altered since they are used in the implementations
  static inline std::string to_string(Method method) {
    switch (method) {
    case Method::GET: return "GET";
    case Method::POST: return "POST";
    case Method::OPTIONS: return "OPTIONS";
    }
    return "";
  }

  class HTTPRequest {
  public:
    HTTPRequest(Method method, std::string uri) : method_(method), uri_(std::move(uri)) { }
  
    const Method getMethod() const { return method_; }
    const std::string & getURI() const { return uri_; }
    const std::string & getContent() const { return content_; }
    const std::string & getContentType() const { return content_type_; }
    bool getFollowLocation() const { return follow_location_; }
    int getConnectTimeout() const { return connect_timeout_; }
    int getReadTimeout() const { return read_timeout_; }
    int getConnectionTimeout() const { return connection_timeout_; }
    bool useHTTP2() const { return use_http2_; }
    const std::map<std::string, std::string> & getHeaders() const { return headers_; }
  
    void setContent(std::string content) { content_ = std::move(content); }
    void setContentType(std::string content_type) { content_type_ = std::move(content_type); }

    void setFollowLocation(bool f) { follow_location_ = f; }
    void setConnectTimeout(int t) { connect_timeout_ = t; }
    void setReadTimeout(int t) { read_timeout_ = t; }
    void setConnectionTimeout(int t) { connection_timeout_ = t; }
    void setUseHTTP2(bool t) { use_http2_ = t; }

    void setHeaders(std::map<std::string, std::string> headers) { headers_ = std::move(headers); }
    void addHeader(const std::string & name, const std::string & value) {
      headers_[name] = value;
    }

  private:
    Method method_;
    std::string uri_;
    std::string content_;
    std::string content_type_;
    std::map<std::string, std::string> headers_; 
    bool follow_location_ = true;
    int connect_timeout_ = 0, read_timeout_ = 0, connection_timeout_ = 0;
    bool use_http2_ = false;
  };
};

#endif
