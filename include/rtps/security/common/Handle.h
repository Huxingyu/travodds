#ifndef HANDLE_H
#define HANDLE_H

#include <memory>
#include <string>

TRAVODDS_NAMESPACE_BEGIN
using std::string;
class Handle : public std::enable_shared_from_this<Handle>
{
public:

    const std::string& get_class_id() const
    {
        return class_id_;
    }

    virtual bool nil() const = 0;

protected:

    Handle(const std::string& class_id) : class_id_(class_id) {};

    virtual ~Handle() {}

private:

    std::string class_id_;
};

template<typename T>
class HandleImpl : public Handle
{
public:

    typedef T type;

    HandleImpl() : Handle(T::class_id_), m_impl(new T) {}

    virtual ~HandleImpl() = default;

    //基类指针转换成指向子类
    static HandleImpl<T>& narrow(Handle& handle)
    {
        if (handle.get_class_id().compare(T::class_id_) == 0)
            return reinterpret_cast<HandleImpl<T>&>(handle);

        return HandleImpl<T>::nil_handle;
    }

    static const HandleImpl<T>& narrow(const Handle& handle)
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

    explicit HandleImpl(bool) : Handle(T::class_id_) {}

    std::unique_ptr<T> m_impl;
};

template<typename T>
HandleImpl<T> HandleImpl<T>::nil_handle(true);
class NilHandle : public Handle
{
public:

    NilHandle() : Handle("nil_handle") {}

    virtual ~NilHandle() = default;

    bool nil() const override { return true; }
};

typedef Handle IdentityHandle;
typedef Handle PermissionsHandle;

typedef Handle SecretHandle;
typedef Handle ParticipantCryptoHandle;
typedef Handle EntityCryptoHandle;
typedef Handle DatawriterCryptoHandle;
typedef Handle DatareaderCryptoHandle;

typedef Handle HandshakeHandle;
TRAVODDS_NAMESPACE_END
#endif // HANDLE_H
