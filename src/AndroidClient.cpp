#include <AndroidClient.h>

#include <android/log.h>
#include <vector>
#include <memory>

using namespace std;
using namespace httpclient;

static jbyteArray convertToByteArray(JNIEnv * env, const std::string & s) {
  const jbyte * pNativeMessage = reinterpret_cast<const jbyte*>(s.c_str());
  jbyteArray bytes = env->NewByteArray(s.size());
  env->SetByteArrayRegion(bytes, 0, s.size(), pNativeMessage);
  return bytes;
}

static void logException(JNIEnv * env, const char * error) {
  jthrowable e = env->ExceptionOccurred();
  env->ExceptionClear();
  jclass clazz = env->GetObjectClass(e);
  jmethodID getMessage = env->GetMethodID(clazz, "toString", "()Ljava/lang/String;"); // or getMessage
  jstring message = (jstring)env->CallObjectMethod(e, getMessage);
  string m;
  if (message != NULL) {
    const char *mstr = env->GetStringUTFChars(message, NULL);
    m = mstr;
    env->ReleaseStringUTFChars(message, mstr);
  }
  env->DeleteLocalRef(message);
  env->DeleteLocalRef(clazz);
  env->DeleteLocalRef(e);

  __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "%s: %s", error, m.c_str());
}

class AndroidClientCache {
 public:
  AndroidClientCache(JNIEnv * myEnv) {
    myEnv->GetJavaVM(&javaVM);
    httpClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/HttpURLConnection"));
    urlClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/URL"));
    urlConnectionClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/net/URLConnection"));
    inputStreamClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/io/InputStream"));
    outputStreamClass = (jclass) myEnv->NewGlobalRef(myEnv->FindClass("java/io/OutputStream"));
    setReadTimeoutMethod = myEnv->GetMethodID(urlConnectionClass, "setReadTimeout", "(I)V");
    setConnectTimeoutMethod = myEnv->GetMethodID(urlConnectionClass, "setConnectTimeout", "(I)V");
    getHeaderMethod = myEnv->GetMethodID(httpClass, "getHeaderField", "(Ljava/lang/String;)Ljava/lang/String;");
    getHeaderMethodInt = myEnv->GetMethodID(httpClass, "getHeaderField", "(I)Ljava/lang/String;");
    getHeaderKeyMethod = myEnv->GetMethodID(httpClass, "getHeaderFieldKey", "(I)Ljava/lang/String;");
    inputStreamReadMethod = myEnv->GetMethodID(inputStreamClass, "read", "([B)I");
    inputStreamCloseMethod = myEnv->GetMethodID(inputStreamClass, "close", "()V");
    urlConstructor = myEnv->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
    openConnectionMethod = myEnv->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
    setUseCachesMethod = myEnv->GetMethodID(urlConnectionClass, "setUseCaches", "(Z)V");
    disconnectConnectionMethod = myEnv->GetMethodID(httpClass, "disconnect", "()V");
    getOutputStreamMethod = myEnv->GetMethodID(urlConnectionClass, "getOutputStream", "()Ljava/io/OutputStream;");
    outputStreamWriteMethod = myEnv->GetMethodID(outputStreamClass, "write", "([B)V");
    outputStreamCloseMethod = myEnv->GetMethodID(outputStreamClass, "close", "()V");
    setChunkedStreamingModeMethod = myEnv->GetMethodID(httpClass, "setChunkedStreamingMode", "(I)V");
    setFixedLengthStreamingModeMethod = myEnv->GetMethodID(httpClass, "setFixedLengthStreamingMode", "(I)V");
    setRequestMethod = myEnv->GetMethodID(httpClass, "setRequestMethod", "(Ljava/lang/String;)V");
    setFollowMethod = myEnv->GetMethodID(httpClass, "setInstanceFollowRedirects", "(Z)V");
    setDoInputMethod = myEnv->GetMethodID(httpClass, "setDoInput", "(Z)V");
    setDoOutputMethod = myEnv->GetMethodID(httpClass, "setDoOutput", "(Z)V");
    getResponseCodeMethod = myEnv->GetMethodID(httpClass, "getResponseCode", "()I");
    setRequestPropertyMethod = myEnv->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
    getInputStreamMethod = myEnv->GetMethodID(httpClass, "getInputStream", "()Ljava/io/InputStream;");
    getErrorStreamMethod = myEnv->GetMethodID(httpClass, "getErrorStream", "()Ljava/io/InputStream;");
  }

