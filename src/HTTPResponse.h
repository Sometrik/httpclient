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
  
  bool isSuccess() const {
    return
      result_code == 200 ||
      result_code == 301 ||
      result_code == 302 ||
      result_code == 303 ||
      result_code == 307;
  }
  bool isError() const { return !isSuccess(); }

  const std::string & getContent() { return content; }
  const std::string & getErrorText() { return error_text; }
  int getResultCode() const { return result_code; }
  const std::string & getRedirectUrl() const { return redirect_url; }

#if 0
  const std::map<std::string, std::string> & getHeaders() const { return headers; }
  void addHeader(const char * name, const char * value) {
    headers[name] = value;
  }
#endif

 private:
  int result_code;
  std::string error_text;
  std::string redirect_url;
  std::string content;
#if 0
  std::map<std::string, std::string> headers;
#endif
};

#endif
