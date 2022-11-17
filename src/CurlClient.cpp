
#include "CurlClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cctype>

#include <pthread.h>

using namespace std;

#include <curl/curl.h>
#include <sys/time.h>

static bool is_initialized = false;
static CURLSH * share;
static pthread_mutex_t *share_lockarray;

struct log_context_s {
  std::string log;
};
  
static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp) {
  auto context = reinterpret_cast<log_context_s*>(userp);

  std::string text;
  
  switch(type) {
  case CURLINFO_TEXT:
    text = string("== Info: ") + data;
    break;
  case CURLINFO_HEADER_OUT:
    text = "=> Send header\n";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data\n";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data\n";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header\n";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data\n";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data\n";
    break;
  default: /* in case a new one is introduced to shock us */
    return 0;
  }

  context->log += text;
  
  // dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
  
  return 0;
}

class CurlClient;

struct curl_context_s {
  long long connection_time;
  long long prev_data_time;
  long long prev_idle_time;
  int read_timeout, connection_timeout;
  HTTPClientInterface * callback;
};

static long long get_current_time_ms() {
#ifdef WIN32
  FILETIME ft_now;
  GetSystemTimeAsFileTime(&ft_now);
  long long ll_now = (LONGLONG)ft_now.dwLowDateTime + (((LONGLONG)ft_now.dwHighDateTime) << 32LL);
  ll_now /= 10000;
  return ll_now - 116444736000000000LL;
#else
  struct timeval tv;
  int r = gettimeofday(&tv, 0);
  if (r == 0) {
    return (long long)1000 * tv.tv_sec + tv.tv_usec / 1000;
  } else {
    return 0;
  }
#endif
}

class CurlClient : public HTTPClient {
 public:
  CurlClient(std::string _interface, std::string _user_agent, bool _enable_cookies = true, bool _enable_keepalive = true)
    : HTTPClient(std::move(_user_agent), _enable_cookies, _enable_keepalive),
      interface_name(std::move(_interface))
  {
    assert(is_initialized);
  }

  ~CurlClient() {
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
      curl_easy_cleanup(curl);
    }
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
    log_context_s log_context;
    
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
    if (req.getType() == HTTPRequest::POST || req.getType() == HTTPRequest::OPTIONS) {
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
    
    if (req.getType() == HTTPRequest::POST || req.getType() == HTTPRequest::OPTIONS) {
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 0);
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req.getContent().size());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.getContent().data());
    } else {
      curl_easy_setopt(curl, CURLOPT_POST, 0);
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    }
    if (req.getType() == HTTPRequest::OPTIONS) {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    } else {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);
    }
    if (req.useHTTP2()) {
      curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE); // CURL_HTTP_VERSION_2TLS); // CURL_HTTP_VERSION_2_0);
      curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_3);
    } else {
      curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_NONE);
      curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT);
    }
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, req.getFollowLocation() ? 1 : 0);
    curl_easy_setopt(curl, CURLOPT_URL, req.getURI().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &log_context);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // cerr << "setting connect timeout " << req.getConnectTimeout() << ", read timeout " << req.getReadTimeout() << ", connection timeout = " << req.getConnectionTimeout() << endl;
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, req.getConnectTimeout());

    long long current_time_ms = get_current_time_ms();
    curl_context_s context = { current_time_ms, current_time_ms, 0, req.getReadTimeout(), req.getConnectionTimeout(), &callback };
      
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &context);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &context);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &context);

    char errbuf[CURL_ERROR_SIZE];
    errbuf[0] = 0;
    
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
     
    auto ret = curl_easy_perform(curl);
    if (ret != CURLE_OK) {
      size_t len = strlen(errbuf);
      if (len) callback.handleErrorText(errbuf);
      else {
	callback.handleErrorText(curl_easy_strerror(ret));
      }
    }
    
    callback.handleDisconnect();
    callback.handleLogText(log_context.log);
    
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
	if (!interface_name.empty()) {
	  int r = curl_easy_setopt(curl, CURLOPT_INTERFACE, interface_name.c_str());
	  assert(r != CURLE_INTERFACE_FAILED);
	}
	curl_easy_setopt(curl, CURLOPT_SHARE, share);
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_func);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headers_func);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 300);
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

  auto context = (curl_context_s *)userp;
  assert(context);
  context->prev_data_time = get_current_time_ms();
  
  auto callback = context->callback;
  assert(callback);
  return callback->handleChunk(std::string_view(reinterpret_cast<char *>(buffer), s)) ? s : 0;  
}

