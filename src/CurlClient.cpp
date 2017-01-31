#include "CurlClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cctype>
#include <iostream>
#include <pthread.h>

using namespace std;

#include <curl/curl.h>

static bool is_initialized = false;

class CurlClient : public HTTPClient {
 public:
  CurlClient(const std::string & _interface, const std::string & _user_agent, bool _enable_cookies = true, bool _enable_keepalive = true)
    : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive),
      interface_name(_interface)
  {
    assert(is_initialized);
  }

  ~CurlClient() {
    if (curl) curl_easy_cleanup(curl);
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
#if 0
    if (req.getURI().empty()) {
      return HTTPResponse(0, "empty URI");
    }
#endif
    if (!initialize()) {
      // return HTTPResponse(0, "Unable to initialize client");
      return;
    }

    // string ascii_uri = encodeIDN(uri);
    // if (ascii_uri.empty()) {
    //   cerr << "failed to convert url " << uri << endl;
    //   assert(0);
    // }

    struct curl_slist * headers = 0;
    if (req.getType() == HTTPRequest::POST) {
      if (!req.getContentType().empty()) {
	string h = "Content-type: ";
	h += req.getContentType();
	headers = curl_slist_append(headers, h.c_str());
      }
    }
    
    const BasicAuth * basic = dynamic_cast<const BasicAuth *>(&auth);
    if (basic) {
      string s = basic->getUsername();
      s += ':';
      s += basic->getPassword();
      curl_easy_setopt(curl, CURLOPT_USERPWD, s.c_str());
    } else {
      string auth_header = auth.createHeader();
      if (!auth_header.empty()) {
	string s = auth.getHeaderName();
	s += ": ";
	s += auth_header;
	headers = curl_slist_append(headers, s.c_str());
      }
    }

    map<string, string> combined_headers;
    for (auto & hd : default_headers) {
      combined_headers[hd.first] = hd.second;      
    }
    for (auto & hd : req.getHeaders()) {
      combined_headers[hd.first] = hd.second;
    }
    for (auto & hd : combined_headers) {
      string s = hd.first;
      s += ": ";
      s += hd.second;
      headers = curl_slist_append(headers, s.c_str());
    }
    
    if (req.getType() == HTTPRequest::POST) {
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 0);
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req.getContent().size());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.getContent().data());
    } else {
      curl_easy_setopt(curl, CURLOPT_POST, 0);
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    }
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, req.getFollowLocation());
    curl_easy_setopt(curl, CURLOPT_URL, req.getURI().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, req.getTimeout());
    
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &callback);

    CURLcode res = curl_easy_perform(curl);
    // if (res != 0) {
    //   response.setResultCode(500);
    //   response.setErrorText(curl_easy_strerror(res));
    // }
    
    callback.handleDisconnect();
    
    if (headers) {
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, 0);
      curl_slist_free_all(headers);
    }
    
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, 0);
    curl_easy_setopt(curl, CURLOPT_USERPWD, 0);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, 0);
  }
  
  void clearCookies() override {
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, "ALL"); // what does this do?
    }
  }
  
 protected:
  bool initialize() {
    if (!curl) {
      curl = curl_easy_init();
      assert(curl);
      if (curl) {
#ifndef WIN32
	if (!interface_name.empty()) {
	  int r = curl_easy_setopt(curl, CURLOPT_INTERFACE, interface_name.c_str());
	  assert(r != CURLE_INTERFACE_FAILED);
	}
#endif
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_func);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headers_func);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 600);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");

	if (!cookie_jar.empty()) {
	  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());
	  curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_jar.c_str());
	} else if (enable_cookies) {
	  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
	}
	if (!enable_keepalive) {
	  curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1);
	  curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
	  // curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 10);
	}
      }
    }
    
    return curl != 0;
  }

 private:
  static size_t write_data_func(void * buffer, size_t size, size_t nmemb, void *userp);
  static size_t headers_func(void * buffer, size_t size, size_t nmemb, void *userp);
  static int progress_func(void *  clientp, double dltotal, double dlnow, double ultotal, double ulnow);

  std::string interface_name;
  CURL * curl = 0;
};
  
size_t
CurlClient::write_data_func(void * buffer, size_t size, size_t nmemb, void * userp) {
  size_t s = size * nmemb;
  // Content-Type: multipart/x-mixed-replace; boundary=myboundary
					     
  HTTPClientInterface * callback = (HTTPClientInterface *)(userp);
  assert(callback);
  return callback->handleChunk(s, (const char *)buffer) ? s : 0;  
}

size_t
CurlClient::headers_func(void * buffer, size_t size, size_t nmemb, void *userp) {
  HTTPClientInterface * callback = (HTTPClientInterface *)(userp);
  
  size_t s = size * nmemb;
  bool keep_running = true;
  if (buffer) {
    string input((const char*)buffer, s);

    if (input.compare(0, 5, "HTTP/") == 0) {
      auto pos1 = input.find_first_of(' ');
      if (pos1 != string::npos) {
	auto pos2 = input.find_first_of(' ', pos1 + 1);
	if (pos2 != string::npos) {
	  auto s2 = input.substr(pos1, pos2 - pos1);
	  int code = atoi(s2.c_str());
	  callback->handleResultCode(code);
	}
      }
    } else {
      int pos1 = 0;
      for ( ; pos1 < input.size() && input[pos1] != ':'; pos1++) { }
      int pos2 = input[pos1] == ':' ? pos1 + 1 : pos1;
      for (; pos2 < input.size() && isspace(input[pos2]); pos2++) { }
      int pos3 = input.size();
      for (; pos3 > 0 && isspace(input[pos3 - 1]); pos3--) { }
      std::string key = input.substr(0, pos1);
      std::string value = input.substr(pos2, pos3 - pos2);

      if (key == "Location" && !callback->handleRedirectUrl(value)) {
	keep_running = false;
      }      
      callback->handleHeader(key, value);
    }
  }

  return keep_running ? s : 0;
}

int
CurlClient::progress_func(void * clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
  HTTPClientInterface * callback = (HTTPClientInterface *)(clientp);
  assert(callback);
  return callback->onIdle(true) ? 0 : 1;
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
  ret = (unsigned long)pthread_self();
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

std::unique_ptr<HTTPClient>
CurlClientFactory::createClient(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<CurlClient>(new CurlClient("", _user_agent, _enable_cookies, _enable_keepalive));
}

void
CurlClientFactory::globalInit() {
  is_initialized = true;
  
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
CurlClientFactory::globalCleanup() {
  is_initialized = false;
  
  curl_global_cleanup();
#ifndef __APPLE__
  kill_locks();
#endif
}
