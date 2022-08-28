#ifndef _HTTPCLIENTINTERFACE_H_
#define _HTTPCLIENTINTERFACE_H_

#include <string>
#include <string_view>
#include <cctype>
#include <charconv>

class HTTPClientInterface {
 public:
  virtual ~HTTPClientInterface() = default;
  virtual void handleResultCode(int code) { }
  virtual void handleErrorText(std::string_view s) { }
  virtual void handleLogText(std::string_view s) { }
  virtual bool handleRedirectUrl(std::string_view url) { return true; }
  virtual void handleHeader(std::string_view key, std::string_view value) { }
  virtual bool handleChunk(std::string_view chunk) = 0;
  virtual void handleDisconnect() { }
  virtual bool onIdle() { return true; }
  
  virtual bool handleHeaderChunk(std::string_view input0) {
    std::string_view::size_type pos0 = 0;
    while ( pos0 < input0.size() ) {
      auto pos = input0.find("\r\n", pos0);
      if (pos == std::string_view::npos) pos = input0.size();

      auto input = input0.substr(pos0, pos - pos0);
      pos0 = pos + 2;
	  
      if (input.compare(0, 5, "HTTP/") == 0) {
	auto pos1 = input.find_first_of(' ');
	if (pos1 != std::string_view::npos) {
	  pos1++;
	  auto pos2 = input.find_first_of(' ', pos1);
	  if (pos2 != std::string_view::npos) {
	    auto s2 = input.substr(pos1, pos2 - pos1);
	    int code = 0;
	    auto result = std::from_chars(s2.data(), s2.data() + s2.size(), code);
	    if (result.ec != std::errc::invalid_argument) {
	      handleResultCode(code);
	    }
	  }
	}
      } else {
	int pos1 = 0;
	for ( ; pos1 < static_cast<int>(input.size()) && input[pos1] != ':'; pos1++) { }
	int pos2 = input[pos1] == ':' ? pos1 + 1 : pos1;
	for (; pos2 < static_cast<int>(input.size()) && isspace(input[pos2]); pos2++) { }
	int pos3 = input.size();
	for (; pos3 > 0 && isspace(input[pos3 - 1]); pos3--) { }
	auto key = input.substr(0, pos1);
	auto value = input.substr(pos2, pos3 - pos2);
	
	if (tolower_ascii(key) == "location" && !handleRedirectUrl(value)) {
	  return false;
	}
	handleHeader(key, value);
      }
    }
    return true;
  }

private:
  static inline std::string tolower_ascii(std::string_view input) {
    std::string r;
    for (auto & c : input) r += c >= 0 && c < 128 ? tolower(c) : c;
    return r;
  }
};

#endif
