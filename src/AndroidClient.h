#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPClientInterface.h"
#include "HTTPClient.h"
#include <jni.h>

#include <map>
#include <string>

class AndroidClient : public HTTPClient {
 public:

	AndroidClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive): HTTPClient("", _user_agent, _enable_cookies, _enable_keepalive) {

		androidInit();
	}

	void androidInit(){

		cookieManagerClass =  env->FindClass("android/webkit/CookieManager");
		httpClass = env->FindClass("java/net/HttpURLConnection");
		urlClass = env->FindClass("java/net/URL");

		urlConstructor =  env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
		openConnectionMethod = env->GetMethodID(httpClass, "openConnection", "()java/net/URLConnection;");
		setRequestProperty = env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
		setRequestMethod = env->GetMethodID(httpClass, "setRequestMethod", "(Ljava/lang/String;)V");
		setDoInputMethod = env->GetMethodID(httpClass, "setDoInput", "(Z)V");
		connectMethod = env->GetMethodID(httpClass, "connect", "()V");
		getResponseCodeMethod = env->GetMethodID(httpClass, "getResponseCode ", "()I");
		clearCookiesMethod =  env->GetMethodID(cookieManagerClass, "removeAllCookie", "()V");

	}

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth){

  	jobject url = env->NewObject(urlClass, urlConstructor, env->NewStringUTF(req.getURI().c_str()));
  	//jobject url = env->NewObject(urlClass, urlConstructor, "sometrik.com");

  	//This would normally be casted. How?
  	jobject connection = env->CallObjectMethod(url, openConnectionMethod);
  	//jobject httpConnection = firstConnection.jcast(firstConnection, "java/net/HttpURLConnection", JNI_FALSE, JNI_FALSE);


		switch (req.getType()) {
		case HTTPRequest::POST:
		  	env->CallVoidMethod(connection, setRequestMethod, "POST");
			break;
		case HTTPRequest::GET:
				 env->CallVoidMethod(connection, setRequestMethod, "GET");
			break;
		}

		int responseCode = env->CallIntMethod(connection, getResponseCodeMethod);

		// new HTTPResponse(responseCode, "none");

  }

  void clearCookies() {

  	env->CallVoidMethod(cookieManagerClass, clearCookiesMethod);

  }

 protected:
  bool initialize() { return true; }


 private:
	std::string interfaceName;
	std::string userAgent;
	bool cookieEnable;
	bool keepaliveEnable;

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


};

#endif
