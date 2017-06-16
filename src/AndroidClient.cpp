#include <AndroidClient.h>
#include <jni.h>
#include <android/log.h>
#include <vector>

AndroidClientCache::AndroidClientCache(JNIEnv * _env) : myEnv(_env) {
  myEnv->GetJavaVM(&javaVM);
  cookieManagerClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("android/webkit/CookieManager"));
  httpClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/HttpURLConnection"));
  urlClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/URL"));
  urlConnectionClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/URLConnection"));
  inputStreamClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/io/InputStream"));
  frameworkClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("com/sometrik/framework/FrameWork"));

  getHeaderMethod = myEnv->GetMethodID(httpClass, "getHeaderField", "(Ljava/lang/String;)Ljava/lang/String;");
  getHeaderMethodInt = myEnv->GetMethodID(httpClass, "getHeaderField", "(I)Ljava/lang/String;");
  getHeaderKeyMethod = myEnv->GetMethodID(httpClass, "getHeaderFieldKey", "(I)Ljava/lang/String;");
  readMethod = myEnv->GetMethodID(inputStreamClass, "read", "([B)I");
  urlConstructor = myEnv->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
  openConnectionMethod = myEnv->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
  setUseCachesMethod = myEnv->GetMethodID(urlConnectionClass, "setUseCaches", "(Z)V");
  disconnectConnectionMethod = myEnv->GetMethodID(httpClass, "disconnect", "()V");
  setRequestProperty = myEnv->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
  setRequestMethod = myEnv->GetMethodID(httpClass, "setRequestMethod", "(Ljava/lang/String;)V");
  setFollowMethod = myEnv->GetMethodID(httpClass, "setInstanceFollowRedirects", "(Z)V");
  setDoInputMethod = myEnv->GetMethodID(httpClass, "setDoInput", "(Z)V");
  getResponseCodeMethod = myEnv->GetMethodID(httpClass, "getResponseCode", "()I");
  getResponseMessageMethod = myEnv->GetMethodID(httpClass, "getResponseMessage", "()Ljava/lang/String;");
  setRequestPropertyMethod = myEnv->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
  clearCookiesMethod = myEnv->GetMethodID(cookieManagerClass, "removeAllCookie", "()V");
  getInputStreamMethod = myEnv->GetMethodID(httpClass, "getInputStream", "()Ljava/io/InputStream;");
  getErrorStreamMethod = myEnv->GetMethodID(httpClass, "getErrorStream", "()Ljava/io/InputStream;");
  handleThrowableMethod = myEnv->GetStaticMethodID(frameworkClass, "handleNativeException", "(Ljava/lang/Throwable;)V");
}

AndroidClientCache::~AndroidClientCache() {
  __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClien", "destructor on clientCache");
  myEnv->DeleteGlobalRef(cookieManagerClass);
  myEnv->DeleteGlobalRef(httpClass);
  myEnv->DeleteGlobalRef(urlClass);
  myEnv->DeleteGlobalRef(inputStreamClass);
  myEnv->DeleteGlobalRef(frameworkClass);
}

class AndroidClient : public HTTPClient {
public:
  AndroidClient(const std::shared_ptr<AndroidClientCache> & _cache, const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive), cache(_cache) {

    __android_log_print(ANDROID_LOG_INFO, "Sometrik", "new AndroidClient. No Attach yet");
  }



  ~AndroidClient(){
    __android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "Destructor of androidClient");
    __android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "androidClient destructor done");
   if (stored_env) {
       cache->getJavaVM()->DetachCurrentThread();
   }
  }

  JNIEnv * getEnv() {
    if (stored_env) return stored_env;
    else {
      stored_env = cache->createJNIEnv();
      // cache->checkEnvAttachment(&env);
      return stored_env;
    }
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) {
    JNIEnv * env = getEnv();

    if (env == 0){
      __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Env was null on request start");
    }

    __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "Host = %s", req.getURI().c_str());
    jstring juri = env->NewStringUTF(req.getURI().c_str());
    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "created url string");
    jobject url = env->NewObject(cache->urlClass, cache->urlConstructor, juri);
    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "created url object");
    env->DeleteLocalRef(juri);

    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Creating connection");
    jobject connection = env->CallObjectMethod(url, cache->openConnectionMethod);
    env->CallVoidMethod(connection, cache->setUseCachesMethod, JNI_FALSE);
    env->DeleteLocalRef(url);

    //Authorization example
//    env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF("Authorization"), env->NewStringUTF("myUsername"));
    std::string auth_header = auth.createHeader();
#if 0
    if (!auth_header.empty()) {
      jstring jheaderName = env->NewStringUTF(auth.getHeaderName());
      jstring jheader = env->NewStringUTF(auth_header.c_str());
      env->CallVoidMethod(connection, cache->setRequestPropertyMethod, jheaderName, jheader);
      env->DeleteLocalRef(jheaderName);
      env->DeleteLocalRef(jheader);
    }
