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


	AndroidClient(JNIEnv * _env) : HTTPClient(interface, userAgent, cookieEnable, keepaliveEnable){


		androidInit();
	}

	void androidInit(){

		cookieManagerClass =  env->FindClass("android/webkit/CookieManager");
		clearCookiesMethod =  env->GetMethodID(cookieManagerClass, "removeAllCookie", "()V");

	}

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth){




  }

  void clearCookies() {

  	env->CallVoidMethod(cookieManagerClass, clearCookiesMethod);

  }

 protected:
  bool initialize() { return true; }


 private:
	std::string interface;
	std::string userAgent;
	bool cookieEnable;
	bool keepaliveEnable;

  JNIEnv * env;
  jclass cookieManagerClass;
  jmethodID clearCookiesMethod;


};

#endif
