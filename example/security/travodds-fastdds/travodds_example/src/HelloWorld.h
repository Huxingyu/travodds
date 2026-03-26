#ifndef HelloWorld_H
#define HelloWorld_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "cdr/travoddscdr.h"

#if defined(DDS_TYPE_EXPORT) && defined(_WIN32)
#define DDS_TYPE_API __declspec(dllexport)
#else
#define DDS_TYPE_API 
#endif

#ifdef __cplusplus
extern "C" {
#endif

class DDS_TYPE_API HelloWorld {
public:
    bool operator<(const HelloWorld& rhs) const {
        return false;
    }
    unsigned int index;
    std::string message;
};

#ifdef __cplusplus
}
#endif
#endif /* HelloWorld_H */
