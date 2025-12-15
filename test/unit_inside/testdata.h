#ifndef testdata_H
#define testdata_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "cdr/travoddscdr.h"

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-compare"

#if defined(DDS_TYPE_EXPORT) && defined(_WIN32)
#define DDS_TYPE_API __declspec(dllexport)
#else
#define DDS_TYPE_API 
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace TestModule {

class DDS_TYPE_API BasicData {
public:
    bool operator<(const BasicData& rhs) const {
        return false;
    }
    int id;
    std::string name;
};

class DDS_TYPE_API KeyData {
public:
    bool operator<(const KeyData& rhs) const {
        return false;
    }
    int userId;// @key 
    std::string data;
};

class DDS_TYPE_API SequenceData {
public:
    bool operator<(const SequenceData& rhs) const {
        return false;
    }
    std::vector<int> numbers;
};

} // namespace TestModule

#ifdef __cplusplus
}
#endif
#endif /* testdata_H */
