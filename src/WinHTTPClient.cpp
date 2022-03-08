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
#include <URI.h>
#include <winhttp.h>

using namespace std;

static wstring from_utf8(const std::string & s) {
  std::unique_ptr<wchar_t[]> tmp(new wchar_t[s.size() + 1]);
  auto r = MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, tmp.get(), s.size() + 1);
  return wstring(tmp.get(), static_cast<size_t>(r));
}

static string to_utf8(const wstring & s) {
  std::unique_ptr<char[]> tmp(new char[4 * s.size() + 1]);
  auto r = WideCharToMultiByte(CP_UTF8,
			       0,
			       s.data(),
			       -1,
			       tmp.get(),
			       4*s.size() + 1,
			       NULL,
			       NULL);
  return string(tmp.get(), static_cast<size_t>(r));  
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
    : HTTPClient(user_agent, enable_cookies, enable_keepalive)
  {
  }

  ~WinHTTPClient() {
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
    URI uri(req.getURI());
    
    auto ua_text = from_utf8(getUserAgent());
    auto target_scheme = uri.getScheme();
    auto target_host = from_utf8(uri.getDomain());
    auto target_port = uri.getPort();
    auto target_path = from_utf8(uri.getPath());

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
    auto hSession = WinHttpOpen(ua_text.c_str(),
				WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				WINHTTP_NO_PROXY_NAME,
				WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
      // Specify an HTTP server.
      auto hConnect = WinHttpConnect(hSession, target_host.c_str(), target_port, 0);
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
	  
	auto hRequest = WinHttpOpenRequest(hConnect, request_type.c_str(), target_path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
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
	    WinHttpAddRequestHeaders(hRequest, header.c_str(), -1, WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD);
	  }
	  
	  DWORD dwSize = 0;

	  if (has_data) {
	    dwSize = req.getContent().size();
	  }
	  
	  if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, dwSize, 0) &&
	      (!has_data || WinHttpWriteData(hRequest, req.getContent().data(), dwSize, NULL)) &&
	      WinHttpReceiveResponse(hRequest, NULL)
	      ) {
	    
	    DWORD headerSize = 0;

	    DWORD dwStatusCode = 0;
	    DWORD dwSize = sizeof(dwStatusCode);
	    
	    if (WinHttpQueryHeaders(hRequest, 
				    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 
				    WINHTTP_HEADER_NAME_BY_INDEX, 
				    &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
	      callback.handleResultCode(dwStatusCode);
	    }
	    
	    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &headerSize, WINHTTP_NO_HEADER_INDEX)) {
	      // Allocate buffer by header length
	      // If the function fails and ERROR_INSUFFICIENT_BUFFER is returned, lpdwBufferLength specifies the number of bytes that the application must allocate to receive the string.
	      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		unique_ptr<wchar_t[]> lpHeadBuffer(new wchar_t[dwSize / sizeof(wchar_t)]);
		if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, lpHeadBuffer.get(), &headerSize, WINHTTP_NO_HEADER_INDEX)) {

  		}	
	      }
	    }

	    while ( 1 ) {
	      // Check for available data to get data size in bytes
	      dwSize = 0;
	      if (!WinHttpQueryDataAvailable(hRequest, &dwSize)){
                break;
	      }
	      if (!dwSize) {
                break;  //data size 0          
	      }

	      // Allocate buffer by data size
	      unique_ptr<char[]> outBuffer(new char[dwSize + 1]);	     
	      // Read data from server
	      DWORD dwDownloaded = 0;
	      if (WinHttpReadData(hRequest, outBuffer.get(), dwSize, &dwDownloaded)) {
		bool r = callback.handleChunk(dwDownloaded, outBuffer.get());
		if (!r) break;
	      } else {
		break;
	      }

	      if (!dwDownloaded) break;
	    }
	    
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
  }
  
 protected:
 
 };
  

std::unique_ptr<HTTPClient>
WinHTTPClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<WinHTTPClient>(new WinHTTPClient(_user_agent, _enable_cookies, _enable_keepalive));
}
