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

  JNIEnv * getJNIEnv() {

//    JNIEnv *Myenv = NULL;
//    javaVM->GetEnv((void**)&Myenv, JNI_VERSION_1_6);''
    if (javaVM->GetEnv((void**)&env, JNI_VERSION_1_2) == JNI_EDETACHED) {
      __android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "Client Env was null. Attaching");

      JavaVMAttachArgs args;
      args.version = JNI_VERSION_1_6; // choose your JNI version
      args.name = NULL; // you might want to give the java thread a name
      args.group = NULL; // you might want to assign the java thread to a ThreadGroup
      javaVM->AttachCurrentThread(&env, &args);
      return env;
    } else {
      return env;
    }
  }

  void checkDetaching(){
    if (javaVM->GetEnv((void**) &env, JNI_VERSION_1_2) == JNI_EDETACHED) {
      __android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "JavaVM was detached");
    } else {
      __android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "JavaVM was attached. Detaching");
      javaVM->DetachCurrentThread();
    }
  }

  JavaVM * getJavaVM(){ return javaVM; }

  jclass cookieManagerClass;
  jmethodID clearCookiesMethod;
  jclass bitmapClass;
  jclass factoryClass;
  jclass httpClass;
  jclass urlClass;
  jclass urlConnectionClass;
  jclass bufferedReaderClass;
  jclass inputStreamReaderClass;
  jclass inputStreamClass;
  jclass frameworkClass;
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
  jmethodID handleThrowableMethod;
  jmethodID setUseCachesMethod;
  jmethodID disconnectConnectionMethod;

 private:
  JavaVM * javaVM;
  JNIEnv * env;
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
