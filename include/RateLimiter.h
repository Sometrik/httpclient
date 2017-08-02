#ifndef _RATELIMITER_H_
#define _RATELIMITER_H_

#include <unordered_map>

struct endpoint_data_s {
  
};

class RateLimiter {
 public:
  static RateLimiter & getInstance() {
    if (!instance) {
      instance = new RateLimiter();
    }
    return *instance;
  }

 private:
  RateLimiter() { }

  static RateLimiter * instance;
  
  std::unordered_map<std::string, endpoint_data_s> endpoints;
};

#endif
