#include <AndroidClient.h>
#include <jni.h>
#include <android/log.h>
#include <vector>

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
		getHeaderMethodInt = env->GetMethodID(httpClass, "getHeaderField", "(I)Ljava/lang/String;");
		getHeaderKeyMethod = env->GetMethodID(httpClass, "getHeaderFieldKey", "(I)Ljava/lang/String;");
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
		getErrorStreamMethod =  env->GetMethodID(httpClass, "getErrorStream", "()Ljava/io/InputStream;");

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


		//Setting headers for request
		auto & headerMap = req.getHeaders();

		for (auto it = headerMap.begin(); it != headerMap.end(); ++it)
		{
						__android_log_print(ANDROID_LOG_INFO, "httpRequest", "Setting header property name = %s", it->first.c_str());
						__android_log_print(ANDROID_LOG_INFO, "httpRequest", "Setting header property value = %s", it->second.c_str());
			env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF(it->first.c_str()), env->NewStringUTF(it->second.c_str()));
		}


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

		//Server not found error
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
			__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "EXCEPTION http request responsecode = %i", responseCode);
			return HTTPResponse(0, "Server not found");
		}

		const char *errorMessage = "";
		jobject input;

		if (responseCode >= 400 && responseCode <= 599 ){
			__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "request responsecode = %i", responseCode);

			jstring javaMessage = (jstring)env->CallObjectMethod(connection, getResponseMessageMethod);
			errorMessage = env->GetStringUTFChars(javaMessage, 0);

			__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "errorMessage = %s", errorMessage);
			input = env->CallObjectMethod(connection, getErrorStreamMethod);

		} else {

			__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "http request responsecode = %i", responseCode);

			input = env->CallObjectMethod(connection, getInputStreamMethod);
			env->ExceptionClear();
		}

		jbyteArray array = env->NewByteArray(4096);
		int g = 0;

		HTTPResponse response;
		__android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "Starting to gather content");

		//Gather content
		while ((g = env->CallIntMethod(input, readMethod, array)) != -1) {

			jbyte* content_array = env->GetByteArrayElements(array, NULL);
			if (callback) {
				callback->handleChunk(g, (char*) content_array);
			} else {
				response.appendContent(std::string((char*) content_array, g));
			}
			env->ReleaseByteArrayElements(array, content_array, JNI_ABORT);
		}

		//Gather headers and values
		for (int i = 0; i < 1000; i++) {
			jstring jheaderKey = (jstring)env->CallObjectMethod(connection, getHeaderKeyMethod, i);
			const char * headerKey = env->GetStringUTFChars(jheaderKey, 0);
			__android_log_print(ANDROID_LOG_INFO, "content", "header key = %s", headerKey);

			jstring jheader = (jstring) env->CallObjectMethod(connection, getHeaderMethodInt, i);
			const char * header = env->GetStringUTFChars(jheader, 0);
			__android_log_print(ANDROID_LOG_INFO, "content", "header value = %s", header);
			if (headerKey == NULL) {
				break;
			}
			response.addHeader(headerKey, header);
			env->ReleaseStringUTFChars(jheaderKey, headerKey);
			env->ReleaseStringUTFChars(jheader, header);
		}

		response.setResultCode(responseCode);

		if (responseCode >= 300 && responseCode <= 399) {

			jstring followURL = (jstring)env->CallObjectMethod(connection, getHeaderMethod, env->NewStringUTF("location"));
			const char *followString = env->GetStringUTFChars(followURL, 0);
			response.setRedirectUrl(followString);
			env->ReleaseStringUTFChars(followURL, followString);
			__android_log_print(ANDROID_LOG_INFO, "content", "followURL = %s", followString);

		}

//		response.addHeader("", "");
		return response; // HTTPResponse(responseCode, errorMessage, followString, content);

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

};

std::shared_ptr<HTTPClient>
AndroidClientFactory::createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::make_shared<AndroidClient>(env, _user_agent, _enable_cookies, _enable_keepalive);
}
