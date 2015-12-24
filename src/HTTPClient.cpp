#include "HTTPClient.h"

#include "URI.h"
#include <iostream>

using namespace std;

#if 0
string
HTTPClient::encodeIDN(const string & input) {
  URI uri(input);
  
  bool ascii = true;
  for (unsigned int i = 0; i < uri.getDomain().size(); i++) {
    if (uri.getDomain()[i] < 0) {
      ascii = false;
      break;
    }
  }
  if (ascii) {
    return input;
  } else {
    vector<string> parts = StringUtils::split(uri.getDomain(), '.');
    string output;
    for (vector<string>::iterator it = parts.begin(); it != parts.end(); it++) {
      if (it != parts.begin()) output += '.';
      output += StringUtils::encodePunycode(*it);
    }
    cerr << "domain converted to " << output << endl;
    uri.setDomain(output);
    return uri.toString();
  }
}
#endif