  ~AndroidClientCache() {
    JNIEnv * env = getEnv();
    env->DeleteGlobalRef(httpClass);
    env->DeleteGlobalRef(urlClass);
    env->DeleteGlobalRef(inputStreamClass);
    env->DeleteGlobalRef(outputStreamClass);
  }

  JNIEnv * getEnv() {
    JNIEnv * env = 0;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    return env;
  }

  jclass httpClass;
  jclass urlClass;
  jclass urlConnectionClass;
  jclass inputStreamClass;
  jclass outputStreamClass;
  jmethodID urlConstructor;
  jmethodID openConnectionMethod;
  jmethodID getOutputStreamMethod;
  jmethodID outputStreamWriteMethod;
  jmethodID outputStreamCloseMethod;
  jmethodID setReadTimeoutMethod;
  jmethodID setConnectTimeoutMethod;
  jmethodID setChunkedStreamingModeMethod;
  jmethodID setFixedLengthStreamingModeMethod;
  jmethodID setRequestMethod;
  jmethodID setDoInputMethod;
  jmethodID setDoOutputMethod;
  jmethodID getResponseCodeMethod;
  jmethodID setRequestPropertyMethod;
  jmethodID getInputStreamMethod;
  jmethodID getErrorStreamMethod;
  jmethodID inputStreamReadMethod;
  jmethodID inputStreamCloseMethod;
  jmethodID setFollowMethod;
  jmethodID getHeaderMethod;
  jmethodID getHeaderMethodInt;
  jmethodID getHeaderKeyMethod;
  jmethodID setUseCachesMethod;
  jmethodID disconnectConnectionMethod;

 private:
  JavaVM * javaVM;
  JNIEnv * myEnv;
};

