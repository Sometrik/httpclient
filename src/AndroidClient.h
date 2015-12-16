#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include "HTTPClient.h"

#include <jni.h>

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

		urlConstructor =  env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
		openConnectionMethod = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
		setRequestProperty = env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
		setRequestMethod = env->GetMethodID(httpClass, "setRequestMethod", "(Ljava/lang/String;)V");
		setDoInputMethod = env->GetMethodID(httpClass, "setDoInput", "(Z)V");
		connectMethod = env->GetMethodID(httpClass, "connect", "()V");
		getResponseCodeMethod = env->GetMethodID(httpClass, "getResponseCode", "()I");
		getResponseMessageMethod = env->GetMethodID(httpClass, "getResponseMessage", "()Ljava/lang/String;");
		setRequestPropertyMethod =  env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
		clearCookiesMethod =  env->GetMethodID(cookieManagerClass, "removeAllCookie", "()V");

		initDone = true;

	}

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth){


		__android_log_print(ANDROID_LOG_VERBOSE, "Sometrik", "AndroidClient request called");

  	if (!initDone){
  		androidInit();
  	}


  	jobject url = env->NewObject(urlClass, urlConstructor, env->NewStringUTF(req.getURI().c_str()));
  	//jobject url = env->NewObject(urlClass, urlConstructor, "sometrik.com");

  	jobject connection = env->CallObjectMethod(url, openConnectionMethod);

  	//Authorization example
		env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF("Authorization"), env->NewStringUTF("myUsername"));

		//  std::string auth_header = auth.createHeader();

		// if (!auth_header.empty()) {
		//		env->CallVoidMethod(connection, setRequestPropertyMethod, env->NewStringUTF(auth.getHeaderName()), env->NewStringUTF(auth_header.c_str()));
		//}




		//Set Follow enabled

		switch (req.getType()) {
		case HTTPRequest::POST:
		  	env->CallVoidMethod(connection, setRequestMethod, env->NewStringUTF("POST"));
			break;
		case HTTPRequest::GET:
				 env->CallVoidMethod(connection, setRequestMethod, env->NewStringUTF("GET"));;
			break;
		}

		//Brings out exception, if URL is bad --- needs exception (IOExcpetion) handling or something
			int responseCode = env->CallIntMethod(connection, getResponseCodeMethod);

			if (env->ExceptionCheck()) {
				__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "FUCK!");
			  return HTTPResponse(0, "Exception");
			}
			__android_log_print(ANDROID_LOG_INFO, "AndroidClient", "http request responsecode = %i", responseCode);


			const char *errorMessage = "";

		if (responseCode >= 400 && responseCode <= 599){
			jstring javaMessage = (jstring)env->CallObjectMethod(connection, getResponseMessageMethod);
			errorMessage = env->GetStringUTFChars(javaMessage, 0);

		}

		// stream
		if (callback) {
			while ( 1 ) {
				  // lue streamia
					callback->handleChunk(0, 0);
			}
  		return HTTPResponse(responseCode, errorMessage);
		} else {
			// lue koko stream
			std::string content;
			return HTTPResponse(responseCode, errorMessage, "", content);
		}
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

  jclass httpClass;
  jclass urlClass;
  jmethodID urlConstructor;
  jmethodID openConnectionMethod;
  jmethodID setRequestProperty;
  jmethodID setRequestMethod;
  jmethodID setDoInputMethod;
  jmethodID connectMethod;
  jmethodID getResponseCodeMethod;
  jmethodID getResponseMessageMethod;
  jmethodID setRequestPropertyMethod;

};

class AndroidClientFactory : public HTTPClientFactory {
 public:
 AndroidClientFactory(JNIEnv * _env) : env(_env) { }

  std::shared_ptr<HTTPClient> createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) override { return std::make_shared<AndroidClient>(env, _user_agent, _enable_cookies, _enable_keepalive); }

 private:
  JNIEnv * env;
};

#endif
