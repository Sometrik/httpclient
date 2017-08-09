#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include "HTTPClient.h"

#include <memory>
#include <jni.h>
#include <android/log.h>

class AndroidClientCache {
 public:
  AndroidClientCache(JNIEnv * _env);
  ~AndroidClientCache();

  JNIEnv * getEnv() {
    myEnv = 0;
    javaVM->GetEnv((void**)&myEnv, JNI_VERSION_1_6);
    return myEnv;
  }

  jclass cookieManagerClass;
  jmethodID clearCookiesMethod;
  jclass httpClass;
  jclass urlClass;
  jclass urlConnectionClass;
  jclass bufferedReaderClass;
  jclass inputStreamReaderClass;
  jclass inputStreamClass;
  jclass frameworkClass;
  jclass outputStreamClass;
  jmethodID urlConstructor;
  jmethodID openConnectionMethod;
  jmethodID getOutputStreamMethod;
  jmethodID outputStreamWriteMethod;
  jmethodID setReadTimeoutMethod;
  jmethodID setChunkedStreamingModeMethod;
  jmethodID setFixedLengthStreamingModeMethod;
  jmethodID setRequestProperty;
  jmethodID setRequestMethod;
  jmethodID setDoInputMethod;
  jmethodID setDoOutputMethod;
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
  jmethodID handleThrowableMethod;
  jmethodID setUseCachesMethod;
  jmethodID disconnectConnectionMethod;

 private:
  JavaVM * javaVM;
  JNIEnv * myEnv;
};

class AndroidClientFactory : public HTTPClientFactory {
 public:
 AndroidClientFactory(const std::shared_ptr<AndroidClientCache> & _cache)
   : cache(_cache) { }
 AndroidClientFactory(const std::shared_ptr<AndroidClientCache> & _cache, const char * _default_user_agent)
   : HTTPClientFactory(_default_user_agent), cache(_cache) { }

  std::unique_ptr<HTTPClient> createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) override;

 private:
  std::shared_ptr<AndroidClientCache> cache;
};

#endif
