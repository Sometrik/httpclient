#ifndef _HTTPCLIENTINTERFACE_H_
#define _HTTPCLIENTINTERFACE_H_

#include <cstddef>
#include <string>

class HTTPClientInterface {
 public:
  virtual ~HTTPClientInterface() { }
  virtual void handleResultCode(int code) { }
  virtual void handleRedirectUrl(const std::string & url) { }
  virtual void handleHeader(const std::string & key, const std::string & value) { }
  virtual bool handleChunk(size_t len, const char * chunk) = 0;
  virtual bool reconnect() const { return true; }
  virtual bool onIdle(bool is_delayed) { return true; }
};

#endif
