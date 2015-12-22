#include "iOSClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>

using namespace std;

class iOSClient : public HTTPClient {
 public:
  iOSClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive) {
  
  }
  iOSClient(const iOSClient & other) : HTTPClient(other) { }
  ~iOSClient() { }

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth) override {
    if (req.getURI().empty()) {
      return HTTPResponse(0, "empty URI");
    }

    NSString* url_string = [NSString stringWithUTF8String:req.getURI().c_str()];

    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url_string] 
				    cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
				    timeoutInterval:req.getTimeout()
				    ];
    [url_string release];
    
    [request setHTTPShouldHandleCookies:NO];
    [request setHTTPShouldUsePipelining:NO];
    
    for (auto & hd : req.getHeaders()) {
    }
  
    if (req.getType() == HTTPRequest::POST) {
      [request setHTTPMethod: @"POST"];
      NSString *post_size = [NSString stringWithFormat:@"%d", (int)req.getContent().size()];
      [request setValue:post_size forHTTPHeaderField:@"Content-Length"];
      [post_size release];

      NSString *content_type = [NSString stringWithUTF8String:req.getContentType().c_str()];
      [request setValue:content_type forHTTPHeaderField:@"Content-Type"];
      [content_type release];

      NSData * post_data = [NSData dataWithBytes:req.getContent().data() length:req.getContent().size()];
      [request setHTTPBody:post_data];
      [post_data release];
    } else {
      [request setHTTPMethod: @"GET"];
    }
    // set follow location
    // set cookies
    NSString* user_agent_string = [NSString stringWithUTF8String: user_agent.c_str()];
    [request setValue:user_agent_string forHTTPHeaderField:@"User-Agent"];
    [user_agent_string release];
    
    NSError *requestError = nil;
    NSURLResponse *urlResponse = nil;
    NSData *responseData = [NSURLConnection sendSynchronousRequest:request
			    returningResponse:&urlResponse error:&requestError];

    int result_code = [urlResponse statusCode];
    string content((const char *)[responseData bytes], [responseData length]);
    // get redirect_url
    // NSDictionary* headers = [(NSHTTPURLResponse *)response allHeaderFields];
    
    [request release];
    [responseData release];
    [requestError release];
    [urlResponse release];

    return HTTPResponse(result_code, "", "", content);
  }

  void clearCookies() override {

  }
};

std::shared_ptr<HTTPClient>
iOSClientFactory::createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::make_shared<iOSClient>(_user_agent, _enable_cookies, _enable_keepalive);
}
