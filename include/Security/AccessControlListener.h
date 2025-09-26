#ifndef AccessControlListener_h__
#define AccessControlListener_h__

#include "travodds_dll.h"
#include "SecurityCommon.h"

TRAVODDS_NAMESPACE_BEGIN

class AccessControl;
typedef void* PermissionsHandle;

class AccessControlListener
{
public:

    /**
     * @fn  bool AccessControlListener::on_revoke_permissions(const AccessControl* plugin, const PermissionsHandle* handle);
     *
     * @brief   permission取消或改变时，提供一个listener.
     *
     * @param   plugin  客户端的AccessControl.
     * @param   handle  与Permissions正在取消的DDS参与者通信的Permissions的PermissionsHandle.
     *
     * @return  成功返回true，否则返回false.
     */

   virtual bool on_revoke_permissions(
        AccessControl* plugin,
        const PermissionsHandle* handle) = 0;
};

TRAVODDS_NAMESPACE_END

#endif // AccessControlListener_h__
