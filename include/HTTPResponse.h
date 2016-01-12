#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include <string>
#include <map>

class HTTPResponse {
 public:
 HTTPResponse() : result_code(0) { }
 HTTPResponse(int _result_code, const std::string & _error_text)
   : result_code(_result_code), error_text(_error_text) { }
 HTTPResponse(int _result_code, const std::string & _error_text, const std::string & _redirect_url, const std::string & _content)
   : result_code(_result_code), error_text(_error_text), redirect_url(_redirect_url), content(_content) { }

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
  
 private:
  int result_code;
  std::string error_text;
  std::string redirect_url;
  std::string content;
  std::map<std::string, std::string> headers;
};

#endif
