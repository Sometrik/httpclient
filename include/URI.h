#ifndef _URI_H_
#define _URI_H_

#include <string>
#include <map>
#include <list>
#include <vector>

class URI {
 public:
  URI();
  URI(const std::string & value, bool _is_canonical = false);

  void parse(const std::string & v);

  void setScheme(const std::string & _scheme) { scheme = _scheme; }
  void setDomain(const std::string & _domain) { domain = _domain; }
  void setPort(unsigned short _port = 0) { port = _port; }
  void setPath(const std::string & _path) { path = _path; }
  void setQueryString(const std::string & _query) { query = _query; }
  void setFragment(const std::string & _fragment) { fragment = _fragment; }
  
  const std::string & getScheme() const { return scheme; }
  const std::string & getDomain() const { return domain; }
  const unsigned short getPort() const { return port; } 
  const std::string & getPath() const { return path; }
  const std::string & getQueryString() const { return query; }
  const std::string & getFragment() const { return fragment; }

  bool isValid() const { return true; }
  bool isDefined() const { return domain.size() || query.size(); }
  
  static void parseQueryString(const std::string & query, std::map<std::string, std::string> & pairs, std::list<std::string> & singletons);
  static std::map<std::string, std::string> parseQueryString(const std::string & query, const char separator = '&');
  std::vector<std::string> splitQueryString(const char separator = '&') const;
  
  void parseQueryString(std::map<std::string, std::string> & pairs, std::list<std::string> & singletons) const { parseQueryString(query, pairs, singletons); }
  std::map<std::string, std::string> parseQueryString(const char separator = '&') const { return parseQueryString(query, separator); }

  std::string toString() const;
  std::string getDisplayUrl() const { return domain + path; }
  
  void setQueryString(const std::map<std::string, std::string> & pairs, const std::list<std::string> & singletons);
  void setQueryString(const std::map<std::string, std::string> & pairs);

  static std::string formatQueryString(const std::map<std::string, std::string> & data, bool encode_punctuation = true);

  std::string getReducedDomain() const;
  long long getDomainHash() const;
  long long getHash() const;
  
  URI replaceFragment() const;
  void replaceFragmentInPlace();

  void replaceQueryWithFragment() {
    query = fragment;
    fragment.clear();
  }

  bool isCanonical() const { return is_canonical; }
  void setIsCanonical(bool t) { is_canonical = t; }

  void urldecodeAll();
  static std::string urlencode(const std::string & str, bool spaces_as_plus = false, bool encode_punctuation = true);
  static std::string urlencodeUtf8(const std::string & str);
  static std::string urldecode(const std::string & str);

 private:
  std::string scheme;
  std::string domain;
  unsigned short port; // not implemented
  std::string path;
  std::string query;
  std::string fragment;
  bool is_canonical;
};

#endif
