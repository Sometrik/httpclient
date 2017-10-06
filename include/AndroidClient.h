#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include <HTTPClient.h>

#include <jni.h>
#include <memory>

class AndroidClientCache;

class AndroidClientFactory : public HTTPClientFactory {
 public:
 AndroidClientFactory() { }
 AndroidClientFactory(const char * _default_user_agent) : HTTPClientFactory(_default_user_agent) { }

  std::unique_ptr<HTTPClient> createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) override;

  static void initialize(JNIEnv * env);

 private:
  static std::shared_ptr<AndroidClientCache> cache;
};

#endif
