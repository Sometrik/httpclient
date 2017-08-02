#ifndef _RATELIMITER_H_
#define _RATELIMITER_H_

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

  std::unordered_map<std::string, endpoint_data_s> endpoints;
};

#endif
