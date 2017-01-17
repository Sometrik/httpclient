#include <AndroidClient.h>
#include <jni.h>
#include <android/log.h>
#include <vector>

AndroidClientCache::AndroidClientCache(JNIEnv * _env) {
  _env->GetJavaVM(&javaVM);
  auto env = getJNIEnv();

  cookieManagerClass = (jclass) env->NewGlobalRef(env->FindClass("android/webkit/CookieManager"));
  httpClass = (jclass) env->NewGlobalRef(env->FindClass("java/net/HttpURLConnection"));
  urlClass = (jclass) env->NewGlobalRef(env->FindClass("java/net/URL"));
  inputStreamClass = (jclass) env->NewGlobalRef(env->FindClass("java/io/InputStream"));

  getHeaderMethod = env->GetMethodID(httpClass, "getHeaderField", "(Ljava/lang/String;)Ljava/lang/String;");
  getHeaderMethodInt = env->GetMethodID(httpClass, "getHeaderField", "(I)Ljava/lang/String;");
  getHeaderKeyMethod = env->GetMethodID(httpClass, "getHeaderFieldKey", "(I)Ljava/lang/String;");
  readMethod = env->GetMethodID(inputStreamClass, "read", "([B)I");
  urlConstructor = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
  openConnectionMethod = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
  setRequestProperty = env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
  setRequestMethod = env->GetMethodID(httpClass, "setRequestMethod", "(Ljava/lang/String;)V");
  setFollowMethod = env->GetMethodID(httpClass, "setInstanceFollowRedirects", "(Z)V");
  setDoInputMethod = env->GetMethodID(httpClass, "setDoInput", "(Z)V");
  connectMethod = env->GetMethodID(httpClass, "connect", "()V");
  getResponseCodeMethod = env->GetMethodID(httpClass, "getResponseCode", "()I");
  getResponseMessageMethod = env->GetMethodID(httpClass, "getResponseMessage", "()Ljava/lang/String;");
  setRequestPropertyMethod = env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
  clearCookiesMethod = env->GetMethodID(cookieManagerClass, "removeAllCookie", "()V");
  getInputStreamMethod = env->GetMethodID(httpClass, "getInputStream", "()Ljava/io/InputStream;");
  getErrorStreamMethod = env->GetMethodID(httpClass, "getErrorStream", "()Ljava/io/InputStream;");
}

AndroidClientCache::~AndroidClientCache() {
  auto env = getJNIEnv();
  env->DeleteGlobalRef(cookieManagerClass);
  env->DeleteGlobalRef(httpClass);
  env->DeleteGlobalRef(urlClass);
  env->DeleteGlobalRef(inputStreamClass);
}

class AndroidClient : public HTTPClient {
public:
  AndroidClient(const std::shared_ptr<AndroidClientCache> & _cache, const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive), cache(_cache) {
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) {
    JNIEnv * env = cache->getJNIEnv();
    
    jobject url = env->NewObject(cache->urlClass, cache->urlConstructor, env->NewStringUTF(req.getURI().c_str()));
    jobject connection = env->CallObjectMethod(url, cache->openConnectionMethod);

    //Authorization example
    //env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF("Authorization"), env->NewStringUTF("myUsername"));
    //  std::string auth_header = auth.createHeader();
    // if (!auth_header.empty()) {
    //		env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF(auth.getHeaderName()), env->NewStringUTF(auth_header.c_str()));
    //}

    env->CallVoidMethod(connection, cache->setFollowMethod, req.getFollowLocation() ? JNI_TRUE : JNI_FALSE);

    // Setting headers for request
    map<string, string> combined_headers;
    for (auto & hd : default_headers) {
      combined_headers[hd.first] = hd.second;      
    }
    for (auto & hd : req.getHeaders()) {
      combined_headers[hd.first] = hd.second;
    }
    for (auto & hd : combined_headers) {
      env->CallVoidMethod(connection, cache->setRequestPropertyMethod, env->NewStringUTF(hd.first.c_str()), env->NewStringUTF(hd.second.c_str()));
    }

    env->CallVoidMethod(connection, cache->setRequestMethod, env->NewStringUTF(req.getTypeString()));

    int responseCode = env->CallIntMethod(connection, cache->getResponseCodeMethod);

    // Server not found error
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "EXCEPTION http request responsecode = %i", responseCode);
      callback.handleResultCode(500);
      // callback->handleResultText("Server not found");
      return;
    }

    callback.handleResultCode(responseCode);

    const char *errorMessage = "";
    jobject input;

    if (responseCode >= 400 && responseCode <= 599) {
      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "request responsecode = %i", responseCode);

      jstring javaMessage = (jstring)env->CallObjectMethod(connection, cache->getResponseMessageMethod);
      errorMessage = env->GetStringUTFChars(javaMessage, 0);

      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "errorMessage = %s", errorMessage);
      input = env->CallObjectMethod(connection, cache->getErrorStreamMethod);
    } else {
      __android_log_print(ANDROID_LOG_INFO, "AndroidClient", "http request responsecode = %i", responseCode);

      input = env->CallObjectMethod(connection, cache->getInputStreamMethod);
      env->ExceptionClear();
    }

    jbyteArray array = env->NewByteArray(4096);
    int g = 0;

    __android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "Starting to gather content");

    // Gather headers and values
    for (int i = 0; ; i++) {
      jstring jheaderKey = (jstring)env->CallObjectMethod(connection, cache->getHeaderKeyMethod, i);
      const char * headerKey = env->GetStringUTFChars(jheaderKey, 0);
      __android_log_print(ANDROID_LOG_INFO, "content", "header key = %s", headerKey);

      jstring jheader = (jstring)env->CallObjectMethod(connection, cache->getHeaderMethodInt, i);
      const char * header = env->GetStringUTFChars(jheader, 0);
      __android_log_print(ANDROID_LOG_INFO, "content", "header value = %s", header);
      if (headerKey == NULL) {
	break;
      }
      callback.handleHeader(headerKey, header);
      env->ReleaseStringUTFChars(jheaderKey, headerKey);
      env->ReleaseStringUTFChars(jheader, header);
    }

    // Gather content
    while ((g = env->CallIntMethod(input, cache->readMethod, array)) != -1) {
      jbyte* content_array = env->GetByteArrayElements(array, NULL);
      callback.handleChunk(g, (char*) content_array);
      env->ReleaseByteArrayElements(array, content_array, JNI_ABORT);
    }

    if (responseCode >= 300 && responseCode <= 399) {
      jstring followURL = (jstring)env->CallObjectMethod(connection, cache->getHeaderMethod, env->NewStringUTF("location"));
      const char *followString = env->GetStringUTFChars(followURL, 0);
      callback.handleRedirectUrl(followString);
      env->ReleaseStringUTFChars(followURL, followString);
      __android_log_print(ANDROID_LOG_INFO, "content", "followURL = %s", followString);

    }
  }

  void clearCookies() {
    JNIEnv * env = cache->getJNIEnv();
    env->CallVoidMethod(cache->cookieManagerClass, cache->clearCookiesMethod);
  }

private:
  std::shared_ptr<AndroidClientCache> cache;
};

std::unique_ptr<HTTPClient>
AndroidClientFactory::createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<AndroidClient>(new AndroidClient(cache, _user_agent, _enable_cookies, _enable_keepalive));
}
