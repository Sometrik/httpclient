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
#include <mutex>

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

struct winhttp_status_context_s {
public:
  void log(const std::string& s) {
    std::lock_guard<std::mutex> guard(mutex_);
    log_ += s;
  }
  std::string getLog() const {
    std::string tmp;
    {
      std::lock_guard<std::mutex> guard(mutex_);
      tmp = log_;
    }
    return tmp;
  }

private:
  std::string log_;
  mutable std::mutex mutex_;
};

static void __stdcall winhttp_status_callback(HINTERNET handle,
					      DWORD_PTR context0,
					      DWORD status,
					      void* info,
					      DWORD info_len) {
  std::string status_string, info_string;
 
  switch (status) {
  case WINHTTP_CALLBACK_STATUS_HANDLE_CREATED:
    status_string = "handle created";
    break;
  case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
    status_string = "handle closing";
    break;
  case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
    status_string = "resolving";
    info_string = to_utf8(wstring(static_cast<wchar_t*>(info), info_len - 1));
    break;
  case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
    status_string = "resolved";
    break;
  case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
    status_string = "connecting";
    info_string = to_utf8(wstring(static_cast<wchar_t*>(info), info_len - 1));
    break;
  case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
    status_string = "connected";
    break;
  case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
    status_string = "sending";
    break;
  case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
    status_string = "sent";
    break;
  case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
    status_string = "receiving response";
    break;
  case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
    status_string = "response received";
    break;
  case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
    status_string = "connection closing";
    break;
  case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:
    status_string = "connection closed";
    break;
  case WINHTTP_CALLBACK_STATUS_REDIRECT:
    status_string = "redirect";
    info_string = to_utf8(wstring(static_cast<wchar_t*>(info), info_len - 1));
    break;
  case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
    status_string = "data available";
    info_string = to_string(*static_cast<uint32_t*>(info));
    break;
  case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
    status_string = "headers available";
    break;
  case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
    status_string = "read complete";
    info_string = to_string(info_len);
    break;
  case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
    status_string = "send request complete";
    break;
  case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
    status_string = "write complete";
    break;
  case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
    status_string = "request error";
    break;
  case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
    status_string = "https failure";
    info_string = to_string(*static_cast<uint32_t*>(info));
    break;
  default:
    break;
  }

  std::string msg;
  if (!status_string.empty()) msg += "status=" + status_string;
  else msg += "status=" + to_string(status);
  if (!info_string.empty()) msg += ", info(" + to_string(info_len) + ") = " + info_string;

  for (auto& c : msg) if (c == 0) c = '?';

  if (context0) {
    auto* context = reinterpret_cast<winhttp_status_context_s*>(context0);
    context->log(msg + "\r\n");
  }

  string tmp = "status: " + msg + "\r\n";
  OutputDebugStringA(tmp.c_str());
}

