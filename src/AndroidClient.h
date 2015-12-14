#ifndef _ANDROIDCLIENT_H_
#define _ANDROIDCLIENT_H_

#include "Authorization.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPClientInterface.h"
#include "HTTPClient.h"

#include <map>
#include <string>

class AndroidClient : public HTTPClient {
 public:


	AndroidClient() : HTTPClient(interface, userAgent, cookieEnable, keepaliveEnable){


	}

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth){


  }

  void clearCookies() {


  }

 protected:
  bool initialize() { return true; }


 private:
	std::string interface;
	std::string userAgent;
	bool cookieEnable;
	bool keepaliveEnable;


};

#endif
