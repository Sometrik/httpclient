#include "URI.h"

#include <cstdio>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cassert>
#include <cstring>

#include "FNV.h"
#include "utf8.h"

using namespace std;

static size_t split(const string & line, back_insert_iterator< vector<string> > backiter, char delimiter) {
  unsigned int n = 0;
  if (line.size()) {
    char * str = new char[line.size() + 1];
    strcpy(str, line.c_str());
    char * current = str;
    for (unsigned int i = 0, len = (int)line.size(); i < len; i++) {
      if ((delimiter == 0 && isspace(str[i])) || (delimiter != 0 && str[i] == delimiter)) {
	str[i] = 0;
	*backiter++ = current;
	n++;
	if (delimiter == 0) {
	  while (isspace(str[i + 1])) i++;
	}
	current = str + i + 1;
      }
    }
    *backiter++ = current;
    n++;
    delete[] str;
  }
  return n;
}

// empty string returns empty vector
// otherwise returns delimiter count + 1 strings
static vector<string> split(const string & line, char delimiter) {
  vector<string> v;
  // back_insert_iterator< vector<string> > backiter(v);
  split(line, back_inserter(v), delimiter);
  return v;
}

static uint32_t toLower(uint32_t c) {
  if (c <= 127) {
    return (char)tolower((char)c);
  } else if ((c >= 192 && c <= 214) || // Agrave to Ouml
	     (c >= 216 && c <= 222) // Oslash to Thorn
	     ) {
    return c + 32;
  } else if (c >= 0x0410 && c <= 0x042f) { // Cyrillic capitals
    return c + 0x20;
  } else {
    return c;
  }
}

static string toLower(const string & input) {
  string r;
  r.reserve(input.size());

  const char * str = input.c_str();
  const char * str_i = str;
  const char * end = str + input.size(); 

  while ( str_i < end ) {
    uint32_t c = utf8::next(str_i, end); // get 32 bit code of a utf-8 symbol
    utf8::append(toLower(c), back_inserter(r));
  }
  return r;
}

URI::URI() : port(0), is_canonical(false) { // , http_status_code(0) {

}

URI::URI(const string & value, bool _is_canonical) : port(0), is_canonical(_is_canonical) { //, http_status_code(0) {
  parse(value);
}

static bool always_convert_fragment(const std::string & domain) {
  if (domain == "facebook.com") return true;
  else if (domain == "plus.google.com") return true;
  else if (domain == "vkontakte.ru") return true;
  else return false;
}

#if 0
static bool never_convert_fragment(const std::string & domain) {
  if (domain == "photo.weibo.com" || domain == "e.weibo.com" || domain == "www.weibo.com" || domain == "e.weibo.com" || domain == "live.wsj.com" || domain == "www.vogue.fr" ||
      domain == "www.oivaus.fi" || domain == "www.vgtv.no" || domain == "tweetedtimes.com" || domain == "fi.foodie.fm" || domain == "www.bublaa.com" ||
      domain == "www.radiorock.fi" || domain == "www.radiosuomipop.fi" || domain == "www.radioaalto.fi" || domain == "springpad.com" || domain == "showyourbest.olympic.org" ||
      domain == "www.networka.com" || domain == "www.rootstrikers.org" || domain == "de.drive-now.com" || domain == "www.loop.fi" || domain == "www.metrohelsinki.fi" ||
      domain == "www.foodie.fi" || domain == "www.daytrotter.com" || domain == "www.accuradio.com" || domain == "www.minmote.no" || domain == "uplay.ubi.com" ||
      domain == "www.veikkaus.fi" || domain == "www.groovefm.fi" || domain == "www.atgplay.se" || domain == "www.statsocial.com" || domain == "tidal.com"
      ) {
    return true;
  } else {
    return false;
  }
}
#endif

static bool isValidScheme(const std::string & s) {
  for (unsigned int i = 0; i < s.size(); i++) {
    char c = tolower(s[i]);
    if (!(c >= 'a' && c <= 'z')) {
      return false;
    }
  }
  return true;
}

