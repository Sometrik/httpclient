#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include "HTTPClient.h"

#include <memory>
#include <jni.h>

class AndroidClientCache {
 public:
  AndroidClientCache(JNIEnv * _env);

  JNIEnv * getJNIEnv() { return env; }
  
  jclass cookieManagerClass;
  jmethodID clearCookiesMethod;
  jclass bitmapClass;
  jclass factoryClass;
  jclass httpClass;
  jclass urlClass;
  jclass bufferedReaderClass;
  jclass inputStreamReaderClass;
  jclass inputStreamClass;
  jmethodID urlConstructor;
  jmethodID openConnectionMethod;
  jmethodID setRequestProperty;
  jmethodID setRequestMethod;
  jmethodID setDoInputMethod;
  jmethodID connectMethod;
  jmethodID getResponseCodeMethod;
  jmethodID getResponseMessageMethod;
  jmethodID setRequestPropertyMethod;
  jmethodID outputStreamConstructor;
  jmethodID factoryDecodeMethod;
  jmethodID getInputStreamMethod;
  jmethodID getErrorStreamMethod;
  jmethodID bufferedReaderConstructor;
  jmethodID inputStreamReaderConstructor;
  jmethodID readLineMethod;
  jmethodID readerCloseMethod;
  jmethodID readMethod;
  jmethodID inputStreamCloseMethod;
  jmethodID setFollowMethod;
  jmethodID getHeaderMethod;
  jmethodID getHeaderMethodInt;
  jmethodID getHeaderKeyMethod;

 private:
  JNIEnv * env;
};

class AndroidClientFactory : public HTTPClientFactory {
 public:
 AndroidClientFactory(const std::shared_ptr<AndroidClientCache> & _cache)
   : cache(_cache) { }
 AndroidClientFactory(const std::shared_ptr<AndroidClientCache> & _cache, const char * _default_user_agent)
   : HTTPClientFactory(_default_user_agent), cache(_cache) { }

  std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) override;

 private:
  std::shared_ptr<AndroidClientCache> cache;
};

#endif
