#include "validation.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

using namespace std;

// unsigned char: passar um char negativo a isdigit/isalnum é comportamento indefinido
static bool isDigit(unsigned char c)    { return isdigit(c); }
static bool isAlnum(unsigned char c)    { return isalnum(c); }
static bool isNameChar(unsigned char c) { return isalnum(c) || c == '-' || c == '_'; }

static bool allOf(const string& s, bool (*pred)(unsigned char)){
    return all_of(s.begin(), s.end(), pred);
}


bool validate_uid(const string& uid){
    return uid.length() == 6 && allOf(uid, isDigit);
}

bool validate_password(const string& pw){
    return pw.length() == 8 && allOf(pw, isAlnum);
}

bool validate_port(const string& port){
    // no máximo 5 dígitos para não haver overflow no atoi
    if (port.empty() || port.length() > 5 || !allOf(port, isDigit)) return false;
    int p = atoi(port.c_str());
    return p >= 1 && p <= 65535;
}

bool validate_filename(const string& filename){
    if (filename.length() > 24) return false;

    size_t dot = filename.find('.');
    if (dot == string::npos || dot == 0) return false;

    string base = filename.substr(0, dot);
    string ext  = filename.substr(dot + 1);
    return allOf(base, isNameChar) && ext.length() == 3 && allOf(ext, isAlnum);
}

bool validate_label(const string& label){
    return !label.empty() && label.length() <= 20 && allOf(label, isNameChar);
}

bool validate_fsize(long long fsize){
    return fsize >= 0 && fsize <= 10000000;
}
