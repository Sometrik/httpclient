#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <string>
#include <map>

namespace httpclient {
  class HTTPRequest {
  public:
    enum RequestType { 
      GET = 1,
      POST,
      // PUT,
      // DELETE,
      OPTIONS
    };

    HTTPRequest(RequestType _type, const std::string & _uri) : type(_type), uri(_uri) { }

    const char * getTypeString() const {
      switch (type) {
      case GET: return "GET";
      case POST: return "POST";
	// case PUT: return "PUT";
	// case DELETE: return "DELETE";
      case OPTIONS: return "OPTIONS";
      }
      return "";
    }
  
    const RequestType getType() const { return type; }
    const std::string & getURI() const { return uri; }
    const std::string & getContent() const { return content; }
    const std::string & getContentType() const { return content_type; }
    bool getFollowLocation() const { return follow_location; }
    int getConnectTimeout() const { return connect_timeout; }
    int getReadTimeout() const { return read_timeout; }
    int getConnectionTimeout() const { return connection_timeout; }
    bool useHTTP2() const { return use_http2; }
    const std::map<std::string, std::string> & getHeaders() const { return headers; }
  
    void setContent(const std::string & _content) { content = _content; }
    void setContentType(const std::string & _content_type) { content_type = _content_type; }

    void setFollowLocation(bool f) { follow_location = f; }
    void setConnectTimeout(int t) { connect_timeout = t; }
    void setReadTimeout(int t) { read_timeout = t; }
    void setConnectionTimeout(int t) { connection_timeout = t; }
    void setUseHTTP2(bool t) { use_http2 = t; }

    void setHeaders(const std::map<std::string, std::string> & _headers) { headers = _headers; }
    void addHeader(const std::string & name, const std::string & value) {
      headers[name] = value;
    }

  private:
    RequestType type;
    std::string uri;
    std::string content;
    std::string content_type;
    std::map<std::string, std::string> headers; 
    bool follow_location = true;
    int connect_timeout = 0, read_timeout = 0, connection_timeout = 0;
    bool use_http2 = false;
  };
};

#endif
