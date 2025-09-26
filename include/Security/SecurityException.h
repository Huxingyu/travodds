#ifndef SECURITYEXCEPTION_H__
#define SECURITYEXCEPTION_H__

#include <exception>
#include <string>
#include "travodds_dll.h"

TRAVODDS_NAMESPACE_BEGIN

class SecurityException : public std::exception
{
public:
    SecurityException() 
        : message_("An unspecified security exception occurred."),
          minor_(1) 
    {}

    explicit SecurityException(
        const std::string& message,
        const int32_t minor = 1)
        : message_(message), 
        minor_(minor){};
    
    const char* what() const noexcept{
        return message_.c_str();
    };
private:

    std::string message_;

    int32_t minor_;
};

TRAVODDS_NAMESPACE_END

#endif // ifndef SECURITYEXCEPTION_H__