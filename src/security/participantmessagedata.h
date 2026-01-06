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
#ifndef PARTICIPANT_MESSAGE_DATA_H
#define PARTICIPANT_MESSAGE_DATA_H

#include "rtps/basictypertps.h"
TRAVODDS_NAMESPACE_BEGIN

typedef struct ParticipantMessageData
{
	/** @brief   Participant的标识。 */
	GuidPrefix_t m_participantGuidPrefix;
	/** @brief  表明Liveliness的类型。
	 *          kind.value[0] & 0x80 == 0 // reserved by RTPS
	 *          kind.value[0] & 0x80 == 1 // vendor specific kind
	 */
	char m_kind[4];
	Sequence<char> m_extraData;
} ParticipantMessageData;

typedef struct ParticipantMessageSecureData
{
	/** @brief   Participant的标识。 */
	GuidPrefix_t m_participantGuidPrefix;
	/** @brief  表明Liveliness的类型。
	 *          kind.value[0] & 0x80 == 0 // reserved by RTPS
	 *          kind.value[0] & 0x80 == 1 // vendor specific kind
	 */
	char m_kind[4];
	Sequence<char> m_extraData;
} ParticipantMessageSecureData;

TRAVODDS_NAMESPACE_END
#endif //PARTICIPANT_MESSAGE_DATA_H