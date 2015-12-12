#ifndef _HTTPCLIENTINTERFACE_H_
#define _HTTPCLIENTINTERFACE_H_

#include <cstddef>

class HTTPClientInterface {
 public:
  virtual ~HTTPClientInterface() { }
  virtual bool handleChunk(size_t len, const char * chunk) = 0;
  virtual bool reconnect() const { return true; }
  virtual bool onIdle(bool is_delayed) { return true; }
};

#endif
