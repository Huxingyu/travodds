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
#ifndef _SECURITY_AUTHENTICATION_SM4GCMGMAC_TRANSFORM_H_
#define _SECURITY_AUTHENTICATION_SM4GCMGMAC_TRANSFORM_H_

#include "security/cryptotransform.h"
#include "cryptography/SM4GCMGMAC_Types.h"

TRAVODDS_NAMESPACE_BEGIN

class SM4GCMGMAC_Transform : public CryptoTransform{
public:
    SM4GCMGMAC_Transform();
    ~SM4GCMGMAC_Transform();

    // 基类接口实现（适配器函数，带 override）
    bool get_rtps_message_encode_attributes(
        ParticipantCryptoHandle participant_crypto,
        CryptoRtpsMessageAttributes* rtps_message_encode_attributes) override;

    // 实现函数（保持原有签名）
    bool get_rtps_message_encode_attributes_impl(
        SMParticipantCryptoHandle& participant_crypto,
        CryptoRtpsMessageAttributes& rtps_message_encode_attributes);

    // 基类接口实现（适配器函数，带 override）
    bool get_datawriter_submessage_encode_attributes(
        DatawriterCryptoHandle sending_datawriter_crypto,
        CryptoWriterSubmessageAttributes* writer_submessage_encode_attributes) override;

    // 实现函数（保持原有签名）
    bool get_datawriter_submessage_encode_attributes_impl(
        SMDatawriterCryptoHandle& sending_datawriter_crypto,
        CryptoWriterSubmessageAttributes& writer_submessage_encode_attributes);

    // 基类接口实现（适配器函数，带 override）
    bool get_datareader_submessage_encode_attributes(
        DatareaderCryptoHandle datareader_crypto,
        CryptoReaderSubmessageAttributes* reader_submessage_encode_attributes) override;

    // 实现函数（保持原有签名）
    bool get_datareader_submessage_encode_attributes_impl(
        SMDatareaderCryptoHandle& datareader_crypto,
        CryptoReaderSubmessageAttributes& reader_submessage_encode_attributes);

    // 基类接口实现（适配器函数，带 override）
    bool get_payload_encode_attributes(
        DatawriterCryptoHandle datawriter_crypto,
        CryptoPayloadEncodeAttributes* payload_encode_attributes) override;

    // 实现函数（保持原有签名）
    bool get_payload_encode_attributes_impl(
        SMDatawriterCryptoHandle& datawriter_crypto,
        CryptoPayloadEncodeAttributes& payload_encode_attributes);

