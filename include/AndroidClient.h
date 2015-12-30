#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include "HTTPClient.h"
#include <jni.h>

class AndroidClientFactory : public HTTPClientFactory {
 public:
 AndroidClientFactory(JNIEnv * _env) : env(_env) { }

  std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) override;

 private:
  JNIEnv * env;
};

#endif
