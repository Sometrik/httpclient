#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include <HTTPClientInterface.h>

#include <map>

class HTTPResponse : public HTTPClientInterface {
 public:
  bool isInfo() const { return result_code >= 100 && result_code <= 199; }
  bool isSuccess() const { return result_code >= 200 && result_code <= 299; }
  bool isError() const { return result_code == 0 || (result_code >= 400 && result_code <= 599); }
  bool isRedirect() const { return result_code >= 300 && result_code <= 399; }

  const std::string & getContent() { return content; }
  const std::string & getErrorText() { return error_text; }
  int getResultCode() const { return result_code; }
  const std::string & getRedirectUrl() const { return redirect_url; }

  void setResultCode(int _code) { result_code = _code; }
  void setErrorText(const std::string & _text) { error_text = _text; }
  void setRedirectUrl(const std::string & url) { redirect_url = url; }
  void setContent(const std::string & _content) { content = _content; }

  void addHeader(const std::string & key, const std::string & value) {
    headers[key] = value;
  }

  void appendContent(const std::string & s) {
    content += s;
  }

  const std::map<std::string, std::string> & getHeaders() const { return headers; }

  void handleResultCode(int code) override { result_code = code; }
  bool handleRedirectUrl(const std::string & url) override {
    redirect_url = url;
    return true;
  }
  void handleHeader(const std::string & key, const std::string & value) override {
    addHeader(key, value);
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
  std::map<std::string, std::string> headers;
};

#endif
