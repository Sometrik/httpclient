#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include "Authorization.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPClientInterface.h"
#include "HTTPClient.h"
#include <jni.h>

#include <map>
#include <string>

class AndroidClient : public HTTPClient {
 public:


	AndroidClient(JNIEnv * _env) : HTTPClient(interfaceName, userAgent, cookieEnable, keepaliveEnable){


		androidInit();
	}

	void androidInit(){

		cookieManagerClass =  env->FindClass("android/webkit/CookieManager");
		httpClass = env->FindClass("java/net/HttpURLConnection");
		urlClass = env->FindClass("java/net/URL");

		urlConstructor =  env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
		OpenConnectionMethod = env->GetMethodID(httpClass, "openConnection", "()java/net/URLConnection;");
		SetRequestProperty = env->GetMethodID(httpClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
		SetRequestMethod = env->GetMethodID(httpClass, "setRequestMethod", "(Ljava/lang/String;)V");
		SetDoInputMethod = env->GetMethodID(httpClass, "setDoInput", "(Z)V");
		connectMethod = env->GetMethodID(httpClass, "connect", "()V");
		clearCookiesMethod =  env->GetMethodID(cookieManagerClass, "removeAllCookie", "()V");

	}

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth){

  	jobject url = env->NewObject(urlClass, urlConstructor, env->NewStringUTF(req.getURI().c_str()));

  	//This would normally be casted. How?
  	jobject firstConnection = env->CallObjectMethod(url, OpenConnectionMethod);
  	//jobject httpConnection = firstConnection.jcast(firstConnection, "java/net/HttpURLConnection", JNI_FALSE, JNI_FALSE);



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

  jclass urlClass;
  jmethodID urlConstructor;
  jmethodID OpenConnectionMethod;
  jmethodID SetRequestProperty;
  jmethodID SetRequestMethod;
  jmethodID SetDoInputMethod;
  jmethodID connectMethod;

  jclass httpClass;

};

#endif