class WinHTTPClient : public HTTPClient {
 public:
   WinHTTPClient(const std::string& user_agent, bool enable_cookies = true, bool enable_keepalive = true)
     : HTTPClient(user_agent, enable_cookies, enable_keepalive)
   {
     auto ua_text = from_utf8(getUserAgent());
     // Use WinHttpOpen to obtain a session handle.
     session_ = WinHttpOpen(ua_text.c_str(),
       WINHTTP_ACCESS_TYPE_NO_PROXY,
       WINHTTP_NO_PROXY_NAME,
       WINHTTP_NO_PROXY_BYPASS, 0);

     DWORD decompression = WINHTTP_DECOMPRESSION_FLAG_ALL;
     WinHttpSetOption(session_, WINHTTP_OPTION_DECOMPRESSION, &decompression, sizeof(decompression));

#if 1
     DWORD protocols = WINHTTP_PROTOCOL_FLAG_HTTP2;
     WinHttpSetOption(session_, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &protocols, sizeof(protocols));
     DWORD secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_ALL | 0xa00;
     if (!WinHttpSetOption(session_, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols))) {
       string tmp = "failed to set secure protocols: " + to_string(GetLastError()) + "\r\n";
       OutputDebugStringA(tmp.c_str());
     }
#endif
   }

  ~WinHTTPClient() {
    if (session_) {
      WinHttpCloseHandle(session_);
    }
  }

  void request(const HTTPRequest & req, const Authorization & auth, HTTPClientInterface & callback) override {
    if (!session_) {
      callback.handleErrorText("Not initialized");
      return;
    }

    auto context = make_unique<winhttp_status_context_s>();

    URI uri(req.getURI());
    
    auto target_scheme = uri.getScheme();
    auto target_host = from_utf8(uri.getDomain());
    auto target_port = uri.getPort();

    auto target_path0 = uri.getPath();
    if (!uri.getQueryString().empty()) target_path0 += "?" + uri.getQueryString();

    auto target_path = from_utf8(target_path0);

    bool is_secure = false;
    if (target_scheme == "http") {
      if (!target_port) target_port = INTERNET_DEFAULT_HTTP_PORT;
    } else if (target_scheme == "https") {
      if (!target_port) target_port = INTERNET_DEFAULT_HTTPS_PORT;
      is_secure = true;
    } else {
      return;
    }
        
    auto hConnect = WinHttpConnect(session_, target_host.c_str(), target_port, 0);
    if (!hConnect) {
      callback.handleErrorText("Could not create connection handle: " + to_string(GetLastError()));
    } else {      
      // WINHTTP_OPTION_CONNECT_TIMEOUT (ms)
      // WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT (ms)
      // WINHTTP_OPTION_RECEIVE_TIMEOUT (ms)
      // WINHTTP_OPTION_RESOLVE_TIMEOUT (ms)
      // WINHTTP_OPTION_SEND_TIMEOUT (ms)
      
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
      if (!hRequest) {
	callback.handleErrorText("Could not create request: " + to_string(GetLastError()));
      } else {
	if (WinHttpSetStatusCallback(hRequest, &winhttp_status_callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0) == WINHTTP_INVALID_STATUS_CALLBACK) {
	  string tmp = "Could not set callback: " + to_string(GetLastError()) + "\n";
	  OutputDebugStringA(tmp.c_str());
	}

	auto context_ptr = context.get();
	if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_CONTEXT_VALUE, &context_ptr, sizeof(context_ptr))) {
	  OutputDebugStringA("Failed to set context\n");
	}
	
	DWORD disabled_features = 0;
	if (!req.getFollowLocation()) disabled_features |= WINHTTP_DISABLE_REDIRECTS;
	if (!enable_keepalive) disabled_features |= WINHTTP_DISABLE_KEEP_ALIVE;
	if (!enable_cookies) disabled_features |= WINHTTP_DISABLE_COOKIES;
	if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &disabled_features, sizeof(disabled_features))) {
	  OutputDebugStringA("Could not set options");
	}

	map<string, string> combined_headers;
	for (auto & hd : default_headers) {
	  combined_headers[hd.first] = hd.second;      
	}
	for (auto & hd : req.getHeaders()) {
	  combined_headers[hd.first] = hd.second;
	}
	  
	if (req.getType() == HTTPRequest::POST || req.getType() == HTTPRequest::OPTIONS) {
	  if (!req.getContentType().empty()) {
	    combined_headers["Content-Type"] = req.getContentType();
	  }
	}

	const BasicAuth * basic = dynamic_cast<const BasicAuth *>(&auth);
	if (basic) {
	  // FIXME: implement
	} else {
	  string auth_header = auth.createHeader();
	  if (!auth_header.empty()) {
	    combined_headers[auth.getHeaderName()] = auth_header;
	  }
	}

	for (auto & [ name, value ] : combined_headers) {
	  auto header0 = name + ": " + value;
	  auto header = from_utf8(header0 + "\r\n");
	  context->log("adding header: " + header0 + "\r\n");

	  WinHttpAddRequestHeaders(hRequest, header.c_str(), -1, WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD);
	}
	  
	auto & postData = req.getContent();
	
	context->log("sending request\r\n");

	bool req_r = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (void*)postData.data(), postData.size(), postData.size(), 0);

	if (is_secure && !req_r && GetLastError() == 12175) {
	  context->log("retrying request (https error)\r\n");
	  DWORD security_flags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE | SECURITY_FLAG_IGNORE_ALL_CERT_ERRORS;
	  WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &security_flags, sizeof(security_flags));
	  req_r = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (void*)postData.data(), postData.size(), postData.size(), 0);
	}

	if (!req_r) {
	  callback.handleErrorText("Could not send request: " + to_string(GetLastError()));
	  context->log("failed to send request (" + to_string(GetLastError()) + ")\r\n");
	} else if (WinHttpReceiveResponse(hRequest, NULL)) {
	  context->log("received response\r\n");

	  bool terminate = false;

	  DWORD headerSize = 0;
	  WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &headerSize, WINHTTP_NO_HEADER_INDEX);

	  // Allocate buffer by header length
	  // If the function fails and ERROR_INSUFFICIENT_BUFFER is returned, lpdwBufferLength specifies the number of bytes that the application must allocate to receive the string.
	  if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
	    wstring headerBuffer(headerSize / sizeof(wchar_t), 0);
	    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, headerBuffer.data(), &headerSize, WINHTTP_NO_HEADER_INDEX)) {
	      string headers = to_utf8(headerBuffer);
	      OutputDebugStringA(headers.c_str());
	      if (!callback.handleHeaderChunk(headers.size(), headers.data())) {
		terminate = true;
	      }
	    } else {
	      callback.handleErrorText("Failed to get headers: " + to_string(GetLastError()));
	      terminate = true;
	    }
	  } else {
	    callback.handleErrorText("Failed to get headers: no headers");
	    terminate = true;
	  }
	  
	  while (!terminate) {
	    // Check for available data to get data size in bytes
	    DWORD dwSize = 0;
	    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
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
	    }
	    else {
	      break;
	    }

	    if (!dwDownloaded) break;
	    if (!callback.onIdle()) break;
	  }

	  callback.handleDisconnect();
	} else {
	  callback.handleErrorText("Failed to receive response: " + to_string(GetLastError()));
	}
	  
	WinHttpCloseHandle(hRequest);
      }
      WinHttpCloseHandle(hConnect);
    }

    OutputDebugStringA("termianting\r\n");
    context->log("terminating\r\n");
    callback.handleLogText(context->getLog());
  }
  
  void clearCookies() override {
  }
  
protected:
  HINTERNET session_;
};
  

std::unique_ptr<HTTPClient>
WinHTTPClientFactory::createClient2(const std::string & _user_agent, bool _enable_cookies, bool _enable_keepalive) {
  return std::unique_ptr<WinHTTPClient>(new WinHTTPClient(_user_agent, _enable_cookies, _enable_keepalive));
}
