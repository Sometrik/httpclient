#include "CurlClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>
#include <pthread.h>

using namespace std;

CurlClient::CurlClient(const string & _interface,  const string & _user_agent, bool _enable_cookies, bool _enable_keepalive)
  : HTTPClient(_user_agent, _enable_cookies, _enable_keepalive),
    interface_name(_interface)
{
  
}

CurlClient::CurlClient(const CurlClient & other)
  : HTTPClient(other),
    interface_name(other.interface_name)
{
  
}

CurlClient::~CurlClient() {
  if (curl) curl_easy_cleanup(curl);
}

bool
CurlClient::initialize() {
  if (curl) {
    curl_easy_reset(curl);
    data_in.clear();
    data_in.shrink_to_fit();
    data_out.clear();
    data_out.shrink_to_fit();
  } else {
    curl = curl_easy_init();
  }
  
  assert(curl);

  if (curl) {
#ifndef WIN32
    if (!interface_name.empty()) {
      int r = curl_easy_setopt(curl, CURLOPT_INTERFACE, interface_name.c_str());
      assert(r != CURLE_INTERFACE_FAILED);
    }
#endif
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_func);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 600);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    if (!enable_keepalive) {
      curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1);
      curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
    }
    if (!cookie_jar.empty()) {
      curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());
      curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_jar.c_str());
    } else if (enable_cookies) {
      curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
    }
    // curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 10);

  }

  return curl != 0;
}

HTTPResponse
CurlClient::request(const HTTPRequest & req, const Authorization & auth) {
  if (req.getURI().empty()) {
    return HTTPResponse(0, "empty URI");
  }
  if (!initialize()) {
    return HTTPResponse(0, "Unable to initialize client");
  }

  // string ascii_uri = encodeIDN(uri);
  // if (ascii_uri.empty()) {
  //   cerr << "failed to convert url " << uri << endl;
  //   assert(0);
  // }
  
  struct curl_slist * headers = 0;
  if (req.getType() == HTTPRequest::POST) {
    data_out = req.getContent();

    if (!req.getContentType().empty()) {
      string h = "Content-type: ";
      h += req.getContentType();
      headers = curl_slist_append(headers, h.c_str());
    }
  } else {
    data_out.clear();
  }

  const BasicAuth * basic = dynamic_cast<const BasicAuth *>(&auth);
  if (basic) {
    string s = basic->getUsername();
    s += ':';
    s += basic->getPassword();
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
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

  for (auto & hd : req.getHeaders()) {
    string s = hd.first;
    s += ": ";
    s += hd.second;
    headers = curl_slist_append(headers, s.c_str());
  }
  
  if (req.getType() == HTTPRequest::POST) {
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data_out.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_out.size());
  } else {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
  }
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, req.getFollowLocation());
  curl_easy_setopt(curl, CURLOPT_URL, req.getURI().c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, req.getTimeout());
  if (!cookie_jar.empty()) {
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_jar.c_str());
  }

  CURLcode res = curl_easy_perform(curl);
  int result_code = 0;
  string redirect_url;
  string error_text;
  if (res == 0) {
    unsigned long a = 0;
    if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &a) == CURLE_OK) {
      result_code = a;
    }
    if (!req.getFollowLocation()) {
      char * ptr = 0;
      curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &ptr);
      if (ptr) {
	redirect_url = ptr;
      }
    }
  } else {
    error_text = curl_easy_strerror(res);
  }

  if (headers) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, 0);
    curl_slist_free_all(headers);
  }
  
  HTTPResponse response(result_code, error_text, redirect_url, data_in);
  data_in.clear();
  return response;
}

bool
CurlClient::addToData(size_t len, const char * data) {
  if (callback) {
    return callback->handleChunk(len, data);
  } else {
    data_in += string(data, len);
    return true;
  }  
}

bool
CurlClient::handleProgress(double dltotal, double dlnow, double ultotal, double ulnow) {
  if (callback) {
    return callback->onIdle(true);
  }
  return true;
}

size_t
CurlClient::write_data_func(void * buffer, size_t size, size_t nmemb, void * userp) {
  CurlClient * client = static_cast<CurlClient *>(userp);
  assert(client);
  size_t s = size * nmemb;
  if (client->addToData((int)s, (char *)buffer)) {
    return s;
  } else {
    return 0;
  }
}

int
CurlClient::progress_func(void * clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
  CurlClient * client = static_cast<CurlClient *>(clientp);
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
CurlClient::globalInit() {
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
CurlClient::clearCookies() {
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "ALL");
  }
}

void
CurlClient::globalCleanup() {
  curl_global_cleanup();
#ifndef __APPLE__
  kill_locks();
#endif
}

