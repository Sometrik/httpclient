#include <AndroidClient.h>
#include <jni.h>
#include <android/log.h>

class AndroidClient : public HTTPClient {
 public:

	AndroidClient(JNIEnv * _env, const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
 : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive), env(_env) {


		__android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "AndroidClient Constructor called");

	}

	void androidInit(){

		cookieManagerClass =  env->FindClass("android/webkit/CookieManager");
		httpClass = env->FindClass("java/net/HttpURLConnection");
		urlClass = env->FindClass("java/net/URL");
	 	inputStreamClass = env->FindClass("java/io/InputStream");


		getHeaderMethod = env->GetMethodID(httpClass, "getHeaderField", "(Ljava/lang/String;)Ljava/lang/String;");
	 	readMethod = env->GetMethodID(inputStreamClass, "read", "([B)I");
	 	urlConstructor =  env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
		openConnectionMethod = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
		setRequestProperty = env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
		setRequestMethod = env->GetMethodID(httpClass, "setRequestMethod", "(Ljava/lang/String;)V");
		setFollowMethod = env->GetMethodID(httpClass, "setInstanceFollowRedirects", "(Z)V");
		setDoInputMethod = env->GetMethodID(httpClass, "setDoInput", "(Z)V");
		connectMethod = env->GetMethodID(httpClass, "connect", "()V");
		getResponseCodeMethod = env->GetMethodID(httpClass, "getResponseCode", "()I");
		getResponseMessageMethod = env->GetMethodID(httpClass, "getResponseMessage", "()Ljava/lang/String;");
		setRequestPropertyMethod =  env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
		clearCookiesMethod =  env->GetMethodID(cookieManagerClass, "removeAllCookie", "()V");
		getInputStreamMethod =  env->GetMethodID(httpClass, "getInputStream", "()Ljava/io/InputStream;");

		initDone = true;

	}

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth){


		__android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "AndroidClient request called");

  	if (!initDone){
  		androidInit();
  	}


  	jobject url = env->NewObject(urlClass, urlConstructor, env->NewStringUTF(req.getURI().c_str()));
  	jobject connection = env->CallObjectMethod(url, openConnectionMethod);


  	//Authorization example
		//env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF("Authorization"), env->NewStringUTF("myUsername"));

		//  std::string auth_header = auth.createHeader();

		// if (!auth_header.empty()) {
		//		env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF(auth.getHeaderName()), env->NewStringUTF(auth_header.c_str()));
		//}

		env->CallVoidMethod(connection, setFollowMethod, req.getFollowLocation() ? JNI_TRUE : JNI_FALSE);

		switch (req.getType()) {
		case HTTPRequest::POST:
			env->CallVoidMethod(connection, setRequestMethod, env->NewStringUTF("POST"));
			break;
		case HTTPRequest::GET:
			env->CallVoidMethod(connection, setRequestMethod, env->NewStringUTF("GET"));
			;
			break;
		}

		int responseCode = env->CallIntMethod(connection, getResponseCodeMethod);

		if (env->ExceptionCheck()) {
			env->ExceptionClear();
			__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "EXCEPTION http request responsecode = %i", responseCode);
			return HTTPResponse(0, "exception");
		}
		__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "http request responsecode = %i", responseCode);

		const char *errorMessage = "";

		if (responseCode >= 400 && responseCode <= 599){
			jstring javaMessage = (jstring)env->CallObjectMethod(connection, getResponseMessageMethod);
			errorMessage = env->GetStringUTFChars(javaMessage, 0);

		}

		jobject input = env->CallObjectMethod(connection, getInputStreamMethod);
		env->ExceptionClear();

		jbyteArray array = env->NewByteArray(4096);
		int g = 0;
		std::string content;

		while ((g = env->CallIntMethod(input, readMethod, array)) != -1) {

			jbyte* content_array = env->GetByteArrayElements(array, NULL);
			if (callback) {
				callback->handleChunk(g, (char*) content_array);
			} else {
				content += std::string((char*) content_array, g);
			}

			env->ReleaseByteArrayElements(array, content_array, JNI_ABORT);

		}

		const char *followString = "";

		if (responseCode >= 300 && responseCode <= 399) {

			jstring followURL = (jstring)env->CallObjectMethod(connection, getHeaderMethod, env->NewStringUTF("location"));
			followString = env->GetStringUTFChars(followURL, 0);

			__android_log_print(ANDROID_LOG_INFO, "content", "followURL = %s", followString);

		}

		__android_log_print(ANDROID_LOG_INFO, "content", "contentti = %Ld", content.size());

		return HTTPResponse(responseCode, errorMessage, followString, content);

  }

  void clearCookies() {

  	env->CallVoidMethod(cookieManagerClass, clearCookiesMethod);

  }

 protected:
  bool initialize() { return true; }


 private:
	bool initDone = false;

  JNIEnv * env;
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
  jmethodID bufferedReaderConstructor;
  jmethodID inputStreamReaderConstructor;
  jmethodID readLineMethod;
  jmethodID readerCloseMethod;
  jmethodID readMethod;
  jmethodID inputStreamCloseMethod;
  jmethodID setFollowMethod;
  jmethodID getHeaderMethod;

};

std::shared_ptr<HTTPClient>
AndroidClientFactory::createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::make_shared<AndroidClient>(env, _user_agent, _enable_cookies, _enable_keepalive);
}

