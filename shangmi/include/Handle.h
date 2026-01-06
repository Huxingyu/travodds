#ifndef HANDLE_H
#define HANDLE_H

#include <memory>
#include <string>

TRAVODDS_NAMESPACE_BEGIN
using std::string;
class SMHandle : public std::enable_shared_from_this<SMHandle>
{
public:

    const std::string& get_class_id() const
    {
        return class_id_;
    }

    virtual bool nil() const = 0;

protected:

    SMHandle(const std::string& class_id) : class_id_(class_id) {};

    virtual ~SMHandle() {}

private:

    std::string class_id_;
};

template<typename T>
class HandleImpl : public SMHandle
{
public:

    typedef T type;

    HandleImpl() : SMHandle(T::class_id_), m_impl(new T) {}

    virtual ~HandleImpl() = default;

    //基类指针转换成指向子类
    static HandleImpl<T>& narrow(SMHandle& handle)
    {
        if (handle.get_class_id().compare(T::class_id_) == 0)
            return reinterpret_cast<HandleImpl<T>&>(handle);

        return HandleImpl<T>::nil_handle;
    }

    static const HandleImpl<T>& narrow(const SMHandle& handle)
    {
        if (handle.get_class_id().compare(T::class_id_) == 0)
            return reinterpret_cast<const HandleImpl<T>&>(handle);

        return HandleImpl<T>::nil_handle;
    }

    bool nil() const override
    {
        return m_impl ? false : true;
    }

    T* operator*()
    {
        return m_impl.get();
    }

    const T* operator*() const
    {
        return m_impl.get();
    }

    T* operator->()
    {
        return m_impl.get();
    }

    const T* operator->() const
    {
        return m_impl.get();
    }
    
    T* release()
    {
        return m_impl.release();
    }

    static HandleImpl<T> nil_handle;

private:

    explicit HandleImpl(bool) : SMHandle(T::class_id_) {}

    std::unique_ptr<T> m_impl;
};

template<typename T>
HandleImpl<T> HandleImpl<T>::nil_handle(true);
class SMNilHandle : public SMHandle
{
public:

    SMNilHandle() : SMHandle("nil_handle") {}

    virtual ~SMNilHandle() = default;

    bool nil() const override { return true; }
};

typedef SMHandle SMIdentityHandle;
typedef SMHandle SMPermissionsHandle;

typedef SMHandle SMSecretHandle;
typedef SMHandle SMParticipantCryptoHandle;
typedef SMHandle SMEntityCryptoHandle;
typedef SMHandle SMDatawriterCryptoHandle;
typedef SMHandle SMDatareaderCryptoHandle;

typedef SMHandle SMHandshakeHandle;
TRAVODDS_NAMESPACE_END
#endif // HANDLE_H
