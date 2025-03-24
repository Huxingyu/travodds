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
#pragma once
#include "AGIDDS_DLL.h"
AGIDDS_NAMESPACE_BEGIN
// ----------------------------------------------------------------------
// Return codes
// ----------------------------------------------------------------------
using ReturnCode_t = long ;
const	ReturnCode_t	RETCODE_OK = 0;
const	ReturnCode_t	RETCODE_ERROR = 1;
const	ReturnCode_t	RETCODE_UNSUPPORTED = 2;
const	ReturnCode_t	RETCODE_BAD_PARAMETER = 3;
const	ReturnCode_t	RETCODE_PRECONDITION_NOT_MET = 4;
const	ReturnCode_t	RETCODE_OUT_OF_RESOURCES = 5;
const	ReturnCode_t	RETCODE_NOT_ENABLED = 6;
const	ReturnCode_t	RETCODE_IMMUTABLE_POLICY = 7;
const	ReturnCode_t	RETCODE_INCONSISTENT_POLICY = 8;
const	ReturnCode_t	RETCODE_ALREADY_DELETED = 9;
const	ReturnCode_t	RETCODE_TIMEOUT = 10;
const	ReturnCode_t	RETCODE_NO_DATA = 11;
const	ReturnCode_t	RETCODE_ILLEGAL_OPERATION = 12;

AGIDDS_NAMESPACE_END
