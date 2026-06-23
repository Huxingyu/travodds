#ifndef type_H
#define type_H

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

namespace PerformanceTest {

class DDS_TYPE_API TestData {
public:
    bool operator<(const TestData& rhs) const {
        return false;
    }
    unsigned long long sn;
    unsigned long long size;
    std::string data;
};

} // namespace PerformanceTest

#ifdef __cplusplus
}
#endif
#endif /* type_H */
