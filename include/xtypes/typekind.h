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

#ifndef TYPE_KIND_H
#define TYPE_KIND_H

#include "travodds_dll.h"

TRAVODDS_NAMESPACE_BEGIN 

/**
 * @brief 枚举类型 TypeKind
 */
enum TypeKind : unsigned char {
    TK_NONE = 0x00,       /**< 无类型 */
    TK_BOOLEAN = 0x01,    /**< 布尔类型 */
    TK_BYTE = 0x02,       /**< 字节类型 */
    TK_INT16 = 0x03,      /**< 16位整数类型 */
    TK_INT32 = 0x04,      /**< 32位整数类型 */
    TK_INT64 = 0x05,      /**< 64位整数类型 */
    TK_UINT16 = 0x06,     /**< 16位无符号整数类型 */
    TK_UINT32 = 0x07,     /**< 32位无符号整数类型 */
    TK_UINT64 = 0x08,     /**< 64位无符号整数类型 */
    TK_FLOAT32 = 0x09,    /**< 32位浮点类型 */
    TK_FLOAT64 = 0x0A,    /**< 64位浮点类型 */
    TK_FLOAT128 = 0x0B,   /**< 128位浮点类型 */
    TK_CHAR8 = 0x10,      /**< 8位字符类型 */
    TK_CHAR16 = 0x11,     /**< 16位字符类型 */
    TK_STRING8 = 0x20,    /**< 8位字符串类型 */
    TK_STRING16 = 0x21,   /**< 16位字符串类型 */
    TK_ALIAS = 0x30,      /**< 别名类型 */
    TK_ENUM = 0x40,       /**< 枚举类型 */
    TK_BITMASK = 0x41,    /**< 位掩码类型 */
    TK_ANNOTATION = 0x50, /**< 注释类型 */
    TK_STRUCTURE = 0x51,  /**< 结构类型 */
    TK_UNION = 0x52,      /**< 联合类型 */
    TK_BITSET = 0x53,     /**< 位集类型 */
    TK_SEQUENCE = 0x60,   /**< 序列类型 */
    TK_ARRAY = 0x61,      /**< 数组类型 */
    TK_MAP = 0x62         /**< 映射类型 */
};

typedef unsigned int MemberId;

const MemberId MEMBER_ID_INVALID = 0xFFFFFFFF;
const MemberId MEMBER_ID_MAP_KEY = 0xFFFFFFFE;
const MemberId MEMBER_ID_MAP_VALUE = 0xFFFFFFFD;

TRAVODDS_NAMESPACE_END

#endif // TYPE_KIND_H