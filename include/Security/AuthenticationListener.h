#ifndef AuthenticationListener_h__
#define AuthenticationListener_h__

#include "travodds_dll.h"
#include "SecurityCommon.h"

TRAVODDS_NAMESPACE_BEGIN
class Authentication;
//typedef void* IdentityHandle;

class  AuthenticationListener
{
    virtual ~AuthenticationListener() = 0;

    /**
    * @fn	Boolean AuthenticationListener::on_revoke_identity( Authentication plugin, IdentityHandle handle, SecurityException exception);
    *
    * @brief	Executes the revoke identity action.
    *
    * @author	lin
    * @date	2018/1/31
    *
    * @param	plugin   	The plugin.
    * @param	handle   	The handle.
    * @param	exception	Details of the exception.
    *
    * @return	true if it succeeds, false if it fails.
    */

    virtual bool on_revoke_identity(
        Authentication* plugin,
        IdentityHandle handle,
        SecurityException* exception) = 0;
};

TRAVODDS_NAMESPACE_END
#endif