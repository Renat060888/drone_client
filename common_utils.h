#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <cassert>
#include <bitset>
#include <signal.h>
#include <sys/file.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <thread>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

#include <boost/regex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "from_ms_common/common/ms_common_utils.h"


namespace common_utils {

// used to split the file in lines
const boost::regex linesregx("\\r\\n|\\n\\r|\\n|\\r");

// used to split each line to tokens, assuming ',' as column separator
const boost::regex fieldsregx(",(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");

typedef std::vector<std::string> TRow;

static std::vector<TRow> parseCSVFile(const char* data, unsigned int length)
{
    std::vector<TRow> result;

    // iterator splits data to lines
    boost::cregex_token_iterator li(data, data + length, linesregx, -1);
    boost::cregex_token_iterator end;

    while (li != end) {
        std::string line = li->str();
        ++li;

        // Split line to tokens
        boost::sregex_token_iterator ti(line.begin(), line.end(), fieldsregx, -1);
        boost::sregex_token_iterator end2;

        std::vector<std::string> row;
        while (ti != end2) {
            std::string token = ti->str();
            ++ti;
            row.push_back(token);
        }
        if (line.back() == ',') {
            // last character was a separator
            row.push_back("");
        }
        result.push_back(row);
    }
    return result;
}

}

#endif // COMMON_UTILS_H

