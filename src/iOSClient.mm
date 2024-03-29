#include "iOSClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>

#import <Foundation/Foundation.h>

using namespace std;
using namespace httpclient;

class iOSClient : public HTTPClient {
 public:
  iOSClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive) {
  
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
#if 0
    if (req.getURI().empty()) {
      return HTTPResponse(0, "empty URI");
    }
#endif

    NSString* url_string = [NSString stringWithUTF8String:req.getURI().c_str()];

    NSLog(@"iOSClient, request: %@", url_string);

    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url_string] 
				    cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
				    timeoutInterval:req.getConnectTimeout()
				    ];
    // [url_string release];
    
    [request setHTTPShouldHandleCookies:NO];
    [request setHTTPShouldUsePipelining:NO];

    map<string, string> combined_headers;
    for (auto & hd : default_headers) {
      combined_headers[hd.first] = hd.second;      
    }
    for (auto & hd : req.getHeaders()) {
      combined_headers[hd.first] = hd.second;
    }
    for (auto & hd : combined_headers) {
        NSString *n = [NSString stringWithUTF8String:hd.first.c_str()];
        NSString *v = [NSString stringWithUTF8String:hd.second.c_str()];
        [request setValue:v forHTTPHeaderField:n];
    }
  
    if (req.getMethod() == HTTPRequest::POST) {
      [request setHTTPMethod: @"POST"];
      NSString *post_size = [NSString stringWithFormat:@"%d", (int)req.getContent().size()];
      [request setValue:post_size forHTTPHeaderField:@"Content-Length"];
      // [post_size release];

      NSString *content_type = [NSString stringWithUTF8String:req.getContentType().c_str()];
      [request setValue:content_type forHTTPHeaderField:@"Content-Type"];
      // [content_type release];

      NSData * post_data = [NSData dataWithBytes:req.getContent().data() length:req.getContent().size()];
      [request setHTTPBody:post_data];
      // [post_data release];
    } else {
      [request setHTTPMethod: @"GET"];
    }

    const BasicAuth * basic = dynamic_cast<const BasicAuth *>(&auth);
    if (basic) {
#if 0
      string s = basic->getUsername();
      s += ':';
      s += basic->getPassword();
#endif
    } else {
      string auth_header = auth.createHeader();
      if (!auth_header.empty()) {
        NSString *n = [NSString stringWithUTF8String:auth.getHeaderName()];
        NSString *v = [NSString stringWithUTF8String:auth_header.c_str()];
        [request setValue:v forHTTPHeaderField:n];
      }
    }

    // set follow location
    // set cookies
    NSString* user_agent_string = [NSString stringWithUTF8String: user_agent.c_str()];
    [request setValue:user_agent_string forHTTPHeaderField:@"User-Agent"];
    // [user_agent_string release];
    
    NSError *requestError = nil;
    NSURLResponse *urlResponse = nil;
    NSData *responseData = [NSURLConnection sendSynchronousRequest:request
			    returningResponse:&urlResponse error:&requestError];
    int result_code = 0;
    if (requestError) {
      result_code = [requestError code];
    } else if ([urlResponse isKindOfClass:[NSHTTPURLResponse class]]) {
      result_code = [(NSHTTPURLResponse *)urlResponse statusCode];
    }
    
    callback.handleResultCode(result_code);
    callback.handleChunk([responseData length], (const char *)[responseData bytes]);
    callback.handleDisconnect();

    // get redirect_url
    // NSDictionary* headers = [(NSHTTPURLResponse *)response allHeaderFields];
    
    // [request release];
    // [responseData release];
    // [requestError release];
    // [urlResponse release];

    NSLog(@"iOSClient, request done, result_code = %d", result_code);
  }

  void clearCookies() override {

  }
};

std::unique_ptr<HTTPClient>
iOSClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<iOSClient>(new iOSClient(_user_agent, _enable_cookies, _enable_keepalive));
}
