#include "iOSCFClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>
#include <CFNetwork/CFHTTPStream.h>

using namespace std;

class iOSCFClient : public HTTPClient {
 public:
  iOSCFClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive) {
  
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
    cerr << "loading " << req.getURI().c_str() << endl;
      
    CFStringRef url_string = CFStringCreateWithCString(NULL, req.getURI().c_str(), kCFStringEncodingUTF8);
    CFURLRef cfUrl = CFURLCreateWithString(kCFAllocatorDefault, url_string, NULL);
    
    if (!cfUrl) cerr << "could not create url" << endl;

    map<string, string> combined_headers;
    combined_headers["User-Agent"] = user_agent;
    
    string auth_header = auth.createHeader();
    if (!auth_header.empty()) {
      combined_headers[auth.getHeaderName()] = auth_header;
    }
    
    CFHTTPMessageRef cfHttpReq;
    if (req.getType() == HTTPRequest::POST) {
      cerr << "POST" << endl;
      cfHttpReq = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("POST"), cfUrl, kCFHTTPVersion1_1);

      combined_headers["Content-Length"] = to_string(req.getContent().size());
      combined_headers["Content-Type"] = req.getContentType();
      
      CFDataRef body = CFDataCreate(0, (const UInt8 *)req.getContent().data(), (CFIndex)req.getContent().size());
      CFHTTPMessageSetBody(cfHttpReq, body);
      CFRelease(body);
    } else {
      cfHttpReq = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("GET"), cfUrl, kCFHTTPVersion1_1);
    }

    if (!cfHttpReq) cerr << "failed to create request" << endl;
    
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
    
    if (req.getFollowLocation()) {
      CFReadStreamSetProperty(readStream, kCFStreamPropertyHTTPShouldAutoredirect, kCFBooleanTrue);
    }
    
    if (!CFReadStreamOpen(readStream)) {
      cerr << "failed to open stream" << endl;
    }
  
    bool headersReceived = false;
    CFIndex numBytesRead;    
    bool terminate = false;
    
    while (!terminate) {
      const int nBuffSize = 1024;
      UInt8 buff[nBuffSize];
      numBytesRead = CFReadStreamRead(readStream, buff, nBuffSize);
 
      if (!headersReceived) {
        headersReceived = true;
        CFHTTPMessageRef myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(readStream, kCFStreamPropertyHTTPResponseHeader);
        
        if (myResponse) {
          // CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine(myResponse);
          int result_code = (int)CFHTTPMessageGetResponseStatusCode(myResponse);
        
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
          
                if (result_code >= 300 && result_code <= 399 && strcmp(keyStr, "Location") == 0 && !callback.handleRedirectUrl(valStr)) {
                  terminate = true;
                }
              }
            }
            free(keys);
            free(values);
          }
        }
      }

      if (numBytesRead > 0) {
        // cerr << "sending bytes: " << nBuffSize << ": " << (char*)buff << endl;
        if (!callback.handleChunk(numBytesRead, (char *)buff)) {
          terminate = true;
        }
      } else if (numBytesRead < 0) {
        CFStreamError error = CFReadStreamGetError(readStream);
        cerr << "got error: " << error.error << endl;
        terminate = true;
      } else {
        terminate = true;
      }
    }
 
    CFReadStreamClose(readStream);
 
    CFRelease(url_string);
    CFRelease(cfUrl);
    CFRelease(cfHttpReq);
    CFRelease(readStream);

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