class AndroidClient : public HTTPClient {
public:
  AndroidClient(const std::shared_ptr<AndroidClientCache> & _cache, const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive), cache(_cache) {
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) {
    JNIEnv * env = cache->getEnv();

    // __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "Host = %s, ua = %s", req.getURI().c_str(), user_agent.c_str());

    jstring juri = env->NewStringUTF(req.getURI().c_str());
    jobject url = env->NewObject(cache->urlClass, cache->urlConstructor, juri);
    env->DeleteLocalRef(juri);

    if (env->ExceptionCheck()) {
      logException(env, "Malformed URL");
      return;
    }

    jobject connection = env->CallObjectMethod(url, cache->openConnectionMethod);
    env->CallVoidMethod(connection, cache->setUseCachesMethod, JNI_FALSE);
    env->CallVoidMethod(connection, cache->setReadTimeoutMethod, req.getReadTimeout() * 1000);
    env->CallVoidMethod(connection, cache->setConnectTimeoutMethod, req.getConnectTimeout() * 1000);
    env->DeleteLocalRef(url);

    std::string auth_header = auth.createHeader();
    env->CallVoidMethod(connection, cache->setFollowMethod, req.getFollowLocation() ? JNI_TRUE : JNI_FALSE);

    // Setting headers for request
    std::map<std::string, std::string> combined_headers;
    for (auto & hd : default_headers) {
      combined_headers[hd.first] = hd.second;
    }
    combined_headers["User-Agent"] = user_agent;
    if (req.getMethod() == HTTPRequest::POST && !req.getContentType().empty()) {
      combined_headers["Content-Type"] = req.getContentType();
    }
    for (auto & hd : req.getHeaders()) {
      combined_headers[hd.first] = hd.second;
    }
    if (!auth_header.empty()) {
      // __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "Auth: %s = %s", auth.getHeaderName(), auth_header.c_str());
      combined_headers[auth.getHeaderName()] = auth_header;
    }
    for (auto & hd : combined_headers) {
      jstring firstHeader = env->NewStringUTF(hd.first.c_str());
      jstring secondHeader = env->NewStringUTF(hd.second.c_str());
      env->CallVoidMethod(connection, cache->setRequestPropertyMethod, firstHeader, secondHeader);
      env->DeleteLocalRef(firstHeader);
      env->DeleteLocalRef(secondHeader);
    }

    env->CallVoidMethod(connection, cache->setDoOutputMethod, req.getMethod() == HTTPRequest::POST);
    auto method_text = to_string(req.getMethod());
    jstring jTypeString = env->NewStringUTF(method_text.c_str());
    env->CallVoidMethod(connection, cache->setRequestMethod, jTypeString);
    env->DeleteLocalRef(jTypeString);

    bool connection_failed = false;
    
    if (req.getMethod() == HTTPRequest::POST) {
      env->CallVoidMethod(connection, cache->setFixedLengthStreamingModeMethod, (int)req.getContent().size());
      jobject outputStream = env->CallObjectMethod(connection, cache->getOutputStreamMethod);
      if (env->ExceptionCheck()) {
        logException(env, "Failed to open output stream");
	connection_failed = true;
      }

      if (!connection_failed) {
        for (int i = 0; i < req.getContent().size() && !connection_failed; i += 4096) {
          auto a = convertToByteArray(env, req.getContent().substr(i, 4096));
          env->CallVoidMethod(outputStream, cache->outputStreamWriteMethod, a);
	
          if (env->ExceptionCheck()) {
            logException(env, "Failed to write post data");
            connection_failed = true;
          }

          env->DeleteLocalRef(a);
        }

        env->CallVoidMethod(outputStream, cache->outputStreamCloseMethod);
      }

      env->DeleteLocalRef(outputStream);
    }

    int responseCode = 0;
    if (!connection_failed) {
      responseCode = env->CallIntMethod(connection, cache->getResponseCodeMethod);
      if (env->ExceptionCheck()) {
        logException(env, "Failed to get response code");
	connection_failed = true;
      }
    }

    if (connection_failed) {
      callback.handleResultCode(0);
    } else {
      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "http request responsecode = %i", responseCode);
      callback.handleResultCode(responseCode);

      bool force_disconnect = false;

      jobject input = env->CallObjectMethod(connection, cache->getInputStreamMethod);
      if (env->ExceptionCheck()) {
        logException(env, "Failed to open input stream");

        input = env->CallObjectMethod(connection, cache->getErrorStreamMethod);

        if (env->ExceptionCheck()) {
          logException(env, "Failed to open error stream");
          input = 0;
        }
      }

      if (input != 0) {
	// Gather headers and values
	for (int i = 0;; i++) {
	  jstring jheaderKey = (jstring) env->CallObjectMethod(connection, cache->getHeaderKeyMethod, i);
	  if (jheaderKey == NULL) {
	    break;
	  } else {
	    jstring jheader = (jstring) env->CallObjectMethod(connection, cache->getHeaderMethodInt, i);
	    if (jheader != NULL) {
	      const char * headerKey = env->GetStringUTFChars(jheaderKey, 0);
	      const char * header = env->GetStringUTFChars(jheader, 0);
	      callback.handleHeader(headerKey, header);
	  
	      if (strcasecmp(headerKey, "location") == 0) {
	        callback.handleRedirectUrl(header);
	      }
	  
	      env->ReleaseStringUTFChars(jheaderKey, headerKey);
              env->ReleaseStringUTFChars(jheader, header);
              env->DeleteLocalRef(jheader);
            }
	    env->DeleteLocalRef(jheaderKey);
	  }
	}
	
	// Gather content
	jbyteArray array = env->NewByteArray(4096);
	int g = 0;
	while ((g = env->CallIntMethod(input, cache->inputStreamReadMethod, array)) != -1) {
	  if (env->ExceptionCheck()) {
	    logException(env, "Failed to read stream");
	    break;
	  }
	  jbyte* content_array = env->GetByteArrayElements(array, NULL);
	  bool r = callback.handleChunk(g, (char*) content_array);
	  env->ReleaseByteArrayElements(array, content_array, JNI_ABORT);
	  if (!r || !callback.onIdle()) {
	    force_disconnect = true;
	    break;
	  }
	}
	env->CallVoidMethod(input, cache->inputStreamCloseMethod);
	env->DeleteLocalRef(array);
	env->DeleteLocalRef(input);
      }

      if (force_disconnect) { // Terminate connection instead of allowing reuse
	env->CallVoidMethod(connection, cache->disconnectConnectionMethod);
      }
    }
    callback.handleDisconnect();

    env->DeleteLocalRef(connection);
  }

  void clearCookies() { }

private:
  std::shared_ptr<AndroidClientCache> cache;
};

std::shared_ptr<AndroidClientCache> AndroidClientFactory::cache;

void
AndroidClientFactory::initialize(JNIEnv * env) {
  cache = make_shared<AndroidClientCache>(env);
}

std::unique_ptr<HTTPClient>
AndroidClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<AndroidClient>(new AndroidClient(cache, _user_agent, _enable_cookies, _enable_keepalive));
}
