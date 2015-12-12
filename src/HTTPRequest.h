#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <string>

class HTTPRequest {
 public:
  enum RequestType { 
    GET = 1,
    POST,
    PUT,
    DEL
  };

 HTTPRequest(RequestType _type, const std::string & _uri) : type(_type), uri(_uri) { }

  const RequestType getType() const { return type; }
  const std::string & getURI() const { return uri; }
  const std::string & getData() const { return data; }

 private:
  RequestType type;
  std::string uri;
  std::string data;
};

#endif
