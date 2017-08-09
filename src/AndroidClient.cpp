#include <AndroidClient.h>
#include <jni.h>
#include <android/log.h>
#include <vector>



static jbyteArray convertToByteArray(JNIEnv * env, const std::string & s) {
  const jbyte * pNativeMessage = reinterpret_cast<const jbyte*>(s.c_str());
  jbyteArray bytes = env->NewByteArray(s.size());
  env->SetByteArrayRegion(bytes, 0, s.size(), pNativeMessage);
  return bytes;
}

AndroidClientCache::AndroidClientCache(JNIEnv * _env) : myEnv(_env) {
  myEnv->GetJavaVM(&javaVM);
  cookieManagerClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("android/webkit/CookieManager"));
  httpClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/HttpURLConnection"));
  urlClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/URL"));
  urlConnectionClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/URLConnection"));
  inputStreamClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/io/InputStream"));
  frameworkClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("com/sometrik/framework/FrameWork"));
  outputStreamClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/io/OutputStream"));

  setReadTimeoutMethod = myEnv->GetMethodID(urlConnectionClass, "setReadTimeout", "(I)V");
  getHeaderMethod = myEnv->GetMethodID(httpClass, "getHeaderField", "(Ljava/lang/String;)Ljava/lang/String;");
  getHeaderMethodInt = myEnv->GetMethodID(httpClass, "getHeaderField", "(I)Ljava/lang/String;");
  getHeaderKeyMethod = myEnv->GetMethodID(httpClass, "getHeaderFieldKey", "(I)Ljava/lang/String;");
  readMethod = myEnv->GetMethodID(inputStreamClass, "read", "([B)I");
  urlConstructor = myEnv->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
  openConnectionMethod = myEnv->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
  setUseCachesMethod = myEnv->GetMethodID(urlConnectionClass, "setUseCaches", "(Z)V");
  disconnectConnectionMethod = myEnv->GetMethodID(httpClass, "disconnect", "()V");
  getOutputStreamMethod = myEnv->GetMethodID(urlConnectionClass, "getOutputStream", "()Ljava/io/OutputStream;");
  outputStreamWriteMethod = myEnv->GetMethodID(outputStreamClass, "write", "([B)V");
  setChunkedStreamingModeMethod = myEnv->GetMethodID(httpClass, "setChunkedStreamingMode", "(I)V");
  setFixedLengthStreamingModeMethod = myEnv->GetMethodID(httpClass, "setFixedLengthStreamingMode", "(I)V");
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
  __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "destructor on clientCache");
  JNIEnv * env = getEnv();
  env->DeleteGlobalRef(cookieManagerClass);
  env->DeleteGlobalRef(httpClass);
  env->DeleteGlobalRef(urlClass);
  env->DeleteGlobalRef(inputStreamClass);
  env->DeleteGlobalRef(frameworkClass);
  env->DeleteGlobalRef(outputStreamClass);
}

class AndroidClient : public HTTPClient {
public:
  AndroidClient(const std::shared_ptr<AndroidClientCache> & _cache, const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive), cache(_cache) {

    __android_log_print(ANDROID_LOG_INFO, "Sometrik", "New AndroidClient");
  }



  ~AndroidClient() {

  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) {
    JNIEnv * env = cache->getEnv();

    __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "Host = %s, ua = %s", req.getURI().c_str(), user_agent.c_str());
    jstring juri = env->NewStringUTF(req.getURI().c_str());
    jobject url = env->NewObject(cache->urlClass, cache->urlConstructor, juri);
    env->DeleteLocalRef(juri);

    jobject connection = env->CallObjectMethod(url, cache->openConnectionMethod);
    env->CallVoidMethod(connection, cache->setUseCachesMethod, JNI_FALSE);
    env->CallVoidMethod(connection, cache->setReadTimeoutMethod, req.getReadTimeout() * 1000);
//    env->CallVoidMethod(connection, cache->setConnectTimeoutMethod, req.getConnectTimeout() * 1000);
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
    if (req.getType() == HTTPRequest::POST && !req.getContentType().empty()) {
      combined_headers["Content-Type"] = req.getContentType();
    }
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

    env->CallVoidMethod(connection, cache->setDoOutputMethod, req.getType() == HTTPRequest::POST);
    jstring jTypeString = env->NewStringUTF(req.getTypeString());
    env->CallVoidMethod(connection, cache->setRequestMethod, jTypeString);
    env->DeleteLocalRef(jTypeString);

    bool connection_failed = false;
    
    if (req.getType() == HTTPRequest::POST) {
      env->CallVoidMethod(connection, cache->setFixedLengthStreamingModeMethod, req.getContent().size());
      jobject outputStream = env->CallObjectMethod(connection, cache->getOutputStreamMethod);
      if (env->ExceptionCheck()) {
	__android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Exception while posting content");
	env->ExceptionClear();
	connection_failed = true;
      }

      for (int i = 0; i < req.getContent().size() && !connection_failed; i += 4096) {
        auto a = convertToByteArray(env, req.getContent().substr(i, 4096));
        env->CallVoidMethod(outputStream, cache->outputStreamWriteMethod, a);
	
	if (env->ExceptionCheck()) {
          __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Exception while posting content");
	  env->ExceptionClear();
	  connection_failed = true;
        }

	env->DeleteLocalRef(a);
      }
    }

    int responseCode = 0;
    if (!connection_failed) {
      responseCode = env->CallIntMethod(connection, cache->getResponseCodeMethod);
      if (env->ExceptionCheck()) {
	env->ExceptionClear();
	__android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "EXCEPTION http request failed");
	connection_failed = true;
      }
    }

    if (connection_failed) {
      callback.handleResultCode(0);
    } else {
      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "http request responsecode = %i", responseCode);
      callback.handleResultCode(responseCode);

      jobject input = env->CallObjectMethod(connection, cache->getInputStreamMethod);
      if (env->ExceptionCheck()) {
        env->ExceptionClear();
        __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Exception while getting input stream");
        input = env->CallObjectMethod(connection, cache->getErrorStreamMethod);

        if (env->ExceptionCheck()) {
          env->ExceptionClear();
          __android_log_print(ANDROID_LOG_VERBOSE, "AndroidClient", "Exception while getting error stream");
          input = 0;
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
	  // __android_log_print(ANDROID_LOG_INFO, "content", "header: %s = %s", headerKey, header);
	  
	  callback.handleHeader(headerKey, header);
	  
	  if (strcasecmp(headerKey, "Location") == 0) {
	    callback.handleRedirectUrl(header);
	  }
	  
	  env->ReleaseStringUTFChars(jheaderKey, headerKey);
          env->ReleaseStringUTFChars(jheader, header);
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
	  callback.onIdle();
	}
	env->DeleteLocalRef(array);
	env->DeleteLocalRef(input);
      }
      
      env->CallVoidMethod(connection, cache->disconnectConnectionMethod);
    }
    callback.handleDisconnect();

    env->DeleteLocalRef(connection);
}


  void clearCookies() {
//    JNIEnv * env = cache->getJNIEnv();
//    env->CallVoidMethod(cache->cookieManagerClass, cache->clearCookiesMethod);
  }

private:
  std::shared_ptr<AndroidClientCache> cache;
};

std::unique_ptr<HTTPClient>
AndroidClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<AndroidClient>(new AndroidClient(cache, _user_agent, _enable_cookies, _enable_keepalive));
}
