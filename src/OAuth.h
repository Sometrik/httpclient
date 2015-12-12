#ifndef _OAUTH_H_
#define _OAUTH_H_

#include "Authorization.h"

#include <map>

class OAuth : public Authorization {
 public:
  OAuth(const std::string & _method, const std::string & _base_url, const std::map<std::string, std::string> & _content)
    : http_method(_method), base_url(_base_url), content(_content) { }
  
  std::string createHeader() const;

  void initialize();

  std::string oauth_consumer_key;
  std::string oauth_consumer_secret;
  std::string oauth_token;
  std::string oauth_secret;
  std::string oauth_callback;

 private:
  std::string http_method, base_url;
  std::map<std::string, std::string> content;
  std::string oauth_nonce;
  time_t oauth_timestamp;
  std::string oauth_signature;
};

#endif