size_t
CurlClient::headers_func(void * buffer, size_t size, size_t nmemb, void *userp) {
  auto context = (curl_context_s *)userp;
  assert(context);
  context->prev_data_time = get_current_time_ms();
  
  size_t s = size * nmemb;
  bool keep_running = true;
  if (buffer) {
    auto callback = context->callback;

    auto v = std::string_view(reinterpret_cast<char *>(buffer), s);
    
    if (!callback->handleHeaderChunk(std::move(v))) {
      keep_running = false;
    }
  }

  return keep_running ? s : 0;
}

int
CurlClient::progress_func(void * clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
  long long current_time_ms = get_current_time_ms();
  curl_context_s * context = (curl_context_s *)clientp;
#if 0
  if (context && context->connection_timeout) {
    int d = context->connection_time + 1000 * context->connection_timeout - current_time_ms;
    // cerr << "checking connection timeout " << d << endl;
  }
#endif
  if (!context) {
    return 0; // do nothing
  } else if (context->connection_timeout && context->connection_time + 1000 * context->connection_timeout <= current_time_ms) {
    return 1;
  } else if (context->read_timeout && context->prev_data_time + 1000 * context->read_timeout <= current_time_ms) {
    return 1;
  } else if (context->prev_idle_time + 1000 <= current_time_ms) {
    context->prev_idle_time = current_time_ms;
    HTTPClientInterface * callback = context->callback;
    assert(callback);
    return callback->onIdle() ? 0 : 1;
  } else {
    return 0;
  }
}

static pthread_mutex_t *lockarray;

#if 0
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
  ret = (unsigned long)pthread_self();
  return(ret);
}
#endif

static void init_locks(void) {
#if 0
  int i;
 
  lockarray = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
  for (i = 0; i < CRYPTO_num_locks(); i++) {
    pthread_mutex_init(&(lockarray[i]),NULL);
  }
 
  CRYPTO_set_id_callback((unsigned long (*)())thread_id);
  CRYPTO_set_locking_callback(lock_callback); // (void (*)())lock_callback);
#endif
}

static void kill_locks(void)
{
#if 0
  int i;
 
  CRYPTO_set_locking_callback(NULL);
  for (i=0; i<CRYPTO_num_locks(); i++)
    pthread_mutex_destroy(&(lockarray[i]));
 
  OPENSSL_free(lockarray);
#endif
}

#if 0
#include <gcrypt.h>
#include <errno.h>
#include <gnutls/gnutls.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

std::unique_ptr<HTTPClient>
CurlClientFactory::createClient2(std::string user_agent, bool enable_cookies, bool enable_keepalive) {
  return std::unique_ptr<CurlClient>(new CurlClient("", std::move(user_agent), enable_cookies, enable_keepalive));
}

static void lock_cb(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) {
  assert(data >= 0 && data < 10);
  pthread_mutex_lock(&share_lockarray[data]); /* uses a global lock array */
}

static void unlock_cb(CURL *handle, curl_lock_data data, void *userptr) {
  assert(data >= 0 && data < 10);
  pthread_mutex_unlock(&share_lockarray[data]); /* uses a global lock array */
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

  share_lockarray = (pthread_mutex_t *)malloc(10 * sizeof(pthread_mutex_t));
  for (int i = 0; i < 10; i++) {
    pthread_mutex_init(&(share_lockarray[i]), NULL);
  }

  share = curl_share_init();
  // curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
  curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  // curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  // curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
  curl_share_setopt(share, CURLSHOPT_LOCKFUNC, lock_cb);
  curl_share_setopt(share, CURLSHOPT_UNLOCKFUNC, unlock_cb);
}

void
CurlClientFactory::globalCleanup() {
  is_initialized = false;
 
  curl_global_cleanup();
#ifndef __APPLE__
  kill_locks();
#endif
}
