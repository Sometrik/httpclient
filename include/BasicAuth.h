#ifndef _BASICAUTH_H_
#define _BASICAUTH_H_

#include <Authorization.h>
#include <Base64.h>

class BasicAuth : public Authorization {
 public:
  BasicAuth(const std::string & _username, const std::string & _password) : username(_username), password(_password) { }
  
  const std::string & getUsername() const { return username; }
  const std::string & getPassword() const { return password; }
  
  std::string createHeader() const {
    std::string userpassword = username + ":" + password;
    std::string encodedAuth = Base64::encode(userpassword, 0, 1000);
    return "Basic " + encodedAuth;
  }
  
 public:
  std::string username, password;
};

#endif
