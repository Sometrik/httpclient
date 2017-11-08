#ifndef _RATELIMITER_H_
#define _RATELIMITER_H_

#include <string>
#include <unordered_map>
#include <map>
#include <Mutex.h>

struct endpoint_data_s {
  endpoint_data_s() : amount(0), period(0) { }
  endpoint_data_s(unsigned int _amount, unsigned int _period) : amount(_amount), period(_period) { }
  
  unsigned int countHits(time_t t) const {
    unsigned int count = 0;
    for (auto it = hits.begin(); it != hits.end(); it++) {
      if (it->first >= t - period) {
        count += it->second;
      }
    }
    return count;
  }

  void update(time_t t, unsigned int v = 1) {
    hits[t] += v;
    for (auto it = hits.begin(); it != hits.end(); ) {
      if (it->first < t - period) {
        it = hits.erase(it);
      } else {
        break;
      }
    }
  }

  unsigned int amount, period;

  std::map<time_t, unsigned int> hits;
};

class RateLimiter {
 public:
  static RateLimiter & getInstance() {
    if (!instance) {
      instance = new RateLimiter();
    }
    return *instance;
  }

  void addEndpoint(const std::string & url, unsigned int amount, unsigned int period, unsigned int burst_amount, unsigned int burst_period) {
    MutexLocker m(mutex);
    endpoints[url] = std::make_pair(endpoint_data_s(amount, period), endpoint_data_s(burst_amount, burst_period));
  }

  bool tryAndUpdate(const std::string & url, time_t current_time) {
    MutexLocker m(mutex);
    auto it = endpoints.find(url);
    if (it != endpoints.end()) {
      auto & normal = it->second.first;
      auto & burst = it->second.second;
      if (normal.countHits(current_time) < normal.amount) {
        normal.update(current_time);
        return true;
      } else if (burst.countHits(current_time) < burst.amount) {
        burst.update(current_time);
        return true;
      } else {
        return false;
      }
    } else {
      return true;
    }
  }

  std::pair<unsigned int, unsigned int> calcAmounts(const std::string & url, time_t current_time) const {
    MutexLocker m(mutex);
    auto it = endpoints.find(url);
    if (it != endpoints.end()) {
      auto & normal = it->second.first;
      auto & burst = it->second.second;
      return std::make_pair(normal.countHits(current_time), burst.countHits(current_time));
    } else {
      return std::make_pair(0, 0);
    }
  }

  bool isLimited(const std::string & url) const {
    MutexLocker m(mutex);
    return endpoints.find(url) != endpoints.end();
  }

  void fillLimits(const std::string & url, time_t t) {
    MutexLocker m(mutex);
    auto it = endpoints.find(url);
    if (it != endpoints.end()) {
      auto & normal = it->second.first;
      auto & burst = it->second.second;
      normal.update(t, normal.amount);
      burst.update(t, burst.amount);
    }
  }

 private:
  RateLimiter() { }
  RateLimiter(const RateLimiter & other) = delete;
  RateLimiter & operator=(const RateLimiter & other) = delete;

  static RateLimiter * instance;
  
  std::unordered_map<std::string, std::pair<endpoint_data_s, endpoint_data_s> > endpoints;
  mutable Mutex mutex;
};

#endif
