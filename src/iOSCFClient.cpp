#include "iOSCFClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>
#include <CFNetwork/CFHTTPStream.h>

#define _kCFStreamPropertyReadTimeout CFSTR("_kCFStreamPropertyReadTimeout")

using namespace std;

class iOSCFClient : public HTTPClient {
 public:
  iOSCFClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive) {
  
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
    CFStringRef url_string = CFStringCreateWithCString(NULL, req.getURI().c_str(), kCFStringEncodingUTF8);
    CFURLRef cfUrl = CFURLCreateWithString(kCFAllocatorDefault, url_string, NULL);
    
    if (!cfUrl) {
      // url_string is needed here, do not release yet
      CFStringRef url_string2 = CFURLCreateStringByAddingPercentEscapes(NULL, url_string, NULL, NULL, kCFStringEncodingUTF8);
      cfUrl = CFURLCreateWithString(kCFAllocatorDefault, url_string2, NULL);    
      CFRelease(url_string2);
    }

    CFRelease(url_string);

    if (!cfUrl) {
      callback.handleResultCode(0);
      callback.handleDisconnect();
      return;
    }
    
    map<string, string> combined_headers;
    combined_headers["User-Agent"] = user_agent;

    if (enable_keepalive) {
      // combined_headers["Connection"] = "Keep-Alive";
      combined_headers["Keep-Alive"] = "30";
    }
    
    string auth_header = auth.createHeader();
    if (!auth_header.empty()) {
      combined_headers[auth.getHeaderName()] = auth_header;
    }
    
    CFHTTPMessageRef cfHttpReq;
    if (req.getType() == HTTPRequest::POST) {
      cfHttpReq = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("POST"), cfUrl, kCFHTTPVersion1_1);

      combined_headers["Content-Length"] = to_string(req.getContent().size());
      combined_headers["Content-Type"] = req.getContentType();
      
      CFDataRef body = CFDataCreate(0, (const UInt8 *)req.getContent().data(), (CFIndex)req.getContent().size());
      CFHTTPMessageSetBody(cfHttpReq, body);
      CFRelease(body);
    } else {
      cfHttpReq = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("GET"), cfUrl, kCFHTTPVersion1_1);
    }
    
    for (auto & hd : default_headers) {
      combined_headers[hd.first] = hd.second;      
    }
    for (auto & hd : req.getHeaders()) {
      combined_headers[hd.first] = hd.second;
    }
    for (auto & hd : combined_headers) {
      CFStringRef key = CFStringCreateWithCString(NULL, hd.first.c_str(), kCFStringEncodingUTF8);
      CFStringRef value = CFStringCreateWithCString(NULL, hd.second.c_str(), kCFStringEncodingUTF8);
      CFHTTPMessageSetHeaderFieldValue(cfHttpReq, key, value);
      CFRelease(key);
      CFRelease(value);
    }

    CFReadStreamRef readStream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, cfHttpReq);

    CFReadStreamSetProperty(readStream, kCFStreamPropertyHTTPAttemptPersistentConnection, enable_keepalive ? kCFBooleanTrue : kCFBooleanFalse);

    if (req.getReadTimeout()) {
      double timeout = req.getReadTimeout();
      CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &timeout);
      CFReadStreamSetProperty(readStream, _kCFStreamPropertyReadTimeout, number);
      CFRelease(number);
    }
    
    if (req.getFollowLocation()) {
      CFReadStreamSetProperty(readStream, kCFStreamPropertyHTTPShouldAutoredirect, kCFBooleanTrue);
    }

    if (strncasecmp(req.getURI().c_str(), "https", 5) == 0) {
      CFMutableDictionaryRef pDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
      CFDictionarySetValue(pDict, kCFStreamSSLValidatesCertificateChain, kCFBooleanFalse);
      // CFDictionarySetValue(pDict, kCFStreamPropertySocketSecurityLevel, kCFStreamSocketSecurityLevelSSLv3);
      CFReadStreamSetProperty(readStream, kCFStreamPropertySSLSettings, pDict);
      CFRelease(pDict);
    }
    
    if (!CFReadStreamOpen(readStream)) {
      // fail
    }
  
    bool headersReceived = false;
    bool terminate = false;
    int result_code = 0;
    string redirectUrl;
    
    while (!terminate) {
      const int nBuffSize = 4096;
      UInt8 buff[nBuffSize];
      CFIndex numBytesRead = CFReadStreamRead(readStream, buff, nBuffSize);
 
      if (!headersReceived) {
        headersReceived = true;
        CFHTTPMessageRef myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(readStream, kCFStreamPropertyHTTPResponseHeader);
        
        if (myResponse) {
          // CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine(myResponse);
          result_code = (int)CFHTTPMessageGetResponseStatusCode(myResponse);
        
          callback.handleResultCode(result_code);
	  
          CFDictionaryRef headerFields = CFHTTPMessageCopyAllHeaderFields(myResponse);
          auto size = CFDictionaryGetCount(headerFields);
          if (size > 0) {
            CFTypeRef *keys = (CFTypeRef *)malloc(size * sizeof(CFTypeRef));
            CFStringRef *values = (CFStringRef *)malloc(size * sizeof(CFStringRef));
            CFDictionaryGetKeysAndValues(headerFields, (const void **)keys, (const void **)values);
            for (unsigned int i = 0; i < size; i++) {
              const char * keyStr = CFStringGetCStringPtr((CFStringRef)keys[i], kCFStringEncodingUTF8);
              const char * valStr = CFStringGetCStringPtr((CFStringRef)values[i], kCFStringEncodingUTF8);
          
              if (keyStr && valStr) {
                callback.handleHeader(keyStr, valStr);
          
                if (result_code >= 300 && result_code <= 399 && strcasecmp(keyStr, "location") == 0) {
		  redirectUrl = valStr;
		  if (!callback.handleRedirectUrl(redirectUrl)) {
		    terminate = true;
		  }
                }
              }
            }
            free(keys);
            free(values);
          }
	  CFRelease(headerFields);
	  CFRelease(myResponse);
        }
      }

      if (numBytesRead > 0) {
        if (!callback.handleChunk(numBytesRead, (char *)buff)) {
          terminate = true;
        }
      } else if (numBytesRead < 0) {
        CFStreamError error = CFReadStreamGetError(readStream);
        terminate = true;
      } else {
        terminate = true;
      }
    }
 
    CFReadStreamClose(readStream);
 
    CFRelease(cfUrl);
    CFRelease(cfHttpReq);
    CFRelease(readStream);

    // cerr << "loaded " << req.getURI().c_str() << " (" << result_code << ", target = " << redirectUrl << ")\n";

#if 0
    const BasicAuth * basic = dynamic_cast<const BasicAuth *>(&auth);
    if (basic) {
      string s = basic->getUsername();
      s += ':';
      s += basic->getPassword();
    }
#endif
    
    callback.handleDisconnect();
  }

  void clearCookies() override {

  }
};

std::unique_ptr<HTTPClient>
iOSCFClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<iOSCFClient>(new iOSCFClient(_user_agent, _enable_cookies, _enable_keepalive));
}
