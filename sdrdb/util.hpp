#ifndef SDRDB_UTIL_H
#define SDRDB_UTIL_H

std::chrono::system_clock::duration since_epoch()
{
    return std::chrono::system_clock::now().time_since_epoch();
}

// trim from start
std::string & ltrim(std::string & s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

bool is_number(const std::string & s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

// trim from end
std::string & rtrim(std::string & s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string & trim(std::string & s)
{
    return ltrim(rtrim(s));
}

std::string tolower(const std::string & s)
{
    std::string ret { s };

    std::transform(ret.begin(), ret.end(), ret.begin(), [](const int c) {
        if(c >= 'A' && c <= 'Z') {
            return c - 'A' + 'a';
        }

        return c;
    });

    return ret;
}

#endif