void
URI::parse(const string & value) {
  string s;
  if (value.find("\xc2\xad") != string::npos) {
    for (unsigned int i = 0; i < value.size(); i++) {
      if (value[i] == '\xc2' && i + 1 < value.size() && value[i + 1] == '\xad') {
	i++;
      } else {
	s += value[i];
      }
    }
  } else {
    s = value;
  }

  scheme.clear();
  domain.clear();
  path.clear();
  query.clear();
  fragment.clear();

  string::size_type pos;
  pos = s.find('#');
  if (pos != string::npos) {
    fragment = s.substr(pos + 1);
    s.erase(pos);
  }

  pos = s.find('?');
  if (pos != string::npos) {
    query = s.substr(pos + 1);
    s.erase(pos);
  }  

  if (s.compare(0, 2, "//") == 0) { // empty scheme
    s = s.substr(2);
  }
  
  while ( 1 ) {
    pos = s.find("://");
    if (pos == string::npos) {
      break;
    } else {
      if (!scheme.empty()) {
	string::size_type pos2 = s.find_first_of("/");
	assert(pos2 != string::npos);
	if (pos2 < pos) { // other scheme is in path
	  break;
	}
      }
      string tmp = s.substr(0, pos);
      if (isValidScheme(tmp)) {
	scheme = tmp;
      }
      s.erase(0, pos + 3);      
    }
  }
  if (scheme == "http" || scheme == "https") {
    if (s[0] == '/' && s[1] == '/') {
      s.erase(0, 2);
    } else if (s[0] == '/') {
      s.erase(0, 1);
    }
  }
  pos = s.find('/');
  if (pos != string::npos) {
    path = s.substr(pos);
    s.erase(pos);
  }
  pos = s.find(':');
  if (pos != string::npos) {
    string tmp = s.substr(pos + 1);
    port = atoi(tmp.c_str());
    s.erase(pos);
  } else {
    port = 0;
  }
  if (!path.empty() || port || !scheme.empty()) {
    domain = s;
  } else if ((s.size() >= 5 && s.compare(s.size() - 5, 5, ".html") == 0) ||
	     (s.size() >= 4 && s.compare(s.size() - 4, 4, ".htm") == 0)) {
    path = s;
  }
  
  // fprintf(stderr, "URI: %s => (scheme = %s, domain = %s, path = %s, query = %s, fragment = %s)\n", value.c_str(), scheme.c_str(), domain.c_str(), path.c_str(), query.c_str(), fragment.c_str());

#if 0
  if (fragment.size() > 2) {
    string::size_type pos = fragment.find_first_of("/");
    bool has_slash = pos != string::npos;
    bool convert = false;
    bool always_convert = always_convert_fragment(domain);
    if (fragment[0] == '!' && !never_convert_fragment(domain) && (always_convert || fragment[1] == '/')) {
      fragment = fragment.substr(1);
      assert(fragment[0] == '/');
      convert = true;
    } else if (always_convert && has_slash) {
      if (fragment[0] != '/') {
	fragment = "/" + fragment;
      }
      convert = true;
    }
    // https://www.facebook.com/ValioKunstnikud#!/photo.php?fbid=10150665052909222&set=a.276842984221.141823.276599794221&type=1&theater
    // http://m.youtube.com/index?desktop_uri=%2F&gl=US#/watch?v=60jILAVNVvU
    // https://plus.google.com/u/0/?tab=wX#115159547364564609868/posts
    if (convert) {
      if (fragment[0] == '/') {
	path = fragment;
      } else {
	pos = path.find_last_of("/");
	assert(pos != string::npos);
	path.erase(pos + 1);
	path += fragment;
      }
      fragment.clear();
    
      pos = path.find('?');
      if (pos != string::npos) {
	query = path.substr(pos + 1);
	path.erase(pos);
      } else {
	query.clear();
      }       
    
      fprintf(stderr, "converted fragment: %s => (scheme = %s, domain = %s, path = %s, query = %s, fragment = %s)\n", value.c_str(), scheme.c_str(), domain.c_str(), path.c_str(), query.c_str(), fragment.c_str());
    }
  }
#endif
}
  
string
URI::toString() const {
  string s;
  if (!scheme.empty()) {
    s += scheme + "://";
  }
  s += domain;
  if (port && port != 80 && port != 443) {
    s += ":" + to_string(port);
  }
  s += path;
  if (!query.empty()) {
    s += "?" + query;
  }
  if (!fragment.empty()) {
    s += "#" + fragment;
  }
  return s;
}

void
URI::parseQueryString(const string & query, map<string, string> & data, list<string> & singletons) {
  vector<string> pairs = split(query, '&');
  for (vector<string>::iterator it = pairs.begin(); it != pairs.end(); it++) {
    string::size_type sep = it->find_first_of("=");
    if (sep != string::npos) {
      string key = it->substr(0, sep);
      string value = urldecode(it->substr(sep + 1));
      // fprintf(stderr, "key = %s, value = %s\n", key.c_str(), value.c_str());
      data[key] = value;
    } else {
      singletons.push_back(*it);
    }
  }
}

map<string, string>
URI::parseQueryString(const string & query, const char separator) {
  map<string, string> data;
  vector<string> pairs = split(query, separator);
  for (vector<string>::iterator it = pairs.begin(); it != pairs.end(); it++) {
    string::size_type sep = it->find_first_of("=");
    if (sep != string::npos) {
      string key = urldecode(it->substr(0, sep));
      string value = urldecode(it->substr(sep + 1));
      // fprintf(stderr, "key = %s, value = %s\n", key.c_str(), value.c_str());
      data[key] = value;
    }
  }
  return data;
}

vector<string>
URI::splitQueryString(const char separator) const {
  return split(query, separator);
}

void
URI::setQueryString(const map<string, string> & pairs, const list<string> & singletons) {
  query = "";
  for (list<string>::const_iterator it = singletons.begin(); it != singletons.end(); it++) {
    if (query != "") query += '&';
    query += urlencode(*it);
  }
  for (map<string, string>::const_iterator it = pairs.begin(); it != pairs.end(); it++) {
    if (query != "") query += '&';
    query += it->first;
    query += '=';
    query += urlencode(it->second);
  }
}

