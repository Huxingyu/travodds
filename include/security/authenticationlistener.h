//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef AUTHENTICATION_LISTENER_H
#define AUTHENTICATION_LISTENER_H
#include "travodds_dll.h"
#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

class Authentication;
typedef void* IdentityHandle;

class AuthenticationListener
{
	virtual ~AuthenticationListener() = 0;

	/**
	 * @fn	Boolean AuthenticationListener::on_revoke_identity( Authentication plugin, IdentityHandle handle, SecurityException exception);
	 *
	 * @brief	Executes the revoke identity action.
	 *
	 * @param	plugin   	The plugin.
	 * @param	handle   	The handle.
	 * @param	exception	Details of the exception.
	 *
	 * @return	true if it succeeds, false if it fails.
	 */

	//virtual bool on_revoke_identity(
	//	Authentication* plugin,
	//	IdentityHandle handle,
	//	SecurityException exception) = 0;
};

TRAVODDS_NAMESPACE_END
#endif //DDSSECURITY
#endif //AUTHENTICATION_LISTENER_H