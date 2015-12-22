#include "iOSClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>

using namespace std;

class iOSClient : public HTTPClient {
 public:
  iOSClient(const std::string & _interface, const std::string & _user_agent, bool enable_cookies, bool enable_keepalive)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive) {
  
  }
  iOSClient(const iOSClient & other) : HTTPClient(other) { }
  ~iOSClient() { }

  HTTPResponse request(const HTTPRequest & req, const Authorization & auth) override {
    if (req.getURI().empty()) {
      return HTTPResponse(0, "empty URI");
    }
    if (!initialize()) {
      return HTTPResponse(0, "Unable to initialize client");
    }

    NSString* url_string = [NSString stringWithUTF8String:req.getURI().c_str()];

    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url_string] 
				    cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData
				    timeoutInterval:req.getTimeout()
				    ];
    [url_string release];
    
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

    int result_code = [response statusCode];
    string content([responseData bytes], [responseData length]);
    // get redirect_url
    
    [request release];
    // release requestError, urlResponse and responseData
        
    return HTTPResponse(result_code, "", "", content);
  }

  void clearCookies() override {

  }
  
 protected:
  bool initialize() {

  }

 private:
  CURL * curl = 0;
};

bool
iOSClient::addToData(size_t len, const char * data) {
  if (callback) {
    return callback->handleChunk(len, data);
  } else {
    data_in += string(data, len);
    return true;
  }  
}

bool
iOSClient::handleProgress(double dltotal, double dlnow, double ultotal, double ulnow) {
  if (callback) {
    return callback->onIdle(true);
  }
  return true;
}

size_t
iOSClient::write_data_func(void * buffer, size_t size, size_t nmemb, void * userp) {
  iOSClient * client = static_cast<iOSClient *>(userp);
  assert(client);
  size_t s = size * nmemb;
  if (client->addToData((int)s, (char *)buffer)) {
    return s;
  } else {
    return 0;
  }
}

int
iOSClient::progress_func(void * clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
  iOSClient * client = static_cast<iOSClient *>(clientp);
  assert(client);
  if (!client->handleProgress(dltotal, dlnow, ultotal, ulnow)) {
    return 1;
  } else {
    return 0;
  }
}

#ifndef __APPLE__
static pthread_mutex_t *lockarray;

#include <openssl/crypto.h>

static void lock_callback(int mode, int type, const char *file, int line) {
  (void)file;
  (void)line;
  if (mode & CRYPTO_LOCK) {
    pthread_mutex_lock(&(lockarray[type]));
  } else {
    pthread_mutex_unlock(&(lockarray[type]));
  }
}
 
static unsigned long thread_id(void) {
  unsigned long ret;
 
#ifdef WIN32
  ret = (unsigned long)(pthread_self().p);
#else
  ret=(unsigned long)pthread_self();
#endif
  return(ret);
}
 
static void init_locks(void) {
  int i;
 
  lockarray=(pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() *
					      sizeof(pthread_mutex_t));
  for (i=0; i<CRYPTO_num_locks(); i++) {
    pthread_mutex_init(&(lockarray[i]),NULL);
  }
 
  CRYPTO_set_id_callback((unsigned long (*)())thread_id);
  CRYPTO_set_locking_callback(lock_callback); // (void (*)())lock_callback);
}
 
static void kill_locks(void)
{
  int i;
 
  CRYPTO_set_locking_callback(NULL);
  for (i=0; i<CRYPTO_num_locks(); i++)
    pthread_mutex_destroy(&(lockarray[i]));
 
  OPENSSL_free(lockarray);
}
#endif

#if 0
#include <gcrypt.h>
#include <errno.h>
#include <gnutls/gnutls.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

void
iOSClient::globalInit() {
#ifndef __APPLE__
#if 0
  gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
  gnutls_global_init();
#else
  init_locks();
#endif
#endif

  curl_global_init(CURL_GLOBAL_DEFAULT);

#if 0
  thread_setup();
#endif
#if 0
  gnutls_global_init();
#endif
}

void
iOSClient::clearCookies() {
	// implement
}

std::shared_ptr<HTTPClient>
iOSClientFactory::createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::make_shared<iOSClient>(_user_agent, _enable_cookies, _enable_keepalive);
}
