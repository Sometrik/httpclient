#include "WinHTTPClient.h"

#include "BasicAuth.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cctype>

#if defined(WIN32) || defined(WIN64) 
#include <windows.h>
#include <sysinfoapi.h>

#define strcasecmp _stricmp
#endif

#include <iostream>

using namespace std;

#include <sys/time.h>
#include <winhttp.h>

static bool is_initialized = false;

static wstring from_utf8(const std::string & s) {
  wchar_t tmp[s.size() + 1];
  auto r = MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, tmp, s.size() + 1);
  return wstring(tmp, static_cast<size_t>(r));
}

static string to_utf8(const wstring & s) {
  char tmp[4*s.size()+1];
  auto r = WideCharToMultiByte(CP_UTF8,
			       0
			       s.data(),
			       -1,
			       tmp,
			       4*s.size() + 1,
			       NULL,
			       NULL);
  return string(tmp, static_cast<size_t>(r));  
}

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

class WinHTTPClient : public HTTPClient {
 public:
  WinHTTPClient(const std::string & user_agent, bool enable_cookies = true, bool enable_keepalive = true)
    : HTTPClient(user_agent, enable_cookies, enable_keepalive),
  {
    assert(is_initialized_);
  }

  ~WinHTTPClient() {
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
    if (!initialize()) {
      return;
    }

    URI uri(req.getURI());
    
    auto ua_text = from_utf8(getUserAgent());
    auto target_scheme = uri.getScheme();
    auto target_host = from_utf8(uri.getDomain());
    auto target_port = uri.getPort();
    auto target_path = uri.getPath();

    bool is_secure = false;
    if (!target_port) {
      if (target_scheme == "http") {
	target_port = INTERNET_DEFAULT_HTTP_PORT;
      } else if (target_scheme == "https") {
	target_port = INTERNET_DEFAULT_HTTPS_PORT;
	is_secure = true;
      } else {
	return;
      }
    }
        
    // Use WinHttpOpen to obtain a session handle.
    auto hSession = WinHttpOpen(ua_text,
				WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				WINHTTP_NO_PROXY_NAME,
				WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
      // Specify an HTTP server.
      auto hConnect = WinHttpConnect(hSession, target_host, target_port, 0);
      if (hConnect) {
	wstring request_type;
	bool has_data = true;
	switch (req.getType()) {
	case HTTPRequest::GET:
	  request_type = L"GET";
	  has_data = false;
	  break;
	case HTTPRequest::POST:
	  request_type = L"POST";
	  break;
	case HTTPRequest::OPTIONS:
	  request_type = L"OPTIONS";
	  break;
	}    

	DWORD dwFlags = WINHTTP_FLAG_ESCAPE_DISABLE | WINHTTP_FLAG_ESCAPE_DISABLE_QUERY;
	if (is_secure) dwFlags |= WINHTTP_FLAG_SECURE;
	  
	auto hRequest = WinHttpOpenRequest(hConnect, request_type, target_path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
	if (hRequest) {
	  map<string, string> combined_headers;
	  for (auto & hd : default_headers) {
	    combined_headers[hd.first] = hd.second;      
	  }
	  for (auto & hd : req.getHeaders()) {
	    combined_headers[hd.first] = hd.second;
	  }
	  
	  if (req.getType() == HTTPRequest::POST) {
	    if (!req.getContentType().empty()) {
	      combined_headers["Content-Type"] = req.getContentType();
	    }
	  }
	  
	  for (auto & [ name, value ] : combined_headers) {
	    auto header = from_utf8(name + ": " + value + "\r\n");
	    WinHttpAddRequestHeaders(hRequest, header.c_str(), header.length(), WINHTTP_ADDREQ_FLAG_ADD);
	  }
	  
	  DWORD dwSize = 0;
	  DWORD dwDownloaded = 0;
	  LPSTR pszOutBuffer;

	  if (has_data) {
	    dwSize = req.getContent().size();
	  }
	  
	  if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, dwSize, 0) &&
	      (!has_data || WinHttpWriteData(hRequest, req.getContent().data(), dwSize, NULL)) &&
	      WinHttpReceiveResponse(hRequest, NULL)
	      ) {
	    
	    DWORD headerSize = 0;

	    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &headerSize, WINHTTP_NO_HEADER_INDEX)) {
	      // Allocate buffer by header length
	      // If the function fails and ERROR_INSUFFICIENT_BUFFER is returned, lpdwBufferLength specifies the number of bytes that the application must allocate to receive the string.
	      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		lpHeadBuffer = new wchar_t[dwSize / sizeof(wchar_t)];
		bResults = WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_RAW_HEADERS_CRLF,WINHTTP_HEADER_NAME_BY_INDEX, lpHeadBuffer, &dwSize,WINHTTP_NO_HEADER_INDEX);
	      }
	      // Get Content-Type from header to specify encoding
	      if ( NULL != wcsstr(lpHeadBuffer, L"charset=gbk") )
		{
		  bUTF8Code = FALSE;
		}
	    }
	    else
	      {
		cout << "Get header failed!" << endl;
	      }
	  }
    
#if 0
	    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, req.getFollowLocation() ? 1 : 0);
	    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, req.getConnectTimeout());
	    
	    long long current_time_ms = get_current_time_ms();
	    curl_context_s context = { current_time_ms, current_time_ms, 0, req.getReadTimeout(), req.getConnectionTimeout(), &callback };
	    
	    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &context);
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &context);
	    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &context);
#endif
	    
	    callback.handleDisconnect();
	  }
	  
	  WinHttpCloseHandle(hRequest);
	}
	WinHttpCloseHandle(hConnect);
      }
      WinHttpCloseHandle(hSession);
    }
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
  

std::unique_ptr<HTTPClient>
WinHTTPClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<WinHTTPClient>(new WinHTTPClient("", _user_agent, _enable_cookies, _enable_keepalive));
}

void
WinHTTPClientFactory::globalInit() {
  is_initialized = true;
}

void
WinHTTPClientFactory::globalCleanup() {
  is_initialized = false;
}
