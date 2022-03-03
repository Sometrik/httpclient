#ifndef _HTTPCLIENTINTERFACE_H_
#define _HTTPCLIENTINTERFACE_H_

#include <cstddef>
#include <string>

class HTTPClientInterface {
 public:
  virtual ~HTTPClientInterface() = default;
  virtual void handleResultCode(int code) { }
  virtual void handleErrorText(std::string s) { }
  virtual bool handleRedirectUrl(const std::string & url) { return true; }
  virtual void handleHeader(const std::string & key, const std::string & value) { }
  virtual bool handleChunk(size_t len, const char * chunk) = 0;
  virtual void handleDisconnect() { }
  virtual bool onIdle() { return true; }
};

#endif
