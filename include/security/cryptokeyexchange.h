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
#ifndef CRYPTO_KEY_EXCHANGE_H
#define CRYPTO_KEY_EXCHANGE_H
#ifdef DDSSECURITY
#include "security/crycommonsource.h"
TRAVODDS_NAMESPACE_BEGIN

class CryptoKeyExchange
{
public:

    virtual ~CryptoKeyExchange(){};

    /**
     * @fn  bool CryptoKeyExchange::create_local_participant_crypto_tokens( ParticipantCryptoTokenSeq &local_participant_crypto_tokens, ParticipantCryptoHandle locat_participant, ParticipantCryptoHandle remote_participant, SecurityException* exception);
     *
     * @brief   创建本地participant的交换信息.
     *
     * @param [out] local_participant_crypto_tokens 输出的交换信息.
     * @param   locat_participant                   本地participant的加密句柄.
     * @param   remote_participant                  远端participant的加密句柄.
     * @param [out] exception                       发生错误时的错误返回.
     *
     * @return  创建成功则返回true，失败则返回false.
     */
    virtual bool create_local_participant_crypto_tokens(
        ParticipantCryptoTokenSeq* local_participant_crypto_tokens,
        ParticipantCryptoHandle locat_participant,
        ParticipantCryptoHandle remote_participant,
        SecurityException* exception) = 0;

    /**
     * @fn  bool CryptoKeyExchange::set_remote_participant_crypto_tokens( ParticipantCryptoHandle local_participant_crypto, ParticipantCryptoHandle remote_participant_crypto, ParticipantCryptoTokenSeq remote_participant_tokens, SecurityException* exception);
     *
     * @brief   根据交换信息设置远端participant加密句柄的信息.
     *
     * @param   local_participant_crypto    本地participant的加密句柄.
     * @param   remote_participant_crypto   远端participant的加密句柄.
     * @param   remote_participant_tokens   远端participant的交换信息.
     * @param [out] exception               发生错误时的错误返回.
     *
     * @return  设置成功则返回true，失败则返回false.
     */

    virtual bool set_remote_participant_crypto_tokens(
        ParticipantCryptoHandle local_participant_crypto,
        ParticipantCryptoHandle remote_participant_crypto,
        const ParticipantCryptoTokenSeq* remote_participant_tokens,
        SecurityException* exception) = 0;

    /**
     * @fn  bool CryptoKeyExchange::create_local_datawriter_crypto_tokens( ParticipantCryptoTokenSeq &local_datawriter_crypto_tokens, DatawriterCryptoHandle local_datawriter_crypto, DatareaderCryptoHandle remote_datareader_crypto, SecurityException* exception);
     *
     * @brief   创建datawriter的交换信息.
     *
     * @param [out] local_datawriter_crypto_tokens  本地datawriter的交换信息.
     * @param   local_datawriter_crypto             本地datawriter的加密句柄.
     * @param   remote_datareader_crypto            与本地datawriter相匹配的远端的datareader加密句柄.
     * @param [out] exception                       发生错误时的错误返回.
     *
     * @return  创建成功则返回true，失败则返回false.
     */
    virtual bool create_local_datawriter_crypto_tokens(
        DatawriterCryptoTokenSeq* local_datawriter_crypto_tokens,
        DatawriterCryptoHandle local_datawriter_crypto,
        DatareaderCryptoHandle remote_datareader_crypto,
        SecurityException* exception) = 0;

    /**
     * @fn  bool CryptoKeyExchange::set_remote_datawriter_crypto_tokens( DatareaderCryptoHandle local_datareader_crypto, DatawriterCryptoHandle remote_datawriter_crypto, int remote_datawriter_token, SecurityException* exception);
     *
     * @brief   根据远端datawriter的交换信息设置远端datawriter的加密句柄.
     *
     * @param   local_datareader_crypto     与远端datawriter相匹配的本地datareader的加密句柄.
     * @param   remote_datawriter_crypto    远端datawriter的加密句柄.
     * @param   remote_datawriter_token     远端datawriter的交换信息.
     * @param [out] exception               发生错误时的错误返回.
     *
     * @return  设置成功则返回true，失败则返回false.
     */

    virtual bool set_remote_datawriter_crypto_tokens(
        DatareaderCryptoHandle local_datareader_crypto,
        DatawriterCryptoHandle remote_datawriter_crypto,
        const DatawriterCryptoTokenSeq* remote_datawriter_token,
        SecurityException* exception) = 0;

    /**
     * @fn  bool CryptoKeyExchange::create_local_datareader_crypto_tokens( ParticipantCryptoTokenSeq &local_datareader_crypto_tokens, DatareaderCryptoHandle local_datareader_crypto, DatawriterCryptoHandle remote_datawriter_crypto, SecurityException* exception);
     *
     * @brief   创建datareader的交换信息.
     *
     * @param [out]  local_datareader_crypto_tokens     dataereader的交换信息.
     * @param   local_datareader_crypto                 本地的datareader的加密句柄.
     * @param   remote_datawriter_crypto                与本地datareader相匹配的远端datawriter的加密句柄.
     * @param [out] exception                           发生错误时的错误返回.
     *
     * @return  创建成功则返回true，失败则返回false.
     */
    virtual bool create_local_datareader_crypto_tokens(
        DatareaderCryptoTokenSeq* local_datareader_crypto_tokens,
        DatareaderCryptoHandle local_datareader_crypto,
        DatawriterCryptoHandle remote_datawriter_crypto,
        SecurityException* exception) = 0;

    /**
     * @fn  bool CryptoKeyExchange::set_remote_datareader_crypto_tokens( DatawriterCryptoHandle local_datawriter_crypto, DatareaderCryptoHandle remote_datareader_crypto, ParticipantCryptoTokenSeq remote_datareader_tokens, SecurityException* exception);
     *
     * @brief   根据远端datareader的交换信息设置远端datareader的加密句柄.
     *
     * @param   local_datawriter_crypto     与远端datareader相匹配的本地datawriter的加密句柄.
     * @param   remote_datareader_crypto    远端datareader的加密句柄.
     * @param   remote_datareader_tokens    远端datareader的交换信息.
     * @param [out] exception               发生错误时的错误返回.
     *
     * @return  设置成功则返回true，失败则返回false.
     */
    virtual bool set_remote_datareader_crypto_tokens(
        DatawriterCryptoHandle local_datawriter_crypto,
        DatareaderCryptoHandle remote_datareader_crypto,
        const DatareaderCryptoTokenSeq* remote_datareader_tokens,
        SecurityException* exception) = 0;

    /**
     * @fn  bool CryptoKeyExchange::return_crypto_tokens( ParticipantCryptoTokenSeq crypto_tokens, SecurityException* exception);
     *
     * @brief   释放交换信息使用的空间.
     *
     * @param   crypto_tokens   交换信息的集合.
     * @param [out] exception   发生错误时的错误返回.
     *
     * @return  执行成功则返回true，失败则返回fasle.
     */
    virtual bool return_crypto_tokens(
        CryptoTokenSeq* crypto_tokens,
        SecurityException* exception) = 0;
};

TRAVODDS_NAMESPACE_END
#endif //DDSSECURITY
#endif // CRYPTO_KEY_EXCHANGE_H