#endif
    env->CallVoidMethod(connection, cache->setFollowMethod, req.getFollowLocation() ? JNI_TRUE : JNI_FALSE);

#if 0
    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Applying user agent");
    //Apply user agent
    const char * cuser_agent = user_agent.c_str();
    const char * cuser_agent_key = "User-Agent";
    jstring juser_agent = env->NewStringUTF(user_agent.c_str());
    jstring juser_agent_key = env->NewStringUTF(cuser_agent_key);
    env->CallVoidMethod(connection, cache->setRequestProperty, juser_agent_key, juser_agent);
    env->DeleteLocalRef(juser_agent);
    env->DeleteLocalRef(juser_agent_key);
#endif
    
    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Setting headers");
    // Setting headers for request
    std::map<std::string, std::string> combined_headers;
    for (auto & hd : default_headers) {
      combined_headers[hd.first] = hd.second;
    }
    combined_headers["User-Agent"] = user_agent;
    for (auto & hd : req.getHeaders()) {
      combined_headers[hd.first] = hd.second;
    }
    if (!auth_header.empty()) {
      combined_headers[auth.getHeaderName()] = auth_header;
    }
    for (auto & hd : combined_headers) {
      __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Setting some combined header");
      jstring firstHeader = env->NewStringUTF(hd.first.c_str());
      jstring secondHeader = env->NewStringUTF(hd.second.c_str());
      env->CallVoidMethod(connection, cache->setRequestPropertyMethod, firstHeader, secondHeader);
      env->DeleteLocalRef(firstHeader);
      env->DeleteLocalRef(secondHeader);
      __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Some Combined header set");
    }


    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Preparing to get response code");
    jstring jTypeString = env->NewStringUTF(req.getTypeString());
    env->CallVoidMethod(connection, cache->setRequestMethod, jTypeString);
    env->DeleteLocalRef(jTypeString);
    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "About to get response code");
    int responseCode = env->CallIntMethod(connection, cache->getResponseCodeMethod);

    __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "got response code");


    jobject input = 0;

    if (env->ExceptionCheck()) {
      env->ExceptionClear();
      __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "EXCEPTION http request failed");
      callback.handleResultCode(500);

    } else {
      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "http request responsecode = %i", responseCode);

      input = env->CallObjectMethod(connection, cache->getInputStreamMethod);
      callback.handleResultCode(responseCode);

      if (env->ExceptionCheck()) {
        env->ExceptionClear();
        __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Exception while getting stream");
        input = env->CallObjectMethod(connection, cache->getErrorStreamMethod);
      }
    }
    if (input != 0) {
//      __android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "Starting to gather content");

      // Gather headers and values
      for (int i = 0;; i++) {
        jstring jheaderKey = (jstring) env->CallObjectMethod(connection, cache->getHeaderKeyMethod, i);
        if (jheaderKey == NULL) {
          break;
        }
        const char * headerKey = env->GetStringUTFChars(jheaderKey, 0);

        jstring jheader = (jstring) env->CallObjectMethod(connection, cache->getHeaderMethodInt, i);
        const char * header = env->GetStringUTFChars(jheader, 0);
//        __android_log_print(ANDROID_LOG_INFO, "content", "header: %s = %s", headerKey, header);

        callback.handleHeader(headerKey, header);

        if (strcasecmp(headerKey, "Location") == 0) {
          callback.handleRedirectUrl(header);
        }

        env->DeleteLocalRef(jheaderKey);
        env->DeleteLocalRef(jheader);
      }
//      __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Headers gotten");

      // Gather content
      jbyteArray array = env->NewByteArray(4096);
      int g = 0;
      while ((g = env->CallIntMethod(input, cache->readMethod, array)) != -1) {

        if (env->ExceptionCheck()) {
          __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Exception while reading content");
          env->ExceptionClear();
          break;
        }
        jbyte* content_array = env->GetByteArrayElements(array, NULL);
        bool r = callback.handleChunk(g, (char*) content_array);
        env->ReleaseByteArrayElements(array, content_array, JNI_ABORT);
	if (!r) {
	  break;
	}
      }
      env->DeleteLocalRef(array);

//      __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Content gathered");
      callback.handleDisconnect();

      env->CallVoidMethod(connection, cache->disconnectConnectionMethod);
      env->DeleteLocalRef(input);
    }
    env->DeleteLocalRef(connection);
    cache->getJavaVM()->DetachCurrentThread();
    stored_env = 0;
}

  void clearCookies() {
//    JNIEnv * env = cache->getJNIEnv();
//    env->CallVoidMethod(cache->cookieManagerClass, cache->clearCookiesMethod);
  }

private:
  std::shared_ptr<AndroidClientCache> cache;
  JNIEnv * stored_env = 0;
};

std::unique_ptr<HTTPClient>
AndroidClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<AndroidClient>(new AndroidClient(cache, _user_agent, _enable_cookies, _enable_keepalive));
}
