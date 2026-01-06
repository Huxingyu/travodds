#ifndef SECURITYEXCEPTION_H__
#define SECURITYEXCEPTION_H__

#include <exception>
#include <string>
#include <cstring>
#include "travodds_dll.h"
#include "security/securitycommon.h"

TRAVODDS_NAMESPACE_BEGIN

class SMSecurityException : public std::exception
{
public:
    SMSecurityException() 
        : message_("An unspecified security exception occurred."),
          minor_(1) 
    {}

    explicit SMSecurityException(
        const std::string& message,
        const int32_t minor = 1)
        : message_(message), 
        minor_(minor){};
    
    const char* what() const noexcept{
        return message_.c_str();
    };
    
    // 转换为基类的 struct SecurityException
    void to_base_exception(struct SecurityException* base_exception) const
    {
        if (base_exception == nullptr)
            return;
        
        base_exception->code = 1; // 默认错误代码
        base_exception->minor_code = static_cast<char>(minor_);
        
        // 复制消息，确保不超过255字符（留一个给null终止符）
        size_t msg_len = message_.length();
        if (msg_len > 255)
            msg_len = 255;
        std::memcpy(base_exception->message, message_.c_str(), msg_len);
        base_exception->message[msg_len] = '\0';
    }
    
    // 从基类的 struct SecurityException 构造
    static SMSecurityException from_base_exception(const struct SecurityException* base_exception)
    {
        if (base_exception == nullptr)
        {
            return SMSecurityException("Unknown security exception");
        }
        
        return SMSecurityException(
            std::string(base_exception->message),
            static_cast<int32_t>(base_exception->minor_code));
    }
    
private:

    std::string message_;

    int32_t minor_;
};

TRAVODDS_NAMESPACE_END

#endif // ifndef SECURITYEXCEPTION_H__