    // 基类接口实现（适配器函数，带 override）
    bool preprocess_secure_submsg(
        DatawriterCryptoHandle* datawriter_crypto,
        DatareaderCryptoHandle* datareader_crypto,
        SecureSubmessageCategory_t* secure_submessage_category,
        const SerializedBuffer* encoded_rtps_submessage,
        int encoded_rtps_submessage_count,
        ParticipantCryptoHandle receiving_crypto,
        ParticipantCryptoHandle sending_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool preprocess_secure_submsg_impl(
        SMDatawriterCryptoHandle& datawriter_crypto,
        SMDatareaderCryptoHandle& datareader_crypto,
        SecureSubmessageCategory_t& secure_submessage_category,
        const SerializedBuffer& encoded_rtps_submessage,
        int encoded_rtps_submessage_count,
        SMParticipantCryptoHandle&  receiving_crypto,
        SMParticipantCryptoHandle&  sending_crypto,
        SMSecurityException& exception);

    bool deserialize_secure_submessage(
        const SerializedBuffer* encoded,
        int encoded_count,
        SecurePrefixSubMsg& out_prefix,
        CryptoContent& out_body,
        SecurePostfixSubMsg& out_postfix);

    bool deserialize_secure_rtps_message(
        const SerializedBuffer* encoded, 
        int encoded_count,
        SecureRTPSPrefixSubMsg& out_prefix,
        CryptoContent& out_body,
        SecureRTPSPostfixSubMsg& out_postfix);

    // 基类接口实现（适配器函数，带 override）
    bool encode_rtps_message(
        SerializedBuffer* encoded_rtps_message,
        const SerializedBuffer* plain_rtps_message,
        int plain_rtps_message_count,
        ParticipantCryptoHandle sending_crypto,
        const ParticipantCryptoHandle* receiving_crypto_list,
        int receiving_crypto_list_num,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool encode_rtps_message_impl(
        SerializedBuffer& encoded_rtps_message,
        const SerializedBuffer* plain_rtps_message,
        int plain_rtps_message_count,
        SMParticipantCryptoHandle& sending_crypto,
        std::vector<SMParticipantCryptoHandle*>& receiving_crypto_list,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool decode_rtps_message(
        SerializedBuffer* plain_buffer,
        const SerializedBuffer* encoded_buffer,
        int encoded_buffer_count,
        ParticipantCryptoHandle receiving_crypto,
        ParticipantCryptoHandle sending_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool decode_rtps_message_impl(
        SerializedBuffer& plain_buffer,
        const SerializedBuffer* encoded_buffer,
        int encoded_buffer_count,
        SMParticipantCryptoHandle&  receiving_crypto,
        SMParticipantCryptoHandle&  sending_crypto,
        SMSecurityException& exception);
    
    int serialize_secure_rtps_message(
        unsigned char* output_buf,
        unsigned int output_buf_length,
        const SecureRTPSPrefixSubMsg& data_header,
        const CryptoContent& data_body,
        const SecureRTPSPostfixSubMsg& data_tag,
        const unsigned char* tag_bytes, 
        unsigned int tag_len,
        bool little_endian);
    
    int serialize_crypto_content(
        unsigned char* output_buf,
        unsigned int output_buf_length,
        const CryptoContent* data_body,
        bool little_endian);

    int serialize_secure_submessage(
        unsigned char* output_buf,
        unsigned int output_buf_length,
        const SecurePrefixSubMsg& prefix,
        const CryptoContent& body,
        const SecurePostfixSubMsg& postfix,
        bool little_endian);

    // 基类接口实现（适配器函数，带 override）
    bool encode_datawriter_submessage(
        SerializedBuffer* encoded_rtps_submessage,
        const SerializedBuffer* plain_rtps_submessage,
        int plain_rtps_submessage_count,
        DatawriterCryptoHandle sending_datawriter_crypto,
        const DatareaderCryptoHandle* receiving_datareader_crypto_list,
        int receiving_datareader_crypto_num,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool encode_datawriter_submessage_impl(
        SerializedBuffer& encoded_rtps_submessage,
        const SerializedBuffer* plain_rtps_submessage,
        int plain_rtps_submessage_count,
        SMDatawriterCryptoHandle& sending_datawriter_crypto,
        const SMDatareaderCryptoHandle* receiving_datareader_crypto_list,
        int receiving_datareader_crypto_num,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool decode_datawriter_submessage(
        SerializedBuffer* plain_rtps_submessage,
        const SerializedBuffer* encoded_rtps_submessage,
        int encoded_rtps_submessage_count,
        DatareaderCryptoHandle receiving_datareader_crypto,
        DatawriterCryptoHandle sending_datawriter_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool decode_datawriter_submessage_impl(
        SerializedBuffer& plain_rtps_submessage,
        const SerializedBuffer* encoded_rtps_submessage,
        int encoded_rtps_submessage_count,
        SMDatareaderCryptoHandle& receiving_datareader_crypto,
        SMDatawriterCryptoHandle& sending_datawriter_crypto,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool encode_datareader_submessage(
        SerializedBuffer* encoded_rtps_submessage,
        const SerializedBuffer* plain_rtps_submessage,
        int plain_rtps_submessage_count,
        DatareaderCryptoHandle sending_datareader_crypto,
        const DatawriterCryptoHandle* receiving_datawriter_crypto_list,
        int receiving_datawriter_crypto_list_num,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool encode_datareader_submessage_impl(
        SerializedBuffer& encoded_rtps_submessage,
        const SerializedBuffer* plain_rtps_submessage,
        int plain_rtps_submessage_count,
        SMDatareaderCryptoHandle& sending_datareader_crypto,
        const SMDatawriterCryptoHandle* receiving_datawriter_crypto_list,
        int receiving_datawriter_crypto_list_num,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool decode_datareader_submessage(
        SerializedBuffer* plain_rtps_submessage,
        const SerializedBuffer* encoded_rtps_submessage,
        int encoded_rtps_submessage_count,
        DatawriterCryptoHandle receiving_datawriter_crypto,
        DatareaderCryptoHandle sending_datareader_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool decode_datareader_submessage_impl(
        SerializedBuffer& plain_rtps_submessage,
        const SerializedBuffer* encoded_rtps_submessage,
        int encoded_rtps_submessage_count,
        SMDatawriterCryptoHandle& receiving_datawriter_crypto,
        SMDatareaderCryptoHandle& sending_datareader_crypto,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool encode_serialized_payload(
        SerializedBuffer* encoded_buffer,
        SerializedBuffer* extra_inline_qos,
        const SerializedBuffer* plain_buffer,
        int plain_buffer_count,
        DatawriterCryptoHandle sending_datawriter_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool encode_serialized_payload_impl(
        SerializedBuffer& encoded_buffer,
        SerializedBuffer& extra_inline_qos,
        const SerializedBuffer* plain_buffer,
        int plain_buffer_count,
        SMDatawriterCryptoHandle& sending_datawriter_crypto,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool decode_serialized_payload(
        SerializedBuffer* plain_buffer,
        const SerializedBuffer* encoded_buffer,
        int encoded_buffer_count,
        const SerializedBuffer* inline_qos,
        int inline_qos_count,
        DatareaderCryptoHandle receiving_datareader_crypto,
        DatawriterCryptoHandle sending_datawriter_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool decode_serialized_payload_impl(
        SerializedBuffer& plain_buffer,
        const SerializedBuffer* encoded_buffer,
        int encoded_buffer_count,
        const SerializedBuffer& inline_qos,
        int inline_qos_count,
        SMDatareaderCryptoHandle& receiving_datareader_crypto,
        SMDatawriterCryptoHandle& sending_datawriter_crypto,
        SMSecurityException& exception);

    /**
     * @brief 计算并序列化 SecureDataTag (common_mac + receiver_specific_macs)
     * * @param output_buf 输出缓冲区
     * @param output_buf_length 缓冲区总长度
     * @param common_mac 预先计算好的通用 MAC
     * @param iv 用于派生会话密钥和作为 AAD 的初始化向量
     * @param receiving_crypto_list 接收方加密句柄列表
     * @param exception 发生错误时的异常信息
     * @return 成功写入的字节数，或 -1 表示失败
     */
    int serialize_SecureDataTag(
        unsigned char* output_buf,
        unsigned int output_buf_length,
        const unsigned char common_mac[16],
        const unsigned char iv[12],
        const std::vector<const SMParticipantCryptoHandle*>& receiving_crypto_list,
        bool little_endian,
        SMSecurityException& exception);

    int serialize_SecureDataHeader(
        unsigned char* out,
        unsigned int out_cap,
        const CryptoHeader& hdr,
        SMSecurityException& exception);

    int serialize_SecureDataBody(
        unsigned char* out,
        unsigned int out_cap,
        bool little_endian,
        const uint8_t* body_data,
        uint32_t body_len,
        SMSecurityException& exception);

    void compute_sessionkey(
        std::array<uint8_t, 16>& session_key,
        const KeyMaterial_SM4_GCM_GMAC& key_mat,
        const uint32_t session_id);
};

TRAVODDS_NAMESPACE_END

#endif //_SECURITY_AUTHENTICATION_SM4GCMGMAC_TRANSFORM_H_