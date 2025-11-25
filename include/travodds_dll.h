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

#ifndef TRAVODDS_DLL_H
#define TRAVODDS_DLL_H

//#define _DDS_DLL_FILE
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

/* windows */
#ifdef _DDS_STATIC_LIB
#define DDS_DLL
#else

#ifdef _DDS_DLL_FILE
#define DDS_DLL __declspec(dllexport)
#else
#define DDS_DLL __declspec(dllimport)
#endif
#endif

#else/* linux */

#define DDS_DLL

#endif

#define TRAVODDS_NAMESPACE_BEGIN		namespace TRAVODDS { //
#define TRAVODDS_NAMESPACE_END			}
#define USING_TRAVODDS_NAMESPACE		using namespace TRAVODDS;

 #define _XML_INTERFACE_CPP  1

#define DDSSECURITY

#endif