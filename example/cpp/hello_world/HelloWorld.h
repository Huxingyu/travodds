#ifndef HelloWorld_H
#define HelloWorld_H

#include <string>
#include <vector>
#include <map>
#include "cdr/agicdr.h"

#ifdef __cplusplus
extern "C" {
#endif

class HelloWorld {
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