string
URI::formatQueryString(const map<string, string> & data, bool encode_punctuation) {
  string r;
  for (const auto & p : data) {
    if (!r.empty()) r += '&';
    r += urlencode(p.first, false, encode_punctuation);
    r += '=';
    r += urlencode(p.second, false, encode_punctuation);
  }
  return r;
}

void
URI::setQueryString(const map<string, string> & pairs) {
  query = formatQueryString(pairs);
}

string
URI::getReducedDomain() const {
  string d = toLower(domain);
  if (d.compare(0, 4, "www.") == 0) {
    return d.substr(4);
  } else if (d.size() >= 5 && d.compare(0, 3, "www") && isdigit(d[3]) && d[4] == '.') {
    return d.substr(5);
  } else {
    return d;
  }
}

long long
URI::getDomainHash() const {
  return FNV::calcFNV1a_64(getReducedDomain());  
}

long long
URI::getHash() const {
  // no protocol in hash
  string reduced_domain = getReducedDomain();
  string s;
  s += reduced_domain;
  if (!is_canonical && !fragment.empty() && fragment[0] == '!') {
    // string tmp = fragment;
    // assert(fragment.find_first_of('?') == string::npos);
    if (fragment[1] != '/') s += '/';
    s += urldecode(fragment.substr(1));
  } else if (!is_canonical && !fragment.empty() && always_convert_fragment(reduced_domain) && fragment.find_first_of("/") != string::npos) {
    // assert(fragment.find_first_of('?') == string::npos);
    if (fragment[0] != '/') s += '/';
    s += urldecode(fragment);
  } else {
    if (!path.empty()) {
      assert(path[0] == '/');
      s += urldecode(path);
    }
    if (!query.empty()) {
      s += "?" + urldecode(query);
    }
    if (is_canonical && !fragment.empty()) {
      s += '#' + urldecode(fragment);
    }
  }
  long long hash = FNV::calcFNV1a_64(s);
  return hash;
}

void
URI::urldecodeAll() {
  query = urldecode(query);
  path = urldecode(path);
  fragment = urldecode(fragment);
}

URI
URI::replaceFragment() const {
  URI new_uri = *this;
  new_uri.replaceFragmentInPlace();
  return new_uri;
}

void
URI::replaceFragmentInPlace() {
  bool mod = false;
  if (!fragment.empty() && fragment[0] == '!') {
    path.clear();
    if (fragment[1] != '/') path += '/';
    path += fragment.substr(1);
    fragment.clear();
    mod = true;
  } else if (!fragment.empty() && always_convert_fragment(getReducedDomain()) && fragment.find_first_of("/") != string::npos) {
    path.clear();
    if (fragment[0] != '/') path += '/';
    path += fragment;
    fragment.clear();
    mod = true;
  }
  if (mod) {
    string::size_type pos = path.find('?');
    if (pos != string::npos) {
      query = path.substr(pos + 1);
      path.erase(pos);
    } else {
      query.clear();
    }  
  }
}

static char to_hex_digit(int n) {
  if (n >= 0 && n <= 9) {
    return (char)('0' + n);
  } else if (n >= 10 && n <= 15) {
    return (char)('A' + n - 10);
  } else {
    return 0;
  }
}

string
URI::urlencodeUtf8(const string & str) {
  string output;
  for (unsigned int i = 0; i < str.size(); i++) {
    unsigned char c = str[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
	c == '-' || c == '_' || c == '.' || c == '~') { // || c == '(' || c == ')' || c == ':' || c == ',') {
      output += (char)c;
    } else {
      output += '%';
      output += to_hex_digit(c >> 4);
      output += to_hex_digit(c & 0x0f);
    }
  }
  return output;
}

string
URI::urlencode(const string & str, bool spaces_as_plus, bool encode_punctuation) {
  string output;
  for (unsigned int i = 0; i < str.size(); i++) {
    unsigned char c = str[i]; // ?
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
	c == '-' || c == '_' || c == '.' || c == '~' ||
	(!encode_punctuation && (c == '(' || c == ')' || c == ':' || c == ','))) {
      output += (char)c;
    } else if (spaces_as_plus && c == ' ') {
      output += '+';
    } else {
      output += '%';
      output += to_hex_digit(c >> 4);
      output += to_hex_digit(c & 0x0f);
    }
    // no plus allowed here, hexletters must be upcase
  }
  return output;
}

static int get_xdigit(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return 0;
  }
}

string
URI::urldecode(const string & str) {
  string output;
  for (unsigned int i = 0; i < str.size(); i++) {
    char c = str[i];
    if (c == '+') {
      output += ' ';
    } else if (c == '%' && i + 2 < str.size()) {
      char c1 = str[i + 1];
      char c2 = str[i + 2];
      output += (char)(get_xdigit(c1) * 16 + get_xdigit(c2));
      i += 2;
    } else {
      output += c;
    }
  }
  return output;
}
