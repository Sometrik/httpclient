#ifndef _AUTHORIZATION_H_
#define _AUTHORIZATION_H_

#include <string>

class Authorization {
 public:
  Authorization() { }
  virtual ~Authorization() { }
  virtual const char * getHeaderName() const { return "Authorization"; }
  virtual std::string createHeader() const { return ""; }

  static Authorization noAuth;
};

class BasicAuth : public Authorization {
 public:
  BasicAuth(const std::string & _username, const std::string & _password) : username(_username), password(_password) { }
  
  const std::string & getUsername() const { return username; }
  const std::string & getPassword() const { return password; }
  
 public:
  std::string username, password;
};

#endif
