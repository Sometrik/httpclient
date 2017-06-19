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
  setDoOutputMethod = myEnv->GetMethodID(httpClass, "setDoOutput", "(Z)V");
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
  }



  ~AndroidClient(){
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

    __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "Host = %s, ua = %s", req.getURI().c_str(), user_agent.c_str());
    jstring juri = env->NewStringUTF(req.getURI().c_str());
    jobject url = env->NewObject(cache->urlClass, cache->urlConstructor, juri);
    env->DeleteLocalRef(juri);

    jobject connection = env->CallObjectMethod(url, cache->openConnectionMethod);
    env->CallVoidMethod(connection, cache->setUseCachesMethod, JNI_FALSE);
    env->DeleteLocalRef(url);

    //Authorization example
//    env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF("Authorization"), env->NewStringUTF("myUsername"));
    std::string auth_header = auth.createHeader();
   env->CallVoidMethod(connection, cache->setFollowMethod, req.getFollowLocation() ? JNI_TRUE : JNI_FALSE);

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
      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "Auth: %s = %s", auth.getHeaderName(), auth_header.c_str());
      combined_headers[auth.getHeaderName()] = auth_header;
    }
    for (auto & hd : combined_headers) {
      jstring firstHeader = env->NewStringUTF(hd.first.c_str());
      jstring secondHeader = env->NewStringUTF(hd.second.c_str());
      env->CallVoidMethod(connection, cache->setRequestPropertyMethod, firstHeader, secondHeader);
      env->DeleteLocalRef(firstHeader);
      env->DeleteLocalRef(secondHeader);
    }

    if (req.getType() == HTTPRequest::GET) {
      env->CallVoidMethod(connection, cache->setDoOutput, false);
    } else if (req.getType() == HTTPRequest::POST) {
      env->CallVoidMethod(connection, cache->setDoOutput, true);
    } else {
      jstring jTypeString = env->NewStringUTF(req.getTypeString());
      env->CallVoidMethod(connection, cache->setRequestMethod, jTypeString);
      env->DeleteLocalRef(jTypeString);
    }
    
    int responseCode = env->CallIntMethod(connection, cache->getResponseCodeMethod);

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
      // Gather headers and values
      for (int i = 0;; i++) {
        jstring jheaderKey = (jstring) env->CallObjectMethod(connection, cache->getHeaderKeyMethod, i);
        if (jheaderKey == NULL) {
          break;
        }
        const char * headerKey = env->GetStringUTFChars(jheaderKey, 0);

        jstring jheader = (jstring) env->CallObjectMethod(connection, cache->getHeaderMethodInt, i);
        const char * header = env->GetStringUTFChars(jheader, 0);
        __android_log_print(ANDROID_LOG_INFO, "content", "header: %s = %s", headerKey, header);

        callback.handleHeader(headerKey, header);

        if (strcasecmp(headerKey, "Location") == 0) {
          callback.handleRedirectUrl(header);
        }

        env->DeleteLocalRef(jheaderKey);
        env->DeleteLocalRef(jheader);
      }

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
