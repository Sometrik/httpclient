#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include <HTTPClientInterface.h>

#include <unordered_map>

class HTTPResponse : public HTTPClientInterface {
 public:
  bool isInfo() const { return result_code >= 100 && result_code <= 199; }
  bool isSuccess() const { return result_code >= 200 && result_code <= 299; }
  bool isError() const { return result_code == 0 || (result_code >= 400 && result_code <= 599); }
  bool isRedirect() const { return result_code >= 300 && result_code <= 399; }

  const std::string & getContent() { return content; }
  const std::string & getErrorText() { return error_text; }
  const std::string & getLogText() { return log_text; }
  int getResultCode() const { return result_code; }
  const std::string & getRedirectUrl() const { return redirect_url; }

  const std::unordered_map<std::string, std::string> & getHeaders() const { return headers; }

  void handleResultCode(int code) override { result_code = code; }
  void handleErrorText(std::string s) override { error_text = s; }
  void handleLogText(std::string s) override { log_text = s; }
  bool handleRedirectUrl(const std::string & url) override {
    redirect_url = url;
    return true;
  }
  void handleHeader(const std::string & key, const std::string & value) override {
    headers[key] = value;
  }
  bool handleChunk(size_t len, const char * chunk) override {
    content += std::string(chunk, len);
    return true;
  }
  
 private:
  int result_code = 0;
  std::string error_text;
  std::string redirect_url;
  std::string content;
  std::string log_text;
  std::unordered_map<std::string, std::string> headers;
};

#endif
