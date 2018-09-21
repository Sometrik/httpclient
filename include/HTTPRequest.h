#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <string>
#include <map>

class HTTPRequest {
 public:
  enum RequestType { 
    GET = 1,
    POST,
    PUT,
    DELETE
  };

 HTTPRequest(RequestType _type, const std::string & _uri) : type(_type), uri(_uri) { }

  const char * getTypeString() const {
    switch (type) {
    case GET: return "GET";
    case POST: return "POST";
    case PUT: return "PUT";
    case DELETE: return "DELETE";
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
  const std::map<std::string, std::string> & getHeaders() const { return headers; }
  
  void setContent(const std::string & _content) { content = _content; }
  void setContentType(const std::string & _content_type) { content_type = _content_type; }

  void setFollowLocation(bool f) { follow_location = f; }
  void setConnectTimeout(int t) { connect_timeout = t; }
  void setReadTimeout(int t) { read_timeout = t; }
  void setConnectionTimeout(int t) { connection_timeout = t; }
  
  void addHeader(const char * name, const char * value) {
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
};

#endif
