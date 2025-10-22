#include "common/log/logger.h"
#include "cryptography/SM4GCMGMAC_Transform.h"
#include "rtps/messages/cdrmessage_t.h"
#include "auth/GMHelper.h"
#include <arpa/inet.h>

using namespace TRAVODDS;

bool is_msg_profile_empty(const MessageFramingProfile& input_profile) {
    return input_profile.header_fixed == 0 && input_profile.header_mutable_length == 0 &&
           input_profile.encrypt_percentage == 0 && input_profile.tail_fixed == 0 &&
           input_profile.tail_mutable_length == 0;
}

void fill_default_message_attrs(CryptoRtpsMessageAttributes& input_attributes) {
    input_attributes.headerFixLength = sizeof(SecureRTPSPostfixSubMsg) + sizeof(int) + sizeof(int) + 24;;
    input_attributes.headerMutableLengthPerParticipant = 0;
    input_attributes.encodeDataLengthPercentage = 100;
    input_attributes.tailFixLength =  16 + sizeof(int);
    input_attributes.tailMutableLengthPerParticipant = 20;
}

void fill_writer_submsg_defaults(CryptoWriterSubmessageAttributes& input_attributes) {
    input_attributes.headerFixLength = sizeof(SecurePrefixSubMsg) + sizeof(int) + sizeof(int);;
    input_attributes.headerMutableLengthPerReader = 0;
    input_attributes.encodeDataLengthPercentage = 100;
    input_attributes.tailFixLength = 16 + sizeof(int);
    input_attributes.tailMutableLengthPerReader = 20;
}

void fill_reader_submsg_defaults(CryptoReaderSubmessageAttributes& input_attributes) {
    input_attributes.headerFixLength = sizeof(SecurePrefixSubMsg) + sizeof(int) + sizeof(int);
    input_attributes.headerMutableLengthPerWriter = 0;
    input_attributes.encodeDataLengthPercentage = 100;
    input_attributes.tailFixLength = 16 + sizeof(int);
    input_attributes.tailMutableLengthPerWriter = 20;
}

static inline uint16_t read_u16(const unsigned char* base, size_t len, size_t& off, bool little) {
    if (!(base && (off + 2) <= len)) return 0;
    uint16_t v;
    if (little) {
        v = static_cast<uint16_t>(base[off]) | (static_cast<uint16_t>(base[off + 1]) << 8);
    } else {
        v = static_cast<uint16_t>(base[off + 1]) | (static_cast<uint16_t>(base[off]) << 8);
    }
    off += 2;
    return v;
}

// 其它 32 位字段按 L 位对应端序解析
static inline uint32_t read_u32(const unsigned char* base, size_t len, size_t& off, bool little) {
    if (!(base && (off + 4) <= len)) return 0;
    uint32_t v;
    if (little) {
        v = (static_cast<uint32_t>(base[off])      ) |
            (static_cast<uint32_t>(base[off+1])<< 8) |
            (static_cast<uint32_t>(base[off+2])<<16) |
            (static_cast<uint32_t>(base[off+3])<<24);
    } else {
        v = (static_cast<uint32_t>(base[off])<<24) |
            (static_cast<uint32_t>(base[off+1])<<16)|
            (static_cast<uint32_t>(base[off+2])<< 8)|
            (static_cast<uint32_t>(base[off+3])    );
    }
    off += 4;
    return v;
}

static inline bool read_bytes(const unsigned char* base, size_t len, size_t& off,
                              void* out, size_t nbytes) {
    if (!(base && (off + nbytes) <= len)) return false;
    std::memcpy(out, base + off, nbytes);
    off += nbytes;
    return true;
}

bool SM4GCMGMAC_Transform::deserialize_secure_submessage(
    const SerializedBuffer* encoded,
    int encoded_count,
    SecurePrefixSubMsg& out_prefix,
    CryptoContent& out_body,
    SecurePostfixSubMsg& out_postfix)
{
    if (!encoded || encoded_count <= 0 || !encoded->buffer || encoded->buffer_size < 1) {
        return false;
    }

    const unsigned char* p   = reinterpret_cast<const unsigned char*>(encoded->buffer);
    const size_t len = static_cast<size_t>(encoded->buffer_size);
    size_t off = 0;

    const unsigned char* buf = reinterpret_cast<const unsigned char*>(encoded->buffer);

    auto need = [&](size_t n)->bool { return off + n <= len; };

    if (!need(1)) return false;
    out_prefix.msg_kind = static_cast<uint8_t>(buf[off++]);
    if (out_prefix.msg_kind != SEC_PREFIX) return false;

    if (!need(1)) return false;
    out_prefix.flags = static_cast<uint8_t>(buf[off++]);
    const bool little = (out_prefix.flags & 0x01u) != 0;

    if (!need(2)) return false;
    out_prefix.octets_to_next_header = read_u16(buf, len, off, little);

    if (!need(sizeof(CryptoHeader))) return false;
    std::memcpy(&out_prefix.sec_data_header, buf + off, sizeof(CryptoHeader));
    off += sizeof(CryptoHeader);

    if (!need(1)) return false;
    out_body.header.msg_kind = static_cast<uint8_t>(buf[off++]);
    if (out_body.header.msg_kind != SEC_BODY) return false;

    if (!need(1)) return false;
    out_body.header.flags = static_cast<uint8_t>(buf[off++]);
    // if (((out_body.header.flags & 0x01u) != 0) != little) return false;

    if (!need(2)) return false;
    out_body.header.octets_to_next_header = read_u16(buf, len, off, little);

    if (!need(4)) return false;
    const uint32_t body_len = read_u32(buf, len, off, /*little=*/false);
    if (!need(body_len)) return false;

    unsigned char* body_ptr = const_cast<unsigned char*>(buf + off);

    const size_t footer_off = off + static_cast<size_t>(body_len);
    if (footer_off > len) return false;
    off = footer_off;

    if (!need(1)) return false;
    out_postfix.msg_kind = static_cast<uint8_t>(buf[off++]);
    if (out_postfix.msg_kind != SEC_POSTFIX) return false;

    if (!need(1)) return false;
    out_postfix.flags = static_cast<uint8_t>(buf[off++]);
    // if (((out_postfix.flags & 0x01u) != 0) != little) return false;

    if (!need(2)) return false;
    out_postfix.octets_to_next_header = read_u16(buf, len, off, little);

    if (!need(out_postfix.octets_to_next_header)) return false;
    const size_t tag_start = off;
    const size_t tag_end   = off + out_postfix.octets_to_next_header;

    // common_mac
    if (!need(16)) return false;
    std::memcpy(out_postfix.sec_data_tag.common_mac, buf + off, 16);
    off += 16;

    // receiver_count
    if (!need(4)) return false;
    const uint32_t rec_cnt = read_u32(buf, len, off, little);

    // 剩余 tag 字节
    if (tag_end < off) return false;
    size_t tag_left = tag_end - off;

    if (rec_cnt > (tag_left / 20)) return false;

    out_postfix.sec_data_tag.receiver_specific_macs.clear();
    out_postfix.sec_data_tag.receiver_specific_macs.reserve(rec_cnt);

    for (uint32_t i = 0; i < rec_cnt; ++i) {
        // 每项 20 字节：key_id(4) + mac(16)
        if (!need(4 + 16)) return false;

        ReceiverSpecificMAC rec{};

        // key_id
        uint32_t kid_u32 = read_u32(buf, len, off, little);
        static_assert(sizeof(rec.receiver_mac_key_id) == 4, "receiver_mac_key_id must be 4 bytes");
        std::memcpy(&rec.receiver_mac_key_id, &kid_u32, 4);

        // mac
        std::memcpy(rec.receiver_mac, buf + off, 16);
        off += 16;

        out_postfix.sec_data_tag.receiver_specific_macs.push_back(rec);
    }

    // if (off != tag_end) return false;

    out_body.body_length = body_len;
    out_body.secure_data = body_ptr;

    const auto& kind = out_prefix.sec_data_header.transform_identifier.transformation_kind;
    if (kind == c_transform_kind_sm4_gmac) {
        out_body.body_flag = CryptoContent::SIGN;
    } else if (kind == c_transform_kind_sm4_gcm) {
        out_body.body_flag = CryptoContent::ENCRYPT;
    } else {
        return false;
    }

    out_body.data_buffers.src_buffer        = encoded;
    out_body.data_buffers.buffers_num       = static_cast<unsigned int>(encoded_count);
    out_body.data_buffers.cur_buffer_index  = 0;
    out_body.data_buffers.cur_buffer_offset = static_cast<unsigned int>(
        reinterpret_cast<const unsigned char*>(body_ptr) -
        reinterpret_cast<const unsigned char*>(encoded->buffer)
    );
    out_body.data_buffers.little_endian = little;

    return true;
}

void get_initialzation_vector(
    const CryptoHeader* header,
    /*out*/ unsigned char iv[12]){
    if (header == nullptr || iv == nullptr)
    return;

    std::memcpy(iv, header->session_id, 4);
    std::memcpy(iv + 4, header->initialization_vector_suffix, 8);
};

// 派生本次子消息会话密钥
bool get_key_material(
    unsigned char session_key[16],
    const unsigned char iv[12],
    const KeyMaterial_SM4_GCM_GMAC& km,
    bool use_receiver_specific_key = false)
{
    if (session_key == nullptr || iv == nullptr)
        return false;

    // 准备输入数据
    std::vector<uint8_t> key_vec;
    if (use_receiver_specific_key)
    {
        key_vec.assign(km.master_receiver_specific_key.begin(), km.master_receiver_specific_key.end());
    }
    else
    {
        key_vec.assign(km.master_sender_key.begin(), km.master_sender_key.end());
    }
    std::vector<uint8_t> salt_vec(km.master_salt.begin(), km.master_salt.end());
    std::vector<uint8_t> iv_vec(iv, iv + 12);

    // 使用 HMAC-SM3 派生 16 字节会话密钥
    std::vector<uint8_t> data;
    data.reserve(salt_vec.size() + iv_vec.size());
    data.insert(data.end(), salt_vec.begin(), salt_vec.end());
    data.insert(data.end(), iv_vec.begin(), iv_vec.end());

    std::vector<uint8_t> mac_out;
    if (!GMHelper::hmac_sm3(key_vec, data, mac_out) || mac_out.empty()) {
        // 派生失败时退回主密钥
        std::memcpy(session_key, key_vec.data(), 16);
        return false;
    }

    // 取前 16 字节作为本次会话密钥
    std::memcpy(session_key, mac_out.data(), 16);
    return true;
};

// 返回主机是否为小端
static inline bool host_is_little_endian() noexcept {
    static const uint16_t one = 1;
    return *reinterpret_cast<const uint8_t*>(&one) == 1;
}

// 将原始字节序写入并按指定的目标端序进行字节序变换
static inline unsigned char* write_data_endian(
    unsigned char* dst,
    const unsigned char* src,
    size_t elements_num,
    size_t elem_len,
    bool little_endian) noexcept
{
    if (!dst || !src || elem_len == 0) {
        return nullptr;
    }

    // 如果主机端序与目标端序一致，或元素为单字节，直接 memcpy
    const bool need_swap = (host_is_little_endian() != little_endian);
    const size_t total = elements_num * elem_len;

    if (!need_swap || elem_len == 1) {
        std::memcpy(dst, src, total);
        return dst + total;
    }

    switch (elem_len) {
        case 2: {
            for (size_t i = 0; i < total; i += 2) {
                dst[i]     = src[i + 1];
                dst[i + 1] = src[i];
            }
            return dst + total;
        }
        case 4: {
            for (size_t i = 0; i < total; i += 4) {
                dst[i]     = src[i + 3];
                dst[i + 1] = src[i + 2];
                dst[i + 2] = src[i + 1];
                dst[i + 3] = src[i];
            }
            return dst + total;
        }
        case 8: {
            for (size_t i = 0; i < total; i += 8) {
                dst[i]     = src[i + 7];
                dst[i + 1] = src[i + 6];
                dst[i + 2] = src[i + 5];
                dst[i + 3] = src[i + 4];
                dst[i + 4] = src[i + 3];
                dst[i + 5] = src[i + 2];
                dst[i + 6] = src[i + 1];
                dst[i + 7] = src[i];
            }
            return dst + total;
        }
        default: {
            // 通用翻转：逐元素把每个元素内的字节倒序写入
            for (size_t e = 0; e < elements_num; ++e) {
                const unsigned char* s = src + e * elem_len;
                unsigned char*       d = dst + e * elem_len;
                for (size_t j = 0; j < elem_len; ++j) {
                    d[j] = s[elem_len - 1 - j];
                }
            }
            return dst + total;
        }
    }
}

static unsigned char* write_u16(unsigned char* dst, uint16_t v, bool little_endian) noexcept {
    return write_data_endian(dst, reinterpret_cast<unsigned char*>(&v), 1, sizeof(v), little_endian);
}
static inline void write_u16_at(unsigned char* base, size_t pos, uint16_t v, bool little_endian) noexcept {
    write_data_endian(base + pos, reinterpret_cast<unsigned char*>(&v), 1, sizeof(v), little_endian);
}
static unsigned char* write_u32(unsigned char* dst, uint32_t v, bool little_endian) noexcept {
    return write_data_endian(dst, reinterpret_cast<unsigned char*>(&v), 1, sizeof(v), little_endian);
}
static unsigned char* write_u64(unsigned char* dst, uint64_t v, bool little_endian) noexcept {
    return write_data_endian(dst, reinterpret_cast<unsigned char*>(&v), 1, sizeof(v), little_endian);
}

// 根据明文起始若干字节判断子消息类别：DATAWRITER_SUBMESSAGE / DATAREADER_SUBMESSAGE / INFO_SUBMESSAGE / INVALID_SUBMESSAGE
SecureSubmessageCategory_t get_submessage_kind(
    const unsigned char* peek,
    unsigned int avail_len)
{
    if (peek == nullptr || avail_len < 1)
        return INVALID_SUBMESSAGE;

    const unsigned char id = peek[0];

    switch (id)
    {
        case DATA:       // 0x15
        case DATA_FRAG:  // 0x16
            return DATAWRITER_SUBMESSAGE;

        case HEARTBEAT:      // 0x07
        case HEARTBEAT_FRAG: // 0x13
        case GAP:            // 0x08
            return DATAREADER_SUBMESSAGE;

        case INFO_TS:     // 0x09
        case INFO_SRC:    // 0x0c
        case INFO_REPLY:  // 0x0f
        case INFO_DST:    // 0x0e
        case INFO_REPLY_IP4:
            return INFO_SUBMESSAGE;

        default:
            return INVALID_SUBMESSAGE;
    }
};

// 校验接收端 MAC（用于匹配正确的 reader/writer 句柄）
bool verify_receiver_macs(
    const CryptoFooter* tag,
    const unsigned char iv[12],
    const KeyMaterial_SM4_GCM_GMAC& km)
{
    if (tag == nullptr || iv == nullptr)
        return false;

    std::vector<uint8_t> key_vec(km.master_sender_key.begin(),
                                 km.master_sender_key.end());

    std::vector<uint8_t> iv_vec(iv, iv + 12);

    std::vector<uint8_t> plaintext;       // 空数据
    std::vector<uint8_t> aad(iv_vec);     // 把 IV 当作附加认证数据
    std::vector<uint8_t> ciphertext;      // 空密文
    std::vector<uint8_t> calc_tag;        // 输出的 tag
    calc_tag.reserve(16);

    // 计算认证标签
    bool ok = GMHelper::sm4_gcm_encrypt_vector(
        key_vec, iv_vec, aad, plaintext, ciphertext, calc_tag);

    if (!ok || calc_tag.size() != 16)
        return false;

    // 比较接收到的 common_mac
    if (std::memcmp(tag->common_mac, calc_tag.data(), 16) == 0)
        return true;

    // 若有多接收者 MAC，逐个验证 
    for (const auto& rx : tag->receiver_specific_macs)
    {
        if (std::memcmp(rx.receiver_mac, calc_tag.data(), 16) == 0)
            return true;
    }

    return false;
}

bool SM4GCMGMAC_Transform::get_rtps_message_encode_attributes(
    ParticipantCryptoHandle& participant_crypto,
    CryptoRtpsMessageAttributes& attr)
{
    // 窄化到具体实现的 participant 句柄
    SM4GCMGMAC_ParticipantCryptoHandle& pc = SM4GCMGMAC_ParticipantCryptoHandle::narrow(participant_crypto);
    if (pc.nil()) {
        fill_default_message_attrs(attr);
        return true;
    }

    // 直接从句柄里的帧配置拷贝到输出
    const MessageFramingProfile& profile = pc->msg_framing;

    if (!is_msg_profile_empty(profile)) {
        attr.headerFixLength  = profile.header_fixed;
        attr.headerMutableLengthPerParticipant = profile.header_mutable_length;
        attr.encodeDataLengthPercentage = profile.encrypt_percentage;
        attr.tailFixLength = profile.tail_fixed;
        attr.tailMutableLengthPerParticipant = profile.tail_mutable_length;
        return true;
    }

    fill_default_message_attrs(attr);
    return true;
}

bool SM4GCMGMAC_Transform::get_datawriter_submessage_encode_attributes(
    DatawriterCryptoHandle& sending_datawriter_crypto,
    CryptoWriterSubmessageAttributes& writer_submessage_encode_attributes)
{
    SM4GCMGMAC_WriterCryptoHandle& writer = SM4GCMGMAC_WriterCryptoHandle::narrow(sending_datawriter_crypto);
    if (writer.nil()) { fill_writer_submsg_defaults(writer_submessage_encode_attributes); return true; }

    const MessageFramingProfile& profile = writer->submsg_framing;
    if (is_msg_profile_empty(profile)) { fill_writer_submsg_defaults(writer_submessage_encode_attributes); return true; }

    writer_submessage_encode_attributes.headerFixLength = profile.header_fixed;
    writer_submessage_encode_attributes.headerMutableLengthPerReader = profile.header_mutable_length;
    writer_submessage_encode_attributes.encodeDataLengthPercentage = profile.encrypt_percentage;
    writer_submessage_encode_attributes.tailFixLength = profile.tail_fixed;
    writer_submessage_encode_attributes.tailMutableLengthPerReader = profile.tail_mutable_length;
    return true;
}

bool SM4GCMGMAC_Transform::get_datareader_submessage_encode_attributes(
    DatareaderCryptoHandle& datareader_crypto,
    CryptoReaderSubmessageAttributes& reader_submessage_encode_attributes)
{
    SM4GCMGMAC_ReaderCryptoHandle& reader = SM4GCMGMAC_ReaderCryptoHandle::narrow(datareader_crypto);
    if (reader.nil()) { fill_reader_submsg_defaults(reader_submessage_encode_attributes); return true; }

    const MessageFramingProfile& profile = reader->submsg_framing;
    if (is_msg_profile_empty(profile)) { fill_reader_submsg_defaults(reader_submessage_encode_attributes); return true; }

    reader_submessage_encode_attributes.headerFixLength = profile.header_fixed;
    reader_submessage_encode_attributes.headerMutableLengthPerWriter = profile.header_mutable_length;
    reader_submessage_encode_attributes.encodeDataLengthPercentage = profile.encrypt_percentage;
    reader_submessage_encode_attributes.tailFixLength = profile.tail_fixed;
    reader_submessage_encode_attributes.tailMutableLengthPerWriter = profile.header_mutable_length;
    return true;
}

bool SM4GCMGMAC_Transform::get_payload_encode_attributes(
    DatawriterCryptoHandle& datawriter_crypto,
    CryptoPayloadEncodeAttributes& payload_encode_attributes)
{
    SM4GCMGMAC_WriterCryptoHandle& writer = SM4GCMGMAC_WriterCryptoHandle::narrow(datawriter_crypto);
    if (writer.nil()) return false;

    // body 的长度字段占 4 字节
    payload_encode_attributes.headerFixLength = sizeof(CryptoHeader) + sizeof(int);
    payload_encode_attributes.encodeDataLengthPercentage = 100;  // 全加密
    payload_encode_attributes.tailFixLength = 16 + sizeof(int);  // GCM/GMAC TAG + 长度字段
    return true;
}

bool SM4GCMGMAC_Transform::preprocess_secure_submsg( 
    DatawriterCryptoHandle& datawriter_crypto,
    DatareaderCryptoHandle& datareader_crypto,
    SecureSubmessageCategory_t& secure_submessage_category,
    const SerializedBuffer& encoded_rtps_submessage,
    int encoded_rtps_submessage_count,
    ParticipantCryptoHandle& receiving_crypto,
    ParticipantCryptoHandle& sending_crypto,
    SecurityException& exception)
{
    if (encoded_rtps_submessage.buffer_size < 1) {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid encoded_rtps_submessage length");
        exception = SecurityException("Invalid submessage length");
        return false;
    }
    const unsigned char submsg_kind_octet =
        *reinterpret_cast<const unsigned char*>(encoded_rtps_submessage.buffer);
    if (submsg_kind_octet == INFO_TS || submsg_kind_octet == INFO_SRC ||
        submsg_kind_octet == INFO_DST || submsg_kind_octet == INFO_REPLY ||
        submsg_kind_octet == INFO_REPLY_IP4)
    {
        secure_submessage_category = INFO_SUBMESSAGE;
        return true;
    }

    // 反序列化 Secure 前缀/体/后缀
    SecurePrefixSubMsg data_header{};
    CryptoContent data_body{};
    SecurePostfixSubMsg data_tag{};
    if (!deserialize_secure_submessage(&encoded_rtps_submessage,
                                       encoded_rtps_submessage_count,
                                       data_header, data_body, data_tag))
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Failed to deserialize secure submessage.");
        exception = SecurityException("Secure submessage deserialization failed");
        return false;
    }

    const CryptoTransformKeyId sender_key_id =
        data_header.sec_data_header.transform_identifier.transformation_key_id;
    const CryptoTransformKind xform_kind =
        data_header.sec_data_header.transform_identifier.transformation_kind;

    unsigned char iv[12]{};
    get_initialzation_vector(&data_header.sec_data_header, iv);

    SM4GCMGMAC_ParticipantCryptoHandle& local_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(receiving_crypto);
    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(sending_crypto);
    if (local_participant.nil() || remote_participant.nil()) {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid ParticipantCryptoHandle");
        exception = SecurityException("Not a valid ParticipantCryptoHandle received");
        return false;
    }

    unsigned char  peek_buf[16] = {0};
    unsigned char* peek_bytes   = peek_buf;
    unsigned int   peek_len     = 0;

    if (xform_kind == c_transform_kind_sm4_gmac)
    {
        // GMAC：数据未加密，可直接用数据体起始做分类
        peek_bytes = reinterpret_cast<unsigned char*>(data_body.secure_data);
        peek_len   = data_body.body_length;
    }
    else if (xform_kind == c_transform_kind_sm4_gcm)
    {
        unsigned char session_key[16] = {0};
        const bool key_id_is_zero = (sender_key_id == c_transformKeyIdZero);

        if (key_id_is_zero)
        {
            // Participant2ParticipantKxKeyMaterial 是 vector；取第一个可用，否则退回 ParticipantKeyMaterial
            if (!remote_participant->Participant2ParticipantKxKeyMaterial.empty())
            {
                get_key_material(session_key, iv,
                                 remote_participant->Participant2ParticipantKxKeyMaterial.front());
            }
            else
            {
                get_key_material(session_key, iv,
                                 remote_participant->ParticipantKeyMaterial);
            }
        }
        else
        {
            bool material_found = false;

            // 在远端 Writers 中按 sender_key_id 匹配
            for (const auto& wt_sp : remote_participant->Writers)
            {
                SM4GCMGMAC_WriterCryptoHandle& writer =
                    SM4GCMGMAC_WriterCryptoHandle::narrow(*wt_sp);
                if (writer.nil()) continue;

                const auto& wMats = writer->Entity2RemoteKeyMaterial;
                if (!wMats.empty() && wMats.front().sender_key_id == sender_key_id)
                {
                    get_key_material(session_key, iv, wMats.front());
                    material_found = true;
                    break;
                }
            }

            // 若没找到，继续在远端 Readers 中匹配
            if (!material_found)
            {
                for (const auto& read_sp : remote_participant->Readers)
                {
                    SM4GCMGMAC_ReaderCryptoHandle& reader =
                        SM4GCMGMAC_ReaderCryptoHandle::narrow(*read_sp);
                    if (reader.nil()) continue;

                    const auto& rMats = reader->Entity2RemoteKeyMaterial;
                    if (!rMats.empty() && rMats.front().sender_key_id == sender_key_id)
                    {
                        get_key_material(session_key, iv, rMats.front());
                        material_found = true;
                        break;
                    }
                }
            }

            if (!material_found)
            {
                REC_LOG_WARNING(SECURITY_CRYPTO, "Key identifier not found on remote participant");
                exception = SecurityException("Key identifier not found");
                return false;
            }
        }

        unsigned int got = 0;
        int dec = GMHelper::sm4_gcm_gmac_decrypt(
            xform_kind,
            session_key, iv,
            data_body.data_buffers,   // BuffersContainer
            8,
            data_tag.sec_data_tag.common_mac, // 16B
            peek_buf, &got
        );

        peek_len = (dec > 0) ? static_cast<unsigned int>(dec) : 0;
        peek_bytes = (peek_len > 0) ? peek_buf : nullptr;
    }
    else
    {
        exception = SecurityException("Invalid transformation kind (expect SM4_GCM/SM4_GMAC)");
        return false;
    }

    // 判别子消息类型
    SecureSubmessageCategory_t detected = INVALID_SUBMESSAGE;
    if (peek_bytes && peek_len > 0) {
        detected = get_submessage_kind(peek_bytes, peek_len);
    }

    // 遍历远端实体定位
    auto match_remote_writer = [&](SM4GCMGMAC_WriterCryptoHandle& writer)->bool {
        const auto& wMats = writer->Entity2RemoteKeyMaterial;
        return (!wMats.empty() && wMats.front().sender_key_id == sender_key_id);
    };
    auto match_remote_reader = [&](SM4GCMGMAC_ReaderCryptoHandle& reader)->bool {
        const auto& rMats = reader->Entity2RemoteKeyMaterial;
        return (!rMats.empty() && rMats.front().sender_key_id == sender_key_id);
    };

    // 来自远端 writer，去本地找 reader
    if (detected == DATAWRITER_SUBMESSAGE || detected == INVALID_SUBMESSAGE)
    {
        for (const auto& wt_sp : remote_participant->Writers)
        {
            SM4GCMGMAC_WriterCryptoHandle& writer =
                SM4GCMGMAC_WriterCryptoHandle::narrow(*wt_sp);
            if (writer.nil()) continue;
            if (!match_remote_writer(writer)) continue;

            // 在本地读者中寻找对应关系
            for (const auto& read_sp : local_participant->Readers)
            {
                SM4GCMGMAC_ReaderCryptoHandle& reader =
                    SM4GCMGMAC_ReaderCryptoHandle::narrow(*read_sp);
                if (reader.nil()) continue;

                // 没有现成的 MAC 验证/上下文，先按 key_id 匹配
                datawriter_crypto = writer;
                datareader_crypto = reader;
                secure_submessage_category = DATAWRITER_SUBMESSAGE;
                return true;
            }
        }
    }

    // 来自远端 reader，去本地找 writer
    if (detected == DATAREADER_SUBMESSAGE || detected == INVALID_SUBMESSAGE)
    {
        for (const auto& read_sp : remote_participant->Readers)
        {
            SM4GCMGMAC_ReaderCryptoHandle& reader =
                SM4GCMGMAC_ReaderCryptoHandle::narrow(*read_sp);
            if (reader.nil()) continue;
            if (!match_remote_reader(reader)) continue;

            for (const auto& wt_sp : local_participant->Writers)
            {
                SM4GCMGMAC_WriterCryptoHandle& writer =
                    SM4GCMGMAC_WriterCryptoHandle::narrow(*wt_sp);
                if (writer.nil()) continue;

                datawriter_crypto = writer;
                datareader_crypto = reader;
                secure_submessage_category = DATAREADER_SUBMESSAGE;
                return true;
            }
        }
    }

    // 未匹配
    secure_submessage_category = INVALID_SUBMESSAGE;
    exception = SecurityException("Cannot determine datawriter or datareader.");
    return false;
}

static uint8_t fix_flags_L(uint8_t flags, bool little_endian) noexcept {
    flags = static_cast<uint8_t>(flags & ~0x01u);   // 清 L 位
    if (little_endian) flags = static_cast<uint8_t>(flags | 0x01u);
    return flags;
}

bool SM4GCMGMAC_Transform::encode_rtps_message(
    SerializedBuffer& encoded_rtps_message,
    const SerializedBuffer* plain_rtps_message,
    int plain_rtps_message_count,
    ParticipantCryptoHandle& sending_crypto,
    std::vector<ParticipantCryptoHandle*>& receiving_crypto_list,
    SecurityException& exception)
{
    if (encoded_rtps_message.buffer == nullptr || encoded_rtps_message.buffer_size == 0) {
        exception = SecurityException("encoded_rtps_message buffer is null or zero size");
        return false;
    }
    if (plain_rtps_message == nullptr || plain_rtps_message_count <= 0) {
        exception = SecurityException("plain_rtps_message is null or empty");
        return false;
    }

    SM4GCMGMAC_ParticipantCryptoHandle& local_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(sending_crypto);
    if (local_participant.nil()) {
        exception = SecurityException("Invalid sending_crypto handle");
        return false;
    }

    std::unique_lock<std::mutex> lock(local_participant->mutex_);

    bool update_specific_keys = false;
    // 检查会话块计数器是否已达上限
    if (local_participant->max_blocks_per_session > 0 &&
        local_participant->Session.session_block_counter >= local_participant->max_blocks_per_session)
    {
        // 达到上限，触发会话更新
        local_participant->Session.session_id += 1;
        update_specific_keys = true; // 标记需要更新接收方特定密钥
        compute_sessionkey(local_participant->Session.SessionKey,
            local_participant->ParticipantKeyMaterial,
            local_participant->Session.session_id);
        
        local_participant->Session.session_block_counter = 0;
    }
    local_participant->Session.session_block_counter += 1;

    // IV/Nonce 生成
    unsigned char iv_suffix[8];
    if (RAND_bytes(iv_suffix, sizeof(iv_suffix)) != 1)
    {
        exception = SecurityException("Failed to generate random IV suffix");
        return false;
    }

    std::array<uint8_t, 12> iv{};
    // session_id (4 字节) + random suffix (8 字节)
    uint32_t session_id_le = local_participant->Session.session_id;
    std::memcpy(iv.data(), &session_id_le, 4);
    std::memcpy(iv.data() + 4, iv_suffix, 8);

    // 准备 SecureRTPSPrefix
    SecureRTPSPrefixSubMsg prefix{};
    prefix.msg_kind = SRTPS_PREFIX;
    prefix.flags = host_is_little_endian() ? 0x01 : 0x00;
    
    // 填充 CryptoHeader
    prefix.sec_data_header.transform_identifier.transformation_kind = local_participant->ParticipantKeyMaterial.transformation_kind;
    prefix.sec_data_header.transform_identifier.transformation_key_id = local_participant->ParticipantKeyMaterial.sender_key_id;
    std::memcpy(prefix.sec_data_header.session_id, &session_id_le, 4);
    std::memcpy(prefix.sec_data_header.initialization_vector_suffix, iv_suffix, 8);

    // 从 plain_rtps_message 提取 20 字节的 RTPS Header
    {
        int remained = 20;
        unsigned char* dst = prefix.rtps_header;
        for (int i = 0; i < plain_rtps_message_count && remained > 0; ++i) {
            const SerializedBuffer& seg = plain_rtps_message[i];
            const int cp = std::min(remained, static_cast<int>(seg.buffer_size));
            std::memcpy(dst, seg.buffer, cp);
            remained -= cp;
            dst += cp;
        }
        if (remained != 0) {
            exception = SecurityException("Failed to gather 20B RTPS header from input buffers");
            return false;
        }
    }
    
    // 准备加密内容
    std::vector<uint8_t> plain_concat;

    size_t total_len = 0;
    for (int i = 0; i < plain_rtps_message_count; ++i) {
        const SerializedBuffer& seg = plain_rtps_message[i];
        if (seg.buffer == nullptr) {
            exception = SecurityException("plain_rtps_message segment has null buffer");
            return false;
        }
        total_len += static_cast<size_t>(seg.buffer_size);
    }
    if (total_len < 20) {
        exception = SecurityException("Plain RTPS message shorter than 20-byte header");
        return false;
    }

    const size_t payload_len = total_len - 20;
    plain_concat.reserve(payload_len);

    size_t bytes_to_skip = 20;
    for (int i = 0; i < plain_rtps_message_count; ++i) {
        const SerializedBuffer& seg = plain_rtps_message[i];
        const uint8_t* p   = reinterpret_cast<const uint8_t*>(seg.buffer);
        size_t         len = static_cast<size_t>(seg.buffer_size);

        if (bytes_to_skip >= len) {
            bytes_to_skip -= len;
            continue;
        }

        const uint8_t* start = p + bytes_to_skip;
        const size_t copy_len = len - bytes_to_skip;
        plain_concat.insert(plain_concat.end(), start, start + copy_len);
        bytes_to_skip = 0;
    }

    if (plain_concat.size() != payload_len) {
        exception = SecurityException("Assembled payload size mismatch after 20-byte header");
        return false;
    }
    
    // 加密或签名
    CryptoContent body{};
    body.header.msg_kind = SEC_BODY;
    body.header.flags    = prefix.flags;

    std::vector<uint8_t> cipher;
    std::vector<uint8_t> tag_vec;
    const std::vector<uint8_t> key_vec(local_participant->Session.SessionKey.begin(), local_participant->Session.SessionKey.end());
    const std::vector<uint8_t> iv_vec(iv.begin(), iv.end());

    const bool little = (prefix.flags & 0x01) != 0;

    unsigned char aad_buf[64];
    unsigned char* w = aad_buf;
    const unsigned char* const aad_begin = w;

    *w++ = static_cast<unsigned char>(SRTPS_PREFIX);
    *w++ = static_cast<unsigned char>(prefix.flags);
    const size_t len_pos = static_cast<size_t>(w - aad_begin);
    w = write_u16(w, /*placeholder*/0u, little);

    {
        const auto& tk = prefix.sec_data_header.transform_identifier.transformation_kind;
        std::memcpy(w, tk.data(), tk.size());
        w += tk.size();
    }
    std::memcpy(w, prefix.sec_data_header.transform_identifier.transformation_key_id.data(),
            prefix.sec_data_header.transform_identifier.transformation_key_id.size());
    w += prefix.sec_data_header.transform_identifier.transformation_key_id.size();

    std::memcpy(w, prefix.sec_data_header.session_id, 4);
    w += 4;

    std::memcpy(w, prefix.sec_data_header.initialization_vector_suffix, 8);
    w += 8;

    const uint16_t cryptoheader_len = static_cast<uint16_t>((w - aad_begin) - (len_pos + 2));
    write_u16_at(const_cast<unsigned char*>(aad_begin), len_pos, cryptoheader_len, little);

    prefix.octets_to_next_header = cryptoheader_len;

    const std::vector<uint8_t> aad_vec(reinterpret_cast<const uint8_t*>(aad_begin), reinterpret_cast<const uint8_t*>(w));

    if (prefix.sec_data_header.transform_identifier.transformation_kind == c_transform_kind_sm4_gmac)
    {
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, {}, cipher, tag_vec)) {
            exception = SecurityException("SM4-GMAC signing failed");
            return false;
        }
        body.body_flag   = CryptoContent::SIGN;
        body.body_length = static_cast<uint32_t>(plain_concat.size());
        body.secure_data = plain_concat.data();
    }
    else if (prefix.sec_data_header.transform_identifier.transformation_kind == c_transform_kind_sm4_gcm)
    {
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, plain_concat, cipher, tag_vec)) {
            exception = SecurityException("SM4-GCM encryption failed");
            return false;
        }
        body.body_flag   = CryptoContent::ENCRYPT;
        body.body_length = static_cast<uint32_t>(cipher.size());
        body.secure_data = cipher.data();
    }
    else {
        exception = SecurityException("Unsupported transform kind for sender");
        return false;
    }
    body.header.octets_to_next_header = static_cast<uint16_t>(4 + body.body_length);
    if (tag_vec.size() != 16) {
        exception = SecurityException("SM4-GCM/GMAC tag size invalid");
        return false;
    }
    
    SecureRTPSPostfixSubMsg postfix{};
    postfix.msg_kind = SRTPS_POSTFIX;
    postfix.flags    = prefix.flags;
    std::memcpy(postfix.sec_data_tag.common_mac, tag_vec.data(), 16);

    std::vector<const ParticipantCryptoHandle*> rec_list_const;
    rec_list_const.reserve(receiving_crypto_list.size());
    for (auto* h : receiving_crypto_list)
    {
        if (h) rec_list_const.push_back(h);
    }

    const size_t max_tag_len = 16u + 4u + rec_list_const.size() * (4u + 16u);
    std::vector<unsigned char> tag_blob(max_tag_len);

    int tag_len = serialize_SecureDataTag(
        tag_blob.data(),
        static_cast<unsigned int>(tag_blob.size()),
        tag_vec.data(),
        iv.data(),
        rec_list_const,
        little,
        exception);

    if (tag_len < 0) {
        return false; // 内部已设置 exception
    }
    tag_blob.resize(static_cast<size_t>(tag_len));
    postfix.octets_to_next_header = static_cast<uint16_t>(tag_blob.size());
    // 序列化三段到输出缓冲
    const int wrote = serialize_secure_rtps_message(
        reinterpret_cast<unsigned char*>(encoded_rtps_message.buffer),
        static_cast<unsigned int>(encoded_rtps_message.buffer_size),
        prefix, 
        body, 
        postfix, 
        tag_blob.data(),
        static_cast<unsigned int>(tag_blob.size()), 
        host_is_little_endian());

    if (wrote < 0) {
        exception = SecurityException("serialize_secure_rtps_message failed");
        return false;
    }

    encoded_rtps_message.buffer_size = static_cast<unsigned int>(wrote);
    return true;
}

bool SM4GCMGMAC_Transform::decode_rtps_message(
    SerializedBuffer& plain_buffer,
    const SerializedBuffer* encoded_buffer,
    int encoded_buffer_count,
    ParticipantCryptoHandle& receiving_crypto,
    ParticipantCryptoHandle& sending_crypto,
    SecurityException& exception)
{
    if (!encoded_buffer || encoded_buffer_count <= 0) {
        exception = SecurityException("encoded_buffer is null or count <= 0");
        return false;
    }
    if (!plain_buffer.buffer || plain_buffer.buffer_size == 0) {
        exception = SecurityException("plain_buffer is null or zero-sized");
        return false;
    }

    SM4GCMGMAC_ParticipantCryptoHandle& local_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(receiving_crypto);
    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(sending_crypto);

    if (local_participant.nil() || remote_participant.nil()) {
        exception = SecurityException("Not a valid ParticipantCryptoHandle received");
        return false;
    }

    // 反序列化 SecureRTPS 消息
    SecureRTPSPrefixSubMsg  data_header{};
    CryptoContent           data_body{};
    SecureRTPSPostfixSubMsg data_tag{};
    if (!deserialize_secure_rtps_message(encoded_buffer, encoded_buffer_count,
                                         data_header, data_body, data_tag)) {
        exception = SecurityException("Secure RTPS deserialization failed");
        return false;
    }

    if (data_header.msg_kind != SRTPS_PREFIX) {
        exception = SecurityException("Unexpected msg_kind in Secure RTPS prefix");
        return false;
    }

    const CryptoTransformKind xform_kind =
        data_header.sec_data_header.transform_identifier.transformation_kind;

    // 计算 IV (12B = 4B session_id + 8B suffix)
    unsigned char iv[12]{};
    get_initialzation_vector(&data_header.sec_data_header, iv);

    // 重建 AAD 
    unsigned char aad_buf[64];
    unsigned char* w = aad_buf;
    const unsigned char* const aad_begin = aad_buf;
    const bool little = (data_header.flags & 0x01u) != 0;

    // msg_kind, flags
    *w++ = static_cast<unsigned char>(data_header.msg_kind);
    *w++ = static_cast<unsigned char>(data_header.flags);

    // length(2B) 占位
    const size_t len_pos = static_cast<size_t>(w - aad_begin);
    w = write_u16(w, 0u, little);

    // transformation_kind(4B)
    const auto& tk = data_header.sec_data_header.transform_identifier.transformation_kind;
    std::memcpy(w, tk.data(), tk.size());
    w += tk.size();

    // key_id(4B)
    const auto& kid_arr = data_header.sec_data_header.transform_identifier.transformation_key_id;
    std::memcpy(w, kid_arr.data(), kid_arr.size());
    w += kid_arr.size();

    // session_id(4B)
    const unsigned char* sid_arr = data_header.sec_data_header.session_id;
    std::memcpy(w, sid_arr, 4);
    w += 4;

    // iv_suffix(8B)
    const auto& sfx = data_header.sec_data_header.initialization_vector_suffix;
    std::memcpy(w, sfx, sizeof(sfx));
    w += sizeof(sfx);

    // 回填 length (CryptoHeader 总长度)
    const uint16_t cryptoheader_len = static_cast<uint16_t>(w - aad_begin);
    write_u16_at(const_cast<unsigned char*>(aad_begin), len_pos, cryptoheader_len, little);

    const std::vector<uint8_t> aad_vec(
        reinterpret_cast<const uint8_t*>(aad_begin),
        reinterpret_cast<const uint8_t*>(w));

    // 查找密钥材料并派生会话密钥
    const unsigned char* session_bytes = data_header.sec_data_header.session_id;

    const uint32_t sid_u32 = (static_cast<uint32_t>(session_bytes[0]) << 24) |
                             (static_cast<uint32_t>(session_bytes[1]) << 16) |
                             (static_cast<uint32_t>(session_bytes[2]) <<  8) |
                             (static_cast<uint32_t>(session_bytes[3]));
    std::array<uint8_t, 16> session_key{};

    const auto& wanted_key_id = data_header.sec_data_header.transform_identifier.transformation_key_id;
    bool material_found = false;

    // 在远端 Writers 查找
    for (const auto& wt_sp : remote_participant->Writers) {
        SM4GCMGMAC_WriterCryptoHandle& writer = SM4GCMGMAC_WriterCryptoHandle::narrow(*wt_sp);
        if (writer.nil()) continue;
        const auto& wMats = writer->Entity2RemoteKeyMaterial;
        for (const auto& km : wMats) {
            if (km.sender_key_id == wanted_key_id) {
                compute_sessionkey(session_key, km, sid_u32);
                material_found = true;
                break;
            }
        }
        if (material_found) break;
    }

    // 若未找到, 在远端 Readers 查找
    if (!material_found) {
        for (const auto& rd_sp : remote_participant->Readers) {
            SM4GCMGMAC_ReaderCryptoHandle& reader = SM4GCMGMAC_ReaderCryptoHandle::narrow(*rd_sp);
            if (reader.nil()) continue;
            const auto& rMats = reader->Entity2RemoteKeyMaterial;
            for (const auto& km : rMats) {
                if (km.sender_key_id == wanted_key_id) {
                    compute_sessionkey(session_key, km, sid_u32);
                    material_found = true;
                    break;
                }
            }
            if (material_found) break;
        }
    }

    if (!material_found) {
        exception = SecurityException("Key identifier not found on remote participant");
        return false;
    }

    unsigned char* out = reinterpret_cast<unsigned char*>(plain_buffer.buffer);
    size_t         used = 0;

    // 复制 RTPS Header 到明文 buffer
    {
        if (plain_buffer.buffer_size < used + sizeof(data_header.rtps_header)) {
            exception = SecurityException("plain_buffer too small for RTPS header");
            return false;
        }
        std::memcpy(out + used, data_header.rtps_header, sizeof(data_header.rtps_header));
        used += sizeof(data_header.rtps_header);
    }

    std::vector<uint8_t> concat;
    if (data_body.data_buffers.src_buffer && data_body.data_buffers.buffers_num > 0) {
        concat.reserve(data_body.data_buffers.src_buffer->buffer_size * data_body.data_buffers.buffers_num);
        for (unsigned int i = 0; i < data_body.data_buffers.buffers_num; ++i) {
            const SerializedBuffer* buf = data_body.data_buffers.src_buffer + i;
            if (!buf || !buf->buffer || buf->buffer_size == 0) continue;
            const unsigned char* p = reinterpret_cast<const unsigned char*>(buf->buffer);
            concat.insert(concat.end(), p, p + buf->buffer_size);
        }
    }

    if (xform_kind == c_transform_kind_sm4_gcm)
    {
        if (concat.empty()) {
            exception = SecurityException("Empty ciphertext for SM4-GCM");
            return false;
        }

        std::vector<uint8_t> key_vec(16);
        std::memcpy(key_vec.data(), session_key.data(), 16);

        std::vector<uint8_t> iv_vec(iv, iv + 12);
        std::vector<uint8_t> tag_vec(data_tag.sec_data_tag.common_mac,
                                     data_tag.sec_data_tag.common_mac + 16);
        std::vector<uint8_t> plaintext;

        const bool ok = GMHelper::sm4_gcm_decrypt_vector(
            key_vec, iv_vec, aad_vec, concat, tag_vec, plaintext);

        if (!ok) {
            exception = SecurityException("SM4-GCM auth/decrypt failed");
            return false;
        }

        if (plain_buffer.buffer_size < used + plaintext.size()) {
            exception = SecurityException("plain_buffer too small for plaintext");
            return false;
        }

        std::memcpy(out + used, plaintext.data(), plaintext.size());
        used += plaintext.size();
    }
    else if (xform_kind == c_transform_kind_sm4_gmac)
    {
        std::vector<uint8_t> key_vec(16);
        std::memcpy(key_vec.data(), session_key.data(), 16);

        std::vector<uint8_t> iv_vec(iv, iv + 12);
        std::vector<uint8_t> recomputed_tag, dummy_ct;

        // GMAC (认证 AAD 和空明文)
        const bool ok_tag = GMHelper::sm4_gcm_encrypt_vector(
            key_vec, iv_vec, aad_vec,
            /*pt=*/{}, /*ct_out=*/dummy_ct, recomputed_tag);

        if (!ok_tag || recomputed_tag.size() != 16 ||
            std::memcmp(recomputed_tag.data(), data_tag.sec_data_tag.common_mac, 16) != 0)
        {
            exception = SecurityException("SM4-GMAC auth verify failed");
            return false;
        }

        // 认证通过
        if (plain_buffer.buffer_size < used + concat.size()) {
            exception = SecurityException("plain_buffer too small for plaintext");
            return false;
        }
        std::memcpy(out + used, concat.data(), concat.size());
        used += concat.size();
    }
    else
    {
        exception = SecurityException("Invalid transformation kind (expect SM4_GCM or SM4_GMAC)");
        return false;
    }

    plain_buffer.buffer_size = used;
    return true;
}

bool SM4GCMGMAC_Transform::deserialize_secure_rtps_message(
    const SerializedBuffer* encoded, 
    int encoded_count,
    SecureRTPSPrefixSubMsg& out_prefix,
    CryptoContent& out_body,
    SecureRTPSPostfixSubMsg& out_postfix)
{
    if (!encoded || encoded_count <= 0 || !encoded->buffer || encoded->buffer_size < 1) {
        return false;
    }

    // 将第一个 buffer 视为单个连续流
    const unsigned char* buf = reinterpret_cast<const unsigned char*>(encoded->buffer);
    size_t len = static_cast<size_t>(encoded->buffer_size);
    size_t off = 0;

    // 辅助检查剩余字节
    auto need = [&](size_t n) -> bool { return off + n <= len; };

    if (!read_bytes(buf, len, off, out_prefix.rtps_header, 20)) {
        return false;
    }

    // msg_kind
    if (!read_bytes(buf, len, off, &out_prefix.msg_kind, 1)) {
        return false;
    }

    // flags (决定端序)
    if (!read_bytes(buf, len, off, &out_prefix.flags, 1)) {
        return false;
    }
    const bool little = (out_prefix.flags & 0x01u) != 0;

    // length
    if (!need(2)) return false;
    out_prefix.octets_to_next_header = read_u16(buf, len, off, little);

    // CryptoHeader
    if (!read_bytes(buf, len, off, &out_prefix.sec_data_header, sizeof(out_prefix.sec_data_header))) {
        return false;
    }

    // msg_kind (SEC_BODY)
    if (!read_bytes(buf, len, off, &out_body.header.msg_kind, 1)) {
        return false;
    }

    // flags
    if (!read_bytes(buf, len, off, &out_body.header.flags, 1)) {
        return false;
    }
    const bool body_little = (out_body.header.flags & 0x01u) != 0;

    // length
    if (!need(2)) return false;
    out_body.header.octets_to_next_header = read_u16(buf, len, off, body_little);

    // body_length
    if (!need(4)) return false;
    out_body.body_length = read_u32(buf, len, off, /*little=*/false);

    // 检查 payload 长度
    if (!need(out_body.body_length)) return false;
    const unsigned char* body_ptr = buf + off;
    off += out_body.body_length;

    // 4 字节对齐
    const size_t align = ((off + 3) & ~size_t(3)) - off;
    if (align > 0) {
        if (!need(align)) return false;
        off += align;
    }

    // 填充 out_body 结构
    out_body.secure_data = const_cast<unsigned char*>(body_ptr);
    out_body.data_buffers.src_buffer = encoded;
    out_body.data_buffers.buffers_num = static_cast<unsigned int>(encoded_count);
    out_body.data_buffers.cur_buffer_index = 0;
    out_body.data_buffers.cur_buffer_offset =
        static_cast<unsigned int>(body_ptr - buf);
    out_body.data_buffers.little_endian = body_little;

    // msg_kind
    if (!read_bytes(buf, len, off, &out_postfix.msg_kind, 1)) {
        return false;
    }

    // flags
    if (!read_bytes(buf, len, off, &out_postfix.flags, 1)) {
        return false;
    }
    const bool tag_little = (out_postfix.flags & 0x01u) != 0;

    // common_mac
    if (!read_bytes(buf, len, off, out_postfix.sec_data_tag.common_mac, 16)) {
        return false;
    }

    // receiver_count
    if (!need(4)) return false;
    const uint32_t rec_cnt = read_u32(buf, len, off, /*little=*/false);

    out_postfix.sec_data_tag.receiver_specific_macs.clear();
    out_postfix.sec_data_tag.receiver_specific_macs.reserve(rec_cnt);

    for (uint32_t i = 0; i < rec_cnt; ++i) {
        ReceiverSpecificMAC rec{};
        // 4B key_id
        if (!read_bytes(buf, len, off, &rec.receiver_mac_key_id, 4)) {
            return false;
        }
        // 16B mac
        if (!read_bytes(buf, len, off, rec.receiver_mac, 16)) {
            return false;
        }
        out_postfix.sec_data_tag.receiver_specific_macs.push_back(rec);
    }

    // 回填正确的长度
    out_postfix.octets_to_next_header = static_cast<uint16_t>(16 + 4 + rec_cnt * (4 + 16));
    // if (tag_len_read != out_postfix.octets_to_next_header) return false;

    return true;
}

bool SM4GCMGMAC_Transform::encode_datawriter_submessage(
    SerializedBuffer& encoded_rtps_submessage,
    const SerializedBuffer* plain_rtps_submessage,
    int plain_rtps_submessage_count,
    DatawriterCryptoHandle& sending_datawriter_crypto,
    const DatareaderCryptoHandle* receiving_datareader_crypto_list,
    int receiving_datareader_crypto_num,
    SecurityException& exception)
{
    if (encoded_rtps_submessage.buffer == nullptr || encoded_rtps_submessage.buffer_size == 0) {
        exception = SecurityException("encoded_rtps_submessage buffer is null or zero size");
        return false;
    }
    if (plain_rtps_submessage == nullptr || plain_rtps_submessage_count <= 0) {
        exception = SecurityException("plain_rtps_submessage is null or empty");
        return false;
    }
    if (receiving_datareader_crypto_num > 0 && receiving_datareader_crypto_list == nullptr) {
        exception = SecurityException("receiving_datareader_crypto_list is null but count > 0");
        return false;
    }

    SM4GCMGMAC_WriterCryptoHandle& writer = SM4GCMGMAC_WriterCryptoHandle::narrow(sending_datawriter_crypto);
    if (writer.nil()) {
        exception = SecurityException("Invalid DatawriterCryptoHandle");
        return false;
    }
    if (writer->EntityKeyMaterial.empty()) {
        exception = SecurityException("Writer EntityKeyMaterial is empty");
        return false;
    }
    const KeyMaterial_SM4_GCM_GMAC& sender_key = writer->EntityKeyMaterial.front();
    if (sender_key.transformation_kind == c_transform_kind_none) {
        exception = SecurityException("Writer key material kind is NONE");
        return false;
    }

    const bool little_endian = host_is_little_endian();

    SecurePrefixSubMsg prefix{};
    prefix.msg_kind = SEC_PREFIX;
    prefix.flags    = little_endian ? 0x01 : 0x00;

    // CryptoHeader
    prefix.sec_data_header.transform_identifier.transformation_kind   = sender_key.transformation_kind;
    prefix.sec_data_header.transform_identifier.transformation_key_id = sender_key.sender_key_id;

    uint32_t sid = 0;
    unsigned char iv_suffix[8];

    if (auto parent_sp = writer->Parent_participant.lock())
    {
        SM4GCMGMAC_ParticipantCryptoHandle& parent = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*parent_sp);
        if (parent.nil())
        {
            exception = SecurityException("Writer->Parent_participant is nil");
            return false;
        }

        std::unique_lock<std::mutex> lock(parent->mutex_);

        if (parent->max_blocks_per_session > 0 &&
            parent->Session.session_block_counter >= parent->max_blocks_per_session)
        {
            parent->Session.session_id += 1;
            compute_sessionkey(parent->Session.SessionKey,
                            parent->ParticipantKeyMaterial,
                            parent->Session.session_id);
            parent->Session.session_block_counter = 0;
        }
        parent->Session.session_block_counter += 1;

        sid = parent->Session.session_id;
    }
    else
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Writer has no parent participant; fallback to random session_id");
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&sid), 4) != 1)
        {
            exception = SecurityException("Failed to generate fallback random session_id");
            return false;
        }
    }

    // 8B 随机后缀
    if (RAND_bytes(iv_suffix, sizeof(iv_suffix)) != 1)
    {
        exception = SecurityException("Failed to generate random IV suffix");
        return false;
    }

    // 把 sid/iv_suffix 写入 CryptoHeader
    std::memcpy(prefix.sec_data_header.session_id, &sid, 4);
    std::memcpy(prefix.sec_data_header.initialization_vector_suffix, iv_suffix, 8);

    // 组装 12B IV：session_id(4) + iv_suffix(8)
    std::array<uint8_t,12> iv{};
    std::memcpy(iv.data(), &sid, 4);
    std::memcpy(iv.data() + 4, iv_suffix, 8);

    // 会话密钥派生
    unsigned char session_key[16] = {0};
    (void)get_key_material(session_key, iv.data(), sender_key /*use sender master key*/);

    // 串接明文
    std::vector<uint8_t> pt_concat;
    pt_concat.reserve(256);
    for (int i = 0; i < plain_rtps_submessage_count; ++i) {
        const SerializedBuffer& seg = plain_rtps_submessage[i];
        if (seg.buffer && seg.buffer_size) {
            const auto* p = reinterpret_cast<const uint8_t*>(seg.buffer);
            pt_concat.insert(pt_concat.end(), p, p + seg.buffer_size);
        }
    }

    // AAD 组装
    unsigned char aad_buf[64];
    unsigned char* w = aad_buf;
    const unsigned char* const aad_begin = w;

    *w++ = static_cast<unsigned char>(SEC_PREFIX);
    *w++ = static_cast<unsigned char>(fix_flags_L(prefix.flags, little_endian));
    const size_t len_pos = static_cast<size_t>(w - aad_begin);
    w = write_u16(w, /*placeholder*/0u, little_endian);

    {
        const auto& tk = prefix.sec_data_header.transform_identifier.transformation_kind;
        std::memcpy(w, tk.data(), tk.size());
        w += tk.size();
    }
    std::memcpy(w, prefix.sec_data_header.transform_identifier.transformation_key_id.data(), 4);
    w += 4;

    std::memcpy(w, prefix.sec_data_header.session_id, 4);
    w += 4;

    std::memcpy(w, prefix.sec_data_header.initialization_vector_suffix, 8);
    w += 8;

    const uint16_t cryptoheader_len =
        static_cast<uint16_t>((w - aad_begin) - (len_pos + 2));
    write_u16_at(const_cast<unsigned char*>(aad_begin), len_pos, cryptoheader_len, little_endian);
    prefix.octets_to_next_header = cryptoheader_len;

    const std::vector<uint8_t> aad_vec(reinterpret_cast<const uint8_t*>(aad_begin),
                                       reinterpret_cast<const uint8_t*>(w));

    CryptoContent body{};
    body.header.msg_kind = SEC_BODY;
    body.header.flags    = little_endian ? 0x01 : 0x00;

    const std::vector<uint8_t> key_vec(session_key, session_key + 16);
    const std::vector<uint8_t> iv_vec(iv.begin(), iv.end());

    std::vector<uint8_t> cipher;
    std::vector<uint8_t> tag_vec;      // common_mac
    std::array<uint8_t, 16> gcm_tag{};

    if (sender_key.transformation_kind == c_transform_kind_sm4_gmac) {
        std::vector<uint8_t> dummy_ct;
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, {}, dummy_ct, tag_vec)) {
            exception = SecurityException("SM4-GMAC signing failed");
            return false;
        }
        if (tag_vec.size() != 16) {
            exception = SecurityException("SM4-GMAC tag size invalid");
            return false;
        }
        std::memcpy(gcm_tag.data(), tag_vec.data(), 16);

        body.body_flag   = CryptoContent::SIGN;
        body.body_length = static_cast<uint32_t>(pt_concat.size());
        body.secure_data = const_cast<unsigned char*>(pt_concat.data());
        body.header.octets_to_next_header = static_cast<uint16_t>(4 + body.body_length);
    } else if (sender_key.transformation_kind == c_transform_kind_sm4_gcm) {
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, pt_concat, cipher, tag_vec)) {
            exception = SecurityException("SM4-GCM encryption failed");
            return false;
        }
        if (tag_vec.size() != 16) {
            exception = SecurityException("SM4-GCM tag size invalid");
            return false;
        }
        std::memcpy(gcm_tag.data(), tag_vec.data(), 16);

        body.body_flag   = CryptoContent::ENCRYPT;
        body.body_length = static_cast<uint32_t>(cipher.size());
        body.secure_data = cipher.data();
        body.header.octets_to_next_header = static_cast<uint16_t>(4 + body.body_length);
    } else {
        exception = SecurityException("Unsupported transform kind for writer");
        return false;
    }

    SecurePostfixSubMsg postfix{};
    postfix.msg_kind = SEC_POSTFIX;
    postfix.flags    = little_endian ? 0x01 : 0x00;
    std::memcpy(postfix.sec_data_tag.common_mac, gcm_tag.data(), gcm_tag.size());

    const std::vector<uint8_t> rs_aad_vec(gcm_tag.begin(), gcm_tag.end());

    for (int i = 0; i < receiving_datareader_crypto_num; ++i) {
        const DatareaderCryptoHandle& h = receiving_datareader_crypto_list[i];

        SM4GCMGMAC_ReaderCryptoHandle& remote_reader =
            SM4GCMGMAC_ReaderCryptoHandle::narrow(const_cast<DatareaderCryptoHandle&>(h));
        if (remote_reader.nil()) continue;

        if (remote_reader->Entity2RemoteKeyMaterial.empty()) continue;
        const auto& keyMat = remote_reader->Entity2RemoteKeyMaterial.front();

        if (keyMat.receiver_specific_key_id == c_transformKeyIdZero) {
            continue;
        }

        // receiver-specific 会话密钥
        unsigned char rx_session_key[16] = {0};
        if (!get_key_material(rx_session_key, iv.data(), keyMat, /*use_receiver_specific_key=*/true)) {
            continue;
        }

        const std::vector<uint8_t> rs_key_vec(rx_session_key, rx_session_key + 16);
        std::vector<uint8_t> dummy_ct;
        std::vector<uint8_t> rs_tag_vec;

        if (!GMHelper::sm4_gcm_encrypt_vector(rs_key_vec, iv_vec, rs_aad_vec,
                                              /*pt=*/dummy_ct, /*ct_out=*/dummy_ct, rs_tag_vec) ||
            rs_tag_vec.size() != 16)
        {
            exception = SecurityException("SM4-GMAC for receiver-specific MAC failed");
            return false;
        }

        ReceiverSpecificMAC rec{};
        rec.receiver_mac_key_id = keyMat.receiver_specific_key_id;
        std::memcpy(rec.receiver_mac, rs_tag_vec.data(), 16);
        postfix.sec_data_tag.receiver_specific_macs.push_back(rec);
    }

    postfix.octets_to_next_header =
        static_cast<uint16_t>(16 + 4 +
            static_cast<uint32_t>(postfix.sec_data_tag.receiver_specific_macs.size()) * (4 + 16));

    const int wrote = serialize_secure_submessage(
        reinterpret_cast<unsigned char*>(encoded_rtps_submessage.buffer),
        static_cast<unsigned int>(encoded_rtps_submessage.buffer_size),
        prefix, body, postfix, /*little_endian=*/little_endian);

    if (wrote < 0) {
        exception = SecurityException("serialize_secure_submessage failed");
        return false;
    }

    encoded_rtps_submessage.buffer_size = static_cast<unsigned int>(wrote);
    return true;
}

bool SM4GCMGMAC_Transform::decode_datawriter_submessage(
    SerializedBuffer&             plain_rtps_submessage,
    const SerializedBuffer*       encoded_rtps_submessage,
    int                           encoded_rtps_submessage_count,
    DatareaderCryptoHandle&       receiving_datareader_crypto,
    DatawriterCryptoHandle&       sending_datawriter_crypto,
    SecurityException&            exception)
{
    if (!encoded_rtps_submessage || encoded_rtps_submessage_count <= 0) {
        exception = SecurityException("encoded_rtps_submessage is null or count <= 0");
        return false;
    }
    if (!plain_rtps_submessage.buffer || plain_rtps_submessage.buffer_size == 0) {
        exception = SecurityException("plain_rtps_submessage is null or zero-sized");
        return false;
    }

    SM4GCMGMAC_ReaderCryptoHandle& local_reader = SM4GCMGMAC_ReaderCryptoHandle::narrow(receiving_datareader_crypto);
    SM4GCMGMAC_WriterCryptoHandle& remote_writer = SM4GCMGMAC_WriterCryptoHandle::narrow(sending_datawriter_crypto);

    if (local_reader.nil() || remote_writer.nil()) {
        exception = SecurityException("Invalid Datareader/Datawriter crypto handle");
        return false;
    }

    // 反序列化 secure 子消息
    SecurePrefixSubMsg data_header{};
    CryptoContent data_body{};
    SecurePostfixSubMsg data_tag{};
    if (!deserialize_secure_submessage(
            encoded_rtps_submessage, encoded_rtps_submessage_count,
            data_header, data_body, data_tag))
    {
        exception = SecurityException("Secure submessage deserialization failed");
        return false;
    }

    // 读取 transform kind、key id，构造 IV
    const CryptoTransformKind xform_kind =
        data_header.sec_data_header.transform_identifier.transformation_kind;
    const CryptoTransformKeyId sender_key_id =
        data_header.sec_data_header.transform_identifier.transformation_key_id;

    unsigned char iv[12]{};
    get_initialzation_vector(&data_header.sec_data_header, iv); // 4B session_id + 8B 随机/序号

    //  选择发送端密钥材料并派生会话密钥
    const KeyMaterial_SM4_GCM_GMAC* chosen_sender = nullptr;
    for (const auto& km : remote_writer->Entity2RemoteKeyMaterial) {
        if (km.sender_key_id == sender_key_id) { chosen_sender = &km; break; }
    }
    if (!chosen_sender) {
        if (auto parent = remote_writer->Parent_participant.lock()) {
            auto& rp = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*parent);
            if (!rp.nil() && !rp->Participant2ParticipantKeyMaterial.empty()) {
                chosen_sender = &rp->Participant2ParticipantKeyMaterial.front();
            }
        }
    }
    if (!chosen_sender && !remote_writer->Entity2RemoteKeyMaterial.empty()) {
        chosen_sender = &remote_writer->Entity2RemoteKeyMaterial.front();
    }
    if (!chosen_sender) {
        exception = SecurityException("No key material available for sender");
        return false;
    }

    unsigned char session_key_sender[16] = {0};
    if (!get_key_material(session_key_sender, iv, *chosen_sender)) {
        exception = SecurityException("Deriving session key (sender) failed");
        return false;
    }

    // 构造 AAD
    const bool little = (data_header.flags & 0x01u) != 0;
    unsigned char aad_buf[4 + sizeof(CryptoHeader)]{};
    unsigned char* w = aad_buf;

    // id
    *w++ = static_cast<unsigned char>(data_header.msg_kind);

    // flags
    const unsigned char fixed_flags = fix_flags_L(data_header.flags, little);
    *w++ = fixed_flags;

    // u16 octets_to_next_header：按 L 位端序写入
    {
        const uint16_t v = data_header.octets_to_next_header;
        if (little) {
            *w++ = static_cast<unsigned char>(v & 0xFFu);
            *w++ = static_cast<unsigned char>((v >> 8) & 0xFFu);
        } else {
            *w++ = static_cast<unsigned char>((v >> 8) & 0xFFu);
            *w++ = static_cast<unsigned char>(v & 0xFFu);
        }
    }

    // CryptoHeader 拼接
    std::memcpy(w, &data_header.sec_data_header, sizeof(CryptoHeader));
    w += sizeof(CryptoHeader);

    const std::vector<uint8_t> aad_vec(aad_buf, w);

    unsigned char* out_ptr = reinterpret_cast<unsigned char*>(plain_rtps_submessage.buffer);
    const unsigned int out_cap = plain_rtps_submessage.buffer_size;
    unsigned int used = 0;

    if (xform_kind == c_transform_kind_sm4_gcm)
    {
        std::vector<uint8_t> key_vec(session_key_sender, session_key_sender + 16);
        std::vector<uint8_t> iv_vec(iv, iv + 12);
        std::vector<uint8_t> tag_vec(16);
        std::memcpy(tag_vec.data(), data_tag.sec_data_tag.common_mac, 16);

        std::vector<uint8_t> ct_vec;
        if (data_body.secure_data && data_body.body_length > 0) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(data_body.secure_data);
            ct_vec.insert(ct_vec.end(), p, p + data_body.body_length);
        } else if (data_body.data_buffers.src_buffer && data_body.body_length > 0) {
            const SerializedBuffer* src = data_body.data_buffers.src_buffer;
            size_t off  = data_body.data_buffers.cur_buffer_offset;
            size_t need = static_cast<size_t>(data_body.body_length);
            while (need && src) {
                const uint8_t* p = reinterpret_cast<const uint8_t*>(src->buffer);
                const size_t   n = std::min(need, src->buffer_size - off);
                ct_vec.insert(ct_vec.end(), p + off, p + off + n);
                need -= n;
                off = 0;
                ++src;
            }
        }

        std::vector<uint8_t> pt_vec;
        if (!GMHelper::sm4_gcm_decrypt_vector(key_vec, iv_vec, aad_vec, ct_vec, tag_vec, pt_vec)) {
            exception = SecurityException("SM4-GCM decrypt/auth failed (AAD mismatch or tag invalid)");
            return false;
        }

        if (pt_vec.size() > out_cap) {
            exception = SecurityException("plain_rtps_submessage capacity not enough for decrypted payload");
            return false;
        }
        std::memcpy(out_ptr, pt_vec.data(), pt_vec.size());
        used = static_cast<unsigned int>(pt_vec.size());
    }
    else if (xform_kind == c_transform_kind_sm4_gmac)
    {
        std::vector<uint8_t> key_vec(session_key_sender, session_key_sender + 16);
        std::vector<uint8_t> iv_vec(iv, iv + 12);

        std::vector<uint8_t> dummy_ct, dummy_pt, calc_tag;
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, dummy_pt, dummy_ct, calc_tag) ||
            calc_tag.size() != 16)
        {
            exception = SecurityException("SM4-GMAC recompute(common_mac) failed");
            return false;
        }
        if (std::memcmp(calc_tag.data(), data_tag.sec_data_tag.common_mac, 16) != 0) {
            exception = SecurityException("common_mac verification failed");
            return false;
        }

        const KeyMaterial_SM4_GCM_GMAC* rs_km = nullptr;
        if (!local_reader->EntityKeyMaterial.empty()) {
            rs_km = &local_reader->EntityKeyMaterial.front();
        } else if (!local_reader->Entity2RemoteKeyMaterial.empty()) {
            rs_km = &local_reader->Entity2RemoteKeyMaterial.front();
        }
        if (!rs_km || rs_km->receiver_specific_key_id == c_transformKeyIdZero) {
            exception = SecurityException("No receiver-specific key material on local reader");
            return false;
        }

        // 找到 data_tag 里与本地 reader 的 receiver_specific_key_id 匹配的那一项
        const ReceiverSpecificMAC* matched = nullptr;
        for (const auto& rec : data_tag.sec_data_tag.receiver_specific_macs) {
            if (rec.receiver_mac_key_id == rs_km->receiver_specific_key_id) {
                matched = &rec;
                break;
            }
        }
        if (!matched) {
            exception = SecurityException("No matching receiver-specific MAC for this reader");
            return false;
        }

        // 以接收方主密钥派生会话密钥
        unsigned char session_key_receiver[16] = {0};
        if (!get_key_material(session_key_receiver, iv, *rs_km)) {
            exception = SecurityException("Deriving receiver-specific session key failed");
            return false;
        }

        std::vector<uint8_t> rs_key_vec(session_key_receiver, session_key_receiver + 16);
        const std::vector<uint8_t> rs_aad(
            data_tag.sec_data_tag.common_mac,
            data_tag.sec_data_tag.common_mac + 16);

        std::vector<uint8_t> rs_dummy_ct, rs_dummy_pt, rs_tag;
        if (!GMHelper::sm4_gcm_encrypt_vector(rs_key_vec, iv_vec, rs_aad,
                                              rs_dummy_pt, rs_dummy_ct, rs_tag) ||
            rs_tag.size() != 16)
        {
            exception = SecurityException("Recompute receiver-specific GMAC failed");
            return false;
        }
        if (std::memcmp(rs_tag.data(), matched->receiver_mac, 16) != 0) {
            exception = SecurityException("receiver-specific MAC verification failed");
            return false;
        }

        // GMAC 模式下，明文即 data_body.secure_data
        if (data_body.body_length > plain_rtps_submessage.buffer_size) {
            exception = SecurityException("plain_rtps_submessage capacity not enough for plaintext");
            return false;
        }
        if (data_body.secure_data && data_body.body_length) {
            std::memcpy(out_ptr, data_body.secure_data, data_body.body_length);
            used = data_body.body_length;
        } else {
            const SerializedBuffer* src = data_body.data_buffers.src_buffer;
            size_t off  = data_body.data_buffers.cur_buffer_offset;
            size_t need = static_cast<size_t>(data_body.body_length);
            unsigned char* wout = out_ptr;
            while (need && src) {
                const size_t n = std::min(need, src->buffer_size - off);
                std::memcpy(wout, reinterpret_cast<const uint8_t*>(src->buffer) + off, n);
                wout += n;
                need -= n;
                off = 0;
                ++src;
            }
            used = static_cast<unsigned int>(wout - out_ptr);
        }
    }
    else
    {
        exception = SecurityException("Invalid transformation kind (expect SM4_GCM or SM4_GMAC)");
        return false;
    }

    plain_rtps_submessage.buffer_size = used;
    return true;
}

bool SM4GCMGMAC_Transform::encode_datareader_submessage(
    SerializedBuffer& encoded_rtps_submessage,
    const SerializedBuffer* plain_rtps_submessage,
    int plain_rtps_submessage_count,
    DatareaderCryptoHandle& sending_datareader_crypto,
    const DatawriterCryptoHandle* receiving_datawriter_crypto_list,
    int receiving_datawriter_crypto_list_num,
    SecurityException& exception)
{
    if (encoded_rtps_submessage.buffer == nullptr || encoded_rtps_submessage.buffer_size == 0) {
        exception = SecurityException("encoded_rtps_submessage buffer is null or zero size");
        return false;
    }
    if (plain_rtps_submessage == nullptr || plain_rtps_submessage_count <= 0) {
        exception = SecurityException("plain_rtps_submessage is null or empty");
        return false;
    }
    if (receiving_datawriter_crypto_list_num > 0 && receiving_datawriter_crypto_list == nullptr) {
        exception = SecurityException("receiving_datawriter_crypto_list is null but count > 0");
        return false;
    }

    SM4GCMGMAC_ReaderCryptoHandle& reader = SM4GCMGMAC_ReaderCryptoHandle::narrow(sending_datareader_crypto);
    if (reader.nil()) {
        exception = SecurityException("Invalid DatareaderCryptoHandle");
        return false;
    }
    if (reader->EntityKeyMaterial.empty()) {
        exception = SecurityException("Reader EntityKeyMaterial is empty");
        return false;
    }
    const KeyMaterial_SM4_GCM_GMAC& sender_key = reader->EntityKeyMaterial.front();
    if (sender_key.transformation_kind == c_transform_kind_none) {
        exception = SecurityException("Reader key material kind is NONE");
        return false;
    }

    const bool little_endian = host_is_little_endian();

    SecurePrefixSubMsg prefix{};
    prefix.msg_kind = SEC_PREFIX;
    prefix.flags    = little_endian ? 0x01 : 0x00;

    // CryptoHeader
    prefix.sec_data_header.transform_identifier.transformation_kind   = sender_key.transformation_kind;
    prefix.sec_data_header.transform_identifier.transformation_key_id = sender_key.sender_key_id;

    uint32_t sid = 0;
    unsigned char iv_suffix[8];

    if (auto parent_sp = reader->Parent_participant.lock())
    {
        SM4GCMGMAC_ParticipantCryptoHandle& parent = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*parent_sp);
        if (parent.nil())
        {
            exception = SecurityException("Reader->Parent_participant is nil");
            return false;
        }

        std::unique_lock<std::mutex> lock(parent->mutex_);

        if (parent->max_blocks_per_session > 0 &&
            parent->Session.session_block_counter >= parent->max_blocks_per_session)
        {
            parent->Session.session_id += 1;
            compute_sessionkey(parent->Session.SessionKey,
                            parent->ParticipantKeyMaterial,
                            parent->Session.session_id);
            parent->Session.session_block_counter = 0;
        }
        parent->Session.session_block_counter += 1;

        sid = parent->Session.session_id;
    }
    else
    {
        REC_LOG_WARNING(SECURITY_CRYPTO,
            "Reader has no parent participant; fallback to random session_id");
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&sid), 4) != 1)
        {
            exception = SecurityException("Failed to generate fallback random session_id");
            return false;
        }
    }

    // 8B 随机后缀
    if (RAND_bytes(iv_suffix, sizeof(iv_suffix)) != 1)
    {
        exception = SecurityException("Failed to generate random IV suffix");
        return false;
    }

    // 写回 CryptoHeader
    std::memcpy(prefix.sec_data_header.session_id, &sid, 4);
    std::memcpy(prefix.sec_data_header.initialization_vector_suffix, iv_suffix, 8);

    // 组装 12B IV：session_id(4) + iv_suffix(8)
    std::array<uint8_t,12> iv{};
    std::memcpy(iv.data(), &sid, 4);
    std::memcpy(iv.data() + 4, iv_suffix, 8);


    // 会话密钥
    unsigned char session_key[16] = {0};
    (void)get_key_material(session_key, iv.data(), sender_key);

    // 串接明文
    std::vector<uint8_t> pt_concat;
    pt_concat.reserve(256);
    for (int i = 0; i < plain_rtps_submessage_count; ++i) {
        const SerializedBuffer& seg = plain_rtps_submessage[i];
        if (seg.buffer && seg.buffer_size) {
            const auto* p = reinterpret_cast<const uint8_t*>(seg.buffer);
            pt_concat.insert(pt_concat.end(), p, p + seg.buffer_size);
        }
    }

    unsigned char aad_buf[64];
    unsigned char* w = aad_buf;
    const unsigned char* const aad_begin = w;

    *w++ = static_cast<unsigned char>(SEC_PREFIX);
    *w++ = static_cast<unsigned char>(fix_flags_L(prefix.flags, little_endian));
    const size_t len_pos = static_cast<size_t>(w - aad_begin);
    w = write_u16(w, /*placeholder*/0u, little_endian);

    {
        const auto& tk = prefix.sec_data_header.transform_identifier.transformation_kind;
        std::memcpy(w, tk.data(), tk.size());
        w += tk.size();
    }
    std::memcpy(w, prefix.sec_data_header.transform_identifier.transformation_key_id.data(), 4);
    w += 4;

    std::memcpy(w, prefix.sec_data_header.session_id, 4);
    w += 4;

    std::memcpy(w, prefix.sec_data_header.initialization_vector_suffix, 8);
    w += 8;

    const uint16_t cryptoheader_len =
        static_cast<uint16_t>((w - aad_begin) - (len_pos + 2));
    write_u16_at(const_cast<unsigned char*>(aad_begin), len_pos, cryptoheader_len, little_endian);
    prefix.octets_to_next_header = cryptoheader_len;

    const std::vector<uint8_t> aad_vec(reinterpret_cast<const uint8_t*>(aad_begin),
                                       reinterpret_cast<const uint8_t*>(w));

    CryptoContent body{};
    body.header.msg_kind = SEC_BODY;
    body.header.flags    = little_endian ? 0x01 : 0x00;

    const std::vector<uint8_t> key_vec(session_key, session_key + 16);
    const std::vector<uint8_t> iv_vec(iv.begin(), iv.end());

    std::vector<uint8_t> cipher;
    std::vector<uint8_t> tag_vec;  // common_mac
    std::array<uint8_t, 16> gcm_tag{};

    if (sender_key.transformation_kind == c_transform_kind_sm4_gmac) {
        std::vector<uint8_t> dummy_ct;
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, {}, dummy_ct, tag_vec)) {
            exception = SecurityException("SM4-GMAC signing failed");
            return false;
        }
        if (tag_vec.size() != 16) {
            exception = SecurityException("SM4-GMAC tag size invalid");
            return false;
        }
        std::memcpy(gcm_tag.data(), tag_vec.data(), 16);

        body.body_flag   = CryptoContent::SIGN;
        body.body_length = static_cast<uint32_t>(pt_concat.size());
        body.secure_data = const_cast<unsigned char*>(pt_concat.data());
        body.header.octets_to_next_header = static_cast<uint16_t>(4 + body.body_length);
    } else if (sender_key.transformation_kind == c_transform_kind_sm4_gcm) {
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, pt_concat, cipher, tag_vec)) {
            exception = SecurityException("SM4-GCM encryption failed");
            return false;
        }
        if (tag_vec.size() != 16) {
            exception = SecurityException("SM4-GCM tag size invalid");
            return false;
        }
        std::memcpy(gcm_tag.data(), tag_vec.data(), 16);

        body.body_flag   = CryptoContent::ENCRYPT;
        body.body_length = static_cast<uint32_t>(cipher.size());
        body.secure_data = cipher.data();
        body.header.octets_to_next_header = static_cast<uint16_t>(4 + body.body_length);
    } else {
        exception = SecurityException("Unsupported transform kind for reader");
        return false;
    }

    SecurePostfixSubMsg postfix{};
    postfix.msg_kind = SEC_POSTFIX;
    postfix.flags    = little_endian ? 0x01 : 0x00;
    std::memcpy(postfix.sec_data_tag.common_mac, gcm_tag.data(), gcm_tag.size());

    // AAD = common_mac（16B）
    const std::vector<uint8_t> rs_aad_vec(gcm_tag.begin(), gcm_tag.end());

    for (int i = 0; i < receiving_datawriter_crypto_list_num; ++i) {
        const DatawriterCryptoHandle& h = receiving_datawriter_crypto_list[i];
        SM4GCMGMAC_WriterCryptoHandle& remote_writer =
            SM4GCMGMAC_WriterCryptoHandle::narrow(const_cast<DatawriterCryptoHandle&>(h));
        if (remote_writer.nil()) continue;

        const KeyMaterial_SM4_GCM_GMAC* rx_km = nullptr;
        if (auto parent = remote_writer->Parent_participant.lock()) {
            auto& rp = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*parent);
            if (!rp.nil() && !rp->Participant2ParticipantKeyMaterial.empty()) {
                rx_km = &rp->Participant2ParticipantKeyMaterial.front();
            }
        }
        if (!rx_km && !remote_writer->Entity2RemoteKeyMaterial.empty()) {
            rx_km = &remote_writer->Entity2RemoteKeyMaterial.front();
        }
        if (!rx_km) continue;
        if (rx_km->receiver_specific_key_id == c_transformKeyIdZero) continue;

        // 用接收方主密钥派生会话密钥
        unsigned char rx_session_key[16] = {0};
        if (!get_key_material(rx_session_key, iv.data(), *rx_km, /*use_receiver_specific_key=*/true)) {
            continue;
        }

        const std::vector<uint8_t> rs_key_vec(rx_session_key, rx_session_key + 16);
        std::vector<uint8_t> dummy_ct;
        std::vector<uint8_t> rs_tag_vec;

        if (!GMHelper::sm4_gcm_encrypt_vector(rs_key_vec, iv_vec, rs_aad_vec,
                                              /*pt=*/dummy_ct, /*ct=*/dummy_ct, rs_tag_vec) ||
            rs_tag_vec.size() != 16)
        {
            exception = SecurityException("SM4-GMAC for receiver-specific MAC failed");
            return false;
        }

        ReceiverSpecificMAC rec{};
        rec.receiver_mac_key_id = rx_km->receiver_specific_key_id;
        std::memcpy(rec.receiver_mac, rs_tag_vec.data(), 16);
        postfix.sec_data_tag.receiver_specific_macs.push_back(rec);
    }

    postfix.octets_to_next_header =
        static_cast<uint16_t>(16 + 4 +
            static_cast<uint32_t>(postfix.sec_data_tag.receiver_specific_macs.size()) * (4 + 16));

    const int wrote = serialize_secure_submessage(
        reinterpret_cast<unsigned char*>(encoded_rtps_submessage.buffer),
        static_cast<unsigned int>(encoded_rtps_submessage.buffer_size),
        prefix, body, postfix, /*little_endian=*/little_endian);

    if (wrote < 0) {
        exception = SecurityException("serialize_secure_submessage failed");
        return false;
    }
    encoded_rtps_submessage.buffer_size = static_cast<unsigned int>(wrote);
    return true;
}

bool SM4GCMGMAC_Transform::decode_datareader_submessage(
    SerializedBuffer& plain_rtps_submessage,
    const SerializedBuffer* encoded_rtps_submessage,
    int encoded_rtps_submessage_count,
    DatawriterCryptoHandle& receiving_datawriter_crypto,
    DatareaderCryptoHandle& sending_datareader_crypto,
    SecurityException& exception)
{
    if (!encoded_rtps_submessage || encoded_rtps_submessage_count <= 0) {
        exception = SecurityException("encoded_rtps_submessage is null or count <= 0");
        return false;
    }
    if (!plain_rtps_submessage.buffer || plain_rtps_submessage.buffer_size == 0) {
        exception = SecurityException("plain_rtps_submessage is null or zero-sized");
        return false;
    }

    // 本地 writer、远端 reader
    SM4GCMGMAC_WriterCryptoHandle&  local_writer  =
        SM4GCMGMAC_WriterCryptoHandle::narrow(receiving_datawriter_crypto);
    SM4GCMGMAC_ReaderCryptoHandle&  remote_reader =
        SM4GCMGMAC_ReaderCryptoHandle::narrow(sending_datareader_crypto);

    if (local_writer.nil() || remote_reader.nil()) {
        exception = SecurityException("Invalid Datawriter/Datareader crypto handle");
        return false;
    }

    // 反序列化 Secure 子消息
    SecurePrefixSubMsg data_header{};
    CryptoContent data_body{};
    SecurePostfixSubMsg data_tag{};
    if (!deserialize_secure_submessage(
            encoded_rtps_submessage, encoded_rtps_submessage_count,
            data_header, data_body, data_tag))
    {
        exception = SecurityException("Secure submessage deserialization failed");
        return false;
    }

    // 读取变换类型、key id，构造 IV
    const CryptoTransformKind xform_kind =
        data_header.sec_data_header.transform_identifier.transformation_kind;
    const CryptoTransformKeyId sender_key_id =
        data_header.sec_data_header.transform_identifier.transformation_key_id;

    unsigned char iv[12]{};
    get_initialzation_vector(&data_header.sec_data_header, iv); // 4B session_id + 8B 后缀

    // 选择远端密钥材料并派生会话密钥
    const KeyMaterial_SM4_GCM_GMAC* chosen_sender = nullptr;
    for (const auto& km : remote_reader->Entity2RemoteKeyMaterial) {
        if (km.sender_key_id == sender_key_id) { chosen_sender = &km; break; }
    }
    if (!chosen_sender) {
        if (auto parent = remote_reader->Parent_participant.lock()) {
            auto& rp = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*parent);
            if (!rp.nil() && !rp->Participant2ParticipantKeyMaterial.empty()) {
                chosen_sender = &rp->Participant2ParticipantKeyMaterial.front();
            }
        }
    }
    if (!chosen_sender && !remote_reader->Entity2RemoteKeyMaterial.empty()) {
        chosen_sender = &remote_reader->Entity2RemoteKeyMaterial.front();
    }
    if (!chosen_sender) {
        exception = SecurityException("No key material available for sender (remote reader)");
        return false;
    }

    unsigned char session_key_sender[16] = {0};
    if (!get_key_material(session_key_sender, iv, *chosen_sender)) {
        exception = SecurityException("Deriving session key (sender) failed");
        return false;
    }

    // 构造 AAD
    const bool little = (data_header.flags & 0x01u) != 0;
    unsigned char aad_buf[4 + sizeof(CryptoHeader)]{};
    unsigned char* w = aad_buf;

    // id
    *w++ = static_cast<unsigned char>(data_header.msg_kind);

    // flags（修正 L 位，使与序列化对齐）
    const unsigned char fixed_flags = fix_flags_L(data_header.flags, little);
    *w++ = fixed_flags;

    // u16(octets_to_next_header) 按 L 位端序写入
    {
        const uint16_t v = data_header.octets_to_next_header;
        if (little) {
            *w++ = static_cast<unsigned char>(v & 0xFFu);
            *w++ = static_cast<unsigned char>((v >> 8) & 0xFFu);
        } else {
            *w++ = static_cast<unsigned char>((v >> 8) & 0xFFu);
            *w++ = static_cast<unsigned char>(v & 0xFFu);
        }
    }

    // CryptoHeader 原样拼接
    std::memcpy(w, &data_header.sec_data_header, sizeof(CryptoHeader));
    w += sizeof(CryptoHeader);

    const std::vector<uint8_t> aad_vec(aad_buf, w);

    unsigned char* out_ptr = reinterpret_cast<unsigned char*>(plain_rtps_submessage.buffer);
    const unsigned int out_cap = plain_rtps_submessage.buffer_size;
    unsigned int used = 0;

    if (xform_kind == c_transform_kind_sm4_gcm)
    {
        // 解密完整负载（tag = common_mac）
        std::vector<uint8_t> key_vec(session_key_sender, session_key_sender + 16);
        std::vector<uint8_t> iv_vec(iv, iv + 12);
        std::vector<uint8_t> tag_vec(16);
        std::memcpy(tag_vec.data(), data_tag.sec_data_tag.common_mac, 16);

        // 收集密文
        std::vector<uint8_t> ct_vec;
        if (data_body.secure_data && data_body.body_length > 0) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(data_body.secure_data);
            ct_vec.insert(ct_vec.end(), p, p + data_body.body_length);
        } else if (data_body.data_buffers.src_buffer && data_body.body_length > 0) {
            const SerializedBuffer* src = data_body.data_buffers.src_buffer;
            size_t off  = data_body.data_buffers.cur_buffer_offset;
            size_t need = static_cast<size_t>(data_body.body_length);
            while (need && src) {
                const uint8_t* p = reinterpret_cast<const uint8_t*>(src->buffer);
                const size_t   n = std::min(need, src->buffer_size - off);
                ct_vec.insert(ct_vec.end(), p + off, p + off + n);
                need -= n;
                off = 0;
                ++src;
            }
        }

        std::vector<uint8_t> pt_vec;
        if (!GMHelper::sm4_gcm_decrypt_vector(key_vec, iv_vec, aad_vec, ct_vec, tag_vec, pt_vec)) {
            exception = SecurityException("SM4-GCM decrypt/auth failed (AAD mismatch or tag invalid)");
            return false;
        }
        if (pt_vec.size() != static_cast<size_t>(data_body.body_length)) {
            exception = SecurityException("Decrypted length mismatch with body_length");
            return false;
        }
        if (pt_vec.size() > out_cap) {
            exception = SecurityException("plain_rtps_submessage capacity not enough for decrypted payload");
            return false;
        }

        std::memcpy(out_ptr, pt_vec.data(), pt_vec.size());
        used = static_cast<unsigned int>(pt_vec.size());
    }
    else if (xform_kind == c_transform_kind_sm4_gmac)
    {
        std::vector<uint8_t> key_vec(session_key_sender, session_key_sender + 16);
        std::vector<uint8_t> iv_vec(iv, iv + 12);

        std::vector<uint8_t> dummy_ct, dummy_pt, calc_tag;
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, dummy_pt, dummy_ct, calc_tag) ||
            calc_tag.size() != 16)
        {
            exception = SecurityException("SM4-GMAC recompute(common_mac) failed");
            return false;
        }
        if (std::memcmp(calc_tag.data(), data_tag.sec_data_tag.common_mac, 16) != 0) {
            exception = SecurityException("common_mac verification failed");
            return false;
        }

        // 校验 data_tag 中与本地 writer 的 receiver_specific_key_id 匹配的 receiver_mac
        const KeyMaterial_SM4_GCM_GMAC* rs_km = nullptr;
        if (!local_writer->EntityKeyMaterial.empty()) {
            rs_km = &local_writer->EntityKeyMaterial.front();
        } else if (!local_writer->Entity2RemoteKeyMaterial.empty()) {
            rs_km = &local_writer->Entity2RemoteKeyMaterial.front();
        }
        if (!rs_km || rs_km->receiver_specific_key_id == c_transformKeyIdZero) {
            exception = SecurityException("No receiver-specific key material on local writer");
            return false;
        }

        const ReceiverSpecificMAC* matched = nullptr;
        for (const auto& rec : data_tag.sec_data_tag.receiver_specific_macs) {
            if (rec.receiver_mac_key_id == rs_km->receiver_specific_key_id) {
                matched = &rec; break;
            }
        }
        if (!matched) {
            exception = SecurityException("No matching receiver-specific MAC for this writer");
            return false;
        }

        unsigned char session_key_receiver[16] = {0};
        if (!get_key_material(session_key_receiver, iv, *rs_km)) {
            exception = SecurityException("Deriving receiver-specific session key failed");
            return false;
        }

        std::vector<uint8_t> rs_key_vec(session_key_receiver, session_key_receiver + 16);
        const std::vector<uint8_t> rs_aad(
            data_tag.sec_data_tag.common_mac,
            data_tag.sec_data_tag.common_mac + 16);

        std::vector<uint8_t> rs_dummy_ct, rs_dummy_pt, rs_tag;
        if (!GMHelper::sm4_gcm_encrypt_vector(rs_key_vec, iv_vec, rs_aad,
                                              rs_dummy_pt, rs_dummy_ct, rs_tag) ||
            rs_tag.size() != 16)
        {
            exception = SecurityException("Recompute receiver-specific GMAC failed");
            return false;
        }
        if (std::memcmp(rs_tag.data(), matched->receiver_mac, 16) != 0) {
            exception = SecurityException("receiver-specific MAC verification failed");
            return false;
        }

        if (data_body.body_length > plain_rtps_submessage.buffer_size) {
            exception = SecurityException("plain_rtps_submessage capacity not enough for plaintext");
            return false;
        }
        if (data_body.secure_data && data_body.body_length) {
            std::memcpy(out_ptr, data_body.secure_data, data_body.body_length);
            used = data_body.body_length;
        } else if (data_body.data_buffers.src_buffer && data_body.body_length > 0) {
            const SerializedBuffer* src = data_body.data_buffers.src_buffer;
            size_t off  = data_body.data_buffers.cur_buffer_offset;
            size_t need = static_cast<size_t>(data_body.body_length);
            unsigned char* wout = out_ptr;
            while (need && src) {
                const size_t n = std::min(need, src->buffer_size - off);
                std::memcpy(wout, reinterpret_cast<const uint8_t*>(src->buffer) + off, n);
                wout += n;
                need -= n;
                off = 0;
                ++src;
            }
            used = static_cast<unsigned int>(wout - out_ptr);
        } else {
            used = 0;
        }
    }
    else
    {
        exception = SecurityException("Invalid transformation kind (expect SM4_GCM or SM4_GMAC)");
        return false;
    }

    plain_rtps_submessage.buffer_size = used;
    return true;
}

bool SM4GCMGMAC_Transform::encode_serialized_payload(
        SerializedBuffer& encoded_buffer,
        SerializedBuffer& extra_inline_qos,
        const SerializedBuffer* plain_buffer,
        int plain_buffer_count,
        DatawriterCryptoHandle& sending_datawriter_crypto,
        SecurityException& exception)
{
    if (!encoded_buffer.buffer || encoded_buffer.buffer_size == 0) {
        exception = SecurityException("encoded_buffer is null or zero-sized");
        return false;
    }
    if (!plain_buffer || plain_buffer_count <= 0) {
        exception = SecurityException("plain_buffer is null or count <= 0");
        return false;
    }

    SM4GCMGMAC_WriterCryptoHandle& writer =
        SM4GCMGMAC_WriterCryptoHandle::narrow(sending_datawriter_crypto);
    if (writer.nil()) {
        exception = SecurityException("Invalid DatawriterCryptoHandle");
        return false;
    }
    if (writer->EntityKeyMaterial.empty()) {
        exception = SecurityException("Writer EntityKeyMaterial is empty");
        return false;
    }
    const KeyMaterial_SM4_GCM_GMAC& sender_key = writer->EntityKeyMaterial.front();
    if (sender_key.transformation_kind == c_transform_kind_none) {
        exception = SecurityException("Transformation kind NONE is not allowed for payload");
        return false;
    }

    const bool little_endian = host_is_little_endian();

    uint32_t session_id = 0;
    {
        auto parent_sp = writer->Parent_participant.lock();
        if (parent_sp) {
            SM4GCMGMAC_ParticipantCryptoHandle& parent =
                SM4GCMGMAC_ParticipantCryptoHandle::narrow(*parent_sp);
            if (parent.nil()) {
                exception = SecurityException("Parent participant handle is nil");
                return false;
            }
            std::unique_lock<std::mutex> lk(parent->mutex_);
            if (parent->max_blocks_per_session > 0 &&
                parent->Session.session_block_counter >= parent->max_blocks_per_session)
            {
                parent->Session.session_id += 1;
                compute_sessionkey(parent->Session.SessionKey,
                                   parent->ParticipantKeyMaterial,
                                   parent->Session.session_id);
                parent->Session.session_block_counter = 0;
            }
            parent->Session.session_block_counter += 1;
            session_id = parent->Session.session_id;
        } else {
            // 无父参与者：回退到随机 session_id
            if (RAND_bytes(reinterpret_cast<unsigned char*>(&session_id), 4) != 1) {
                exception = SecurityException("RAND_bytes for session_id failed");
                return false;
            }
        }
    }

    unsigned char iv_suffix[8];
    if (RAND_bytes(iv_suffix, sizeof(iv_suffix)) != 1) {
        exception = SecurityException("RAND_bytes for iv_suffix failed");
        return false;
    }
    std::array<uint8_t,12> iv{};
    std::memcpy(iv.data(), &session_id, 4);
    std::memcpy(iv.data() + 4, iv_suffix, 8);

    // 组装 CryptoHeader
    CryptoHeader chead{};
    chead.transform_identifier.transformation_kind   = sender_key.transformation_kind;
    chead.transform_identifier.transformation_key_id = sender_key.sender_key_id;
    std::memcpy(chead.session_id, &session_id, 4);
    std::memcpy(chead.initialization_vector_suffix, iv_suffix, 8);

    const std::vector<uint8_t> aad_vec(
        reinterpret_cast<const uint8_t*>(&chead),
        reinterpret_cast<const uint8_t*>(&chead) + sizeof(CryptoHeader));

    // 会话密钥（基于 sender 主密钥 + IV 派生
    unsigned char session_key[16] = {0};
    if (!get_key_material(session_key, iv.data(), sender_key)) {
        exception = SecurityException("get_key_material failed");
        return false;
    }
    const std::vector<uint8_t> key_vec(session_key, session_key + 16);
    const std::vector<uint8_t> iv_vec(iv.begin(), iv.end());

    // 串接明文
    std::vector<uint8_t> pt_concat;
    {
        size_t reserve_n = 0;
        for (int i = 0; i < plain_buffer_count; ++i) {
            reserve_n += static_cast<size_t>(plain_buffer[i].buffer_size);
        }
        pt_concat.reserve(reserve_n);
        for (int i = 0; i < plain_buffer_count; ++i) {
            const SerializedBuffer& seg = plain_buffer[i];
            if (seg.buffer && seg.buffer_size) {
                const auto* p = reinterpret_cast<const uint8_t*>(seg.buffer);
                pt_concat.insert(pt_concat.end(), p, p + seg.buffer_size);
            }
        }
    }

    std::vector<uint8_t> cipher;
    std::vector<uint8_t> common_mac;
    if (sender_key.transformation_kind == c_transform_kind_sm4_gmac) {
        // GMAC：仅计算 tag（明文不参与加密）
        std::vector<uint8_t> ct_dummy;
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, {}, ct_dummy, common_mac)) {
            exception = SecurityException("SM4-GMAC signing failed");
            return false;
        }
    } else if (sender_key.transformation_kind == c_transform_kind_sm4_gcm) {
        // GCM：加密 payload，得到密文和 tag
        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, pt_concat, cipher, common_mac)) {
            exception = SecurityException("SM4-GCM encryption failed");
            return false;
        }
    } else {
        exception = SecurityException("Unsupported transform kind");
        return false;
    }
    if (common_mac.size() != 16) {
        exception = SecurityException("common_mac size != 16");
        return false;
    }

    unsigned char* out = reinterpret_cast<unsigned char*>(encoded_buffer.buffer);
    const unsigned int cap = static_cast<unsigned int>(encoded_buffer.buffer_size);
    unsigned int used = 0;

    // CryptoHeader
    int hdr_written = serialize_SecureDataHeader(out + used, cap - used, chead, exception);

    if (hdr_written < 0) {
        return false;
    }
    used += static_cast<unsigned int>(hdr_written);

    // SEC_BODY 头
    {
        const uint8_t* body_ptr =
            (sender_key.transformation_kind == c_transform_kind_sm4_gcm)
                ? (cipher.empty() ? nullptr : cipher.data())
                : (pt_concat.empty() ? nullptr : pt_concat.data());
        const uint32_t body_len =
            (sender_key.transformation_kind == c_transform_kind_sm4_gcm)
                ? static_cast<uint32_t>(cipher.size())
                : static_cast<uint32_t>(pt_concat.size());

        int body_written = serialize_SecureDataBody(
            out + used,
            cap - used,
            little_endian,
            body_ptr,
            body_len,
            exception);

        if (body_written < 0) {
            return false;
        }
        used += static_cast<unsigned int>(body_written);
    }

    {
        std::vector<const ParticipantCryptoHandle*> receivers; // 目前无接收者就传空
        const unsigned int remain = cap - used;

        if (remain < 20u) {
            exception = SecurityException("No space for SecureDataTag");
            return false;
        }

        int tag_written = serialize_SecureDataTag(
            out + used,
            remain,
            common_mac.data(),
            iv.data(),
            receivers,
            little_endian,
            exception);

        if (tag_written < 0) {
            return false;
        }
        used += static_cast<unsigned int>(tag_written);
    }

    // 写回长度
    encoded_buffer.buffer_size = used;

    extra_inline_qos.buffer_size = 0;

    return true;
}

int SM4GCMGMAC_Transform::serialize_secure_rtps_message(
    unsigned char* output_buf,
    unsigned int output_buf_length,
    const SecureRTPSPrefixSubMsg& data_header,
    const CryptoContent& data_body,
    const SecureRTPSPostfixSubMsg& data_tag,
    const unsigned char* tag_bytes, 
    unsigned int tag_len,
    bool little_endian)
{
    if (output_buf == nullptr || output_buf_length == 0) {
        return -1;
    }
    if ((tag_bytes == nullptr && tag_len != 0) || (tag_bytes != nullptr && tag_len == 0)) {
        return -1;
    }
    unsigned char* w = output_buf;
    unsigned int   used = 0;

    // rtps_header (20B)
    {
        constexpr unsigned int RTPS_HEADER_LEN = sizeof(data_header.rtps_header);
        if (used + RTPS_HEADER_LEN > output_buf_length) return -1;
        std::memcpy(w, data_header.rtps_header, RTPS_HEADER_LEN);
        w    += RTPS_HEADER_LEN;
        used += RTPS_HEADER_LEN;
    }

    // msg_kind (1B)
    {
        if (used + 1 > output_buf_length) return -1;
        *w++ = static_cast<unsigned char>(data_header.msg_kind);
        used += 1;
    }

    // flags (1B)
    {
        if (used + 1 > output_buf_length) return -1;
        *w++ = static_cast<unsigned char>(data_header.flags);
        used += 1;
    }

    {
        if (used + 2 > output_buf_length) return -1;
        w = write_u16(w, data_header.octets_to_next_header, little_endian);
        used += 2;
    }

    {
        constexpr unsigned int HDR_LEN = sizeof(data_header.sec_data_header);
        if (used + HDR_LEN > output_buf_length) return -1;

        std::memcpy(w, &data_header.sec_data_header, HDR_LEN);
        w    += HDR_LEN;
        used += HDR_LEN;
    }

    {
        if (data_body.body_flag == CryptoContent::ENCRYPT)
        {
            // 计算 CryptoContent 头部和载荷所需的总空间
            constexpr unsigned int CRYPTO_HEADER_SIZE = 1u + 1u + 2u + 4u;
            const unsigned int payload_size = data_body.body_length;
            const unsigned int total_content_size = CRYPTO_HEADER_SIZE + payload_size;

            if (used + total_content_size > output_buf_length) return -1;
            if (payload_size > 0 && data_body.secure_data == nullptr) return -1;

            // 序列化 CryptoContent 头部
            *w++ = static_cast<unsigned char>(data_body.header.msg_kind);

            unsigned char fixed_flags = static_cast<unsigned char>(data_body.header.flags & ~0x01u);
            if (little_endian) fixed_flags |= 0x01u;
            *w++ = fixed_flags;

            w = write_u16(w, data_body.header.octets_to_next_header, little_endian);
            w = write_u32(w, static_cast<uint32_t>(payload_size), /*little_endian=*/false);

            // 序列化 CryptoContent 载荷 (Payload)
            if (payload_size > 0)
            {
                std::memcpy(w, data_body.secure_data, payload_size);
                w += payload_size;
            }

            used += total_content_size;
        }
        else if (data_body.body_flag == CryptoContent::SIGN)
        {
            const unsigned int payload_size = data_body.body_length;
            if (used + payload_size > output_buf_length) return -1;
            if (payload_size > 0 && data_body.secure_data == nullptr) return -1; // 确保数据指针有效

            if (payload_size > 0)
            {
                std::memcpy(w, data_body.secure_data, payload_size);
                w += payload_size;
            }
            used += payload_size;
        }
        else
        {
            // 未知或无效的 body_flag
            return -1;
        }
    }

    {
        if (used + 2 > output_buf_length) return -1;
        *w++ = static_cast<unsigned char>(data_tag.msg_kind);
        *w++ = static_cast<unsigned char>(data_tag.flags);
        used += 2;
    }

    {
        if (used + 2 > output_buf_length) return -1;
        w = write_u16(w, data_tag.octets_to_next_header, little_endian);
        used += 2;
    }
    {
        if (tag_len > 0)
        {
            if (used + tag_len > output_buf_length) return -1;
            std::memcpy(w, tag_bytes, tag_len);
            w    += tag_len;
            used += tag_len;
        }
    }
    // 确认容量
    if (used > output_buf_length) {
        return -1;
    }
    return static_cast<int>(used);
}

int SM4GCMGMAC_Transform::serialize_crypto_content(
    unsigned char* output_buf,
    unsigned int output_buf_length,
    const CryptoContent* data_body,
    bool little_endian)
{
    if (output_buf == nullptr || data_body == nullptr) {
        return -1;
    }

    unsigned char* w = output_buf;
    unsigned int used = 0;

    if (data_body->body_flag == CryptoContent::ENCRYPT)
    {
        const unsigned int need = 1u + 1u + 2u + 4u + static_cast<unsigned int>(data_body->body_length);
        if (need > output_buf_length) {
            return -1;
        }

        // msg_kind
        *w++ = static_cast<unsigned char>(data_body->header.msg_kind);
        used += 1;

        // flags（根据目标端序修正 L 位）
        unsigned char fixed_flags = static_cast<unsigned char>(data_body->header.flags & ~0x01u);
        if (little_endian) fixed_flags = static_cast<unsigned char>(fixed_flags | 0x01u);
        *w++ = fixed_flags;
        used += 1;

        // octets_to_next_header（按照目标端序写入）
        if (used + 2 > output_buf_length) return -1;
        w = write_data_endian(
                w, reinterpret_cast<const unsigned char*>(&data_body->header.octets_to_next_header),
                1, sizeof(uint16_t), little_endian);
        used += 2;

        // body_length（协议规定固定大端序写入）
        if (used + 4 > output_buf_length) return -1;
        {
            const uint32_t be_len = static_cast<uint32_t>(data_body->body_length);
            w = write_data_endian(
                    w, reinterpret_cast<const unsigned char*>(&be_len),
                    1, sizeof(uint32_t), /*little_endian=*/false); // 固定大端
        }
        used += 4;

        // 密文字节在加密阶段已写入缓存，跳过对应长度
        if (used + data_body->body_length > output_buf_length) return -1;
        w    += data_body->body_length;
        used += data_body->body_length;

        return static_cast<int>(used);
    }
    else if (data_body->body_flag == CryptoContent::SIGN)
    {
        return static_cast<int>(data_body->body_length);
    }
    else
    {
        return -1;
    }
}

int SM4GCMGMAC_Transform::serialize_secure_submessage(
    unsigned char* output_buf,
    unsigned int output_buf_length,
    const SecurePrefixSubMsg& prefix,
    const CryptoContent& body,
    const SecurePostfixSubMsg& postfix,
    bool little_endian)
{
    if (!output_buf || output_buf_length == 0) return -1;

    unsigned char* w = output_buf;
    unsigned int   used = 0;

    auto need = [&](unsigned int n)->bool {
        return (used + n) <= output_buf_length;
    };

    // SEC_PREFIX
    {
        if (!need(1)) return -1;
        *w++ = static_cast<unsigned char>(prefix.msg_kind);
        used += 1;
        if (!need(1)) return -1;
        *w++ = fix_flags_L(prefix.flags, little_endian);
        used += 1;
        if (!need(2)) return -1;
        w = write_u16(w, prefix.octets_to_next_header, little_endian); used += 2;

        if (!need(sizeof(CryptoHeader))) return -1;
        std::memcpy(w, &prefix.sec_data_header, sizeof(CryptoHeader));
        w += sizeof(CryptoHeader); used += sizeof(CryptoHeader);
    }

    // SEC_BODY
    {
        if (!need(1)) return -1;
        *w++ = static_cast<unsigned char>(body.header.msg_kind);
        used += 1;
        if (!need(1)) return -1;
        *w++ = fix_flags_L(body.header.flags, little_endian);
        used += 1;

        if (!need(2)) return -1;
        w = write_u16(w, body.header.octets_to_next_header, little_endian);
        used += 2;

        if (!need(4)) return -1;
        w = write_u32(w, static_cast<uint32_t>(body.body_length), false);
        used += 4;

        // 写入 body
        if (body.body_length > 0) {
            if (!need(body.body_length)) return -1;
            std::memcpy(w, body.secure_data, body.body_length);
            w += body.body_length; used += body.body_length;
        }
    }

    // SEC_POSTFIX
    {
        if (!need(1)) return -1;
        *w++ = static_cast<unsigned char>(postfix.msg_kind);
        used += 1;

        if (!need(1)) return -1;
        *w++ = fix_flags_L(postfix.flags, little_endian);
        used += 1;

        if (!need(2)) return -1;
        w = write_u16(w, postfix.octets_to_next_header, little_endian);
        used += 2;

        // common_mac
        if (!need(16)) return -1;
        std::memcpy(w, postfix.sec_data_tag.common_mac, 16);
        w += 16;
        used += 16;

        // receiver_specific_macs: 每条 4B key_id + 16B mac
        for (const auto& rec : postfix.sec_data_tag.receiver_specific_macs) {
            // 写入 key_id
            if (!need(4)) return -1;
            uint32_t key_id_u32 = 0;
            static_assert(sizeof(rec.receiver_mac_key_id) == sizeof(uint32_t), "receiver_mac_key_id must be 4 bytes");
            std::memcpy(&key_id_u32, &rec.receiver_mac_key_id, 4);
            w = write_u32(w, key_id_u32, little_endian);
            used += 4;

            // 写入 mac
            if (!need(16)) return -1;
            std::memcpy(w, rec.receiver_mac, 16);
            w += 16;
            used += 16;
        }
    }

    return static_cast<int>(used);
}

int SM4GCMGMAC_Transform::serialize_SecureDataHeader(
    unsigned char* out,
    unsigned int out_cap,
    const CryptoHeader& hdr,
    SecurityException& exception)
{
    if (!out || out_cap < sizeof(CryptoHeader)) {
        exception = SecurityException("serialize_SecureDataHeader: insufficient buffer");
        return -1;
    }
    std::memcpy(out, &hdr, sizeof(CryptoHeader));
    return static_cast<int>(sizeof(CryptoHeader));
}

int SM4GCMGMAC_Transform::serialize_SecureDataTag(
    unsigned char* output_buf,
    unsigned int output_buf_length,
    const unsigned char common_mac[16],
    const unsigned char iv[12],
    const std::vector<const ParticipantCryptoHandle*>& receiving_crypto_list,
    bool little_endian,
    SecurityException& exception)
{
    if (output_buf == nullptr || common_mac == nullptr || iv == nullptr) {
        exception = SecurityException("Invalid arguments for serialize_SecureDataTag");
        return -1;
    }

    unsigned char* w = output_buf;
    unsigned int remaining_len = output_buf_length;

    // 序列化 common_mac
    if (remaining_len < 16) {
        return -1;
    }
    std::memcpy(w, common_mac, 16);
    w += 16;
    remaining_len -= 16;

    if (remaining_len < 4) {
        return -1;
    }
    unsigned char* length_ptr = w;
    w += 4;
    remaining_len -= 4;
    uint32_t receiver_count = 0;

    const std::vector<uint8_t> empty_pt;

    const std::vector<uint8_t> aad_vec(common_mac, common_mac + 16);

    for (const ParticipantCryptoHandle* rec_handle : receiving_crypto_list)
    {
        if(rec_handle == nullptr) {
            continue;
        }

        const SM4GCMGMAC_ParticipantCryptoHandle& remote_participant =
            SM4GCMGMAC_ParticipantCryptoHandle::narrow(*rec_handle);

        if (remote_participant.nil()) {
            continue;
        }

        if (remote_participant->Participant2ParticipantKeyMaterial.empty()) {
            continue;
        }

        const auto& keyMat = remote_participant->Participant2ParticipantKeyMaterial.front();
        if (keyMat.receiver_specific_key_id == c_transformKeyIdZero) {
            break;
        }

        // 检查缓冲区是否还有足够空间写入 receiver-specific MAC
        if (remaining_len < (4 + 16)) {
             REC_LOG_WARNING(SECURITY_CRYPTO, "Output buffer too small for all receiver specific MACs.");
             break;
        }

        // 派生 receiver-specific 会话密钥
        unsigned char session_key[16];
        if (!get_key_material(session_key, iv, keyMat, /*use_receiver_specific_key=*/true)) {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Failed to derive receiver-specific session key.");
            continue;
        }

        // 使用派生出的会话密钥和 common_mac 作为 AAD 来计算 receiver-specific MAC
        const std::vector<uint8_t> key_vec(session_key, session_key + 16);
        const std::vector<uint8_t> iv_vec(iv, iv + 12);
        std::vector<uint8_t> dummy_ct;
        std::vector<uint8_t> receiver_mac_vec;

        if (!GMHelper::sm4_gcm_encrypt_vector(key_vec, iv_vec, aad_vec, empty_pt, dummy_ct, receiver_mac_vec) ||
            receiver_mac_vec.size() != 16) {
            exception = SecurityException("SM4-GMAC for receiver-specific MAC failed");
            return -1;
        }

        // 序列化 receiver_mac_key_id
        std::memcpy(w, keyMat.receiver_specific_key_id.data(), 4);
        w += 4;
        remaining_len -= 4;

        // 序列化 receiver_mac
        std::memcpy(w, receiver_mac_vec.data(), 16);
        w += 16;
        remaining_len -= 16;

        receiver_count++;
    }

    write_u32(length_ptr, receiver_count, /*little_endian=*/little_endian);

    return static_cast<int>(w - output_buf);
}

int SM4GCMGMAC_Transform::serialize_SecureDataBody(
    unsigned char* out,
    unsigned int out_cap,
    bool little_endian,
    const uint8_t* body_data,
    uint32_t body_len,
    SecurityException& exception)
{
    if (!out || out_cap == 0) {
        exception = SecurityException("serialize_SecureDataBody: invalid output buffer");
        return -1;
    }

    unsigned int used = 0;
    auto need = [&](unsigned int n)->bool { return (used + n) <= out_cap; };

    // msg_kind
    if (!need(1)) { exception = SecurityException("No space for msg_kind"); return -1; }
    out[used++] = static_cast<unsigned char>(SEC_BODY);

    // flags
    if (!need(1)) { exception = SecurityException("No space for flags"); return -1; }
    out[used++] = static_cast<unsigned char>(little_endian ? 0x01 : 0x00);

    // octets_to_next_header
    const uint16_t next_hdr = static_cast<uint16_t>(4u + body_len);
    if (!need(2)) { exception = SecurityException("No space for octets_to_next_header"); return -1; }
    write_u16(out + used, next_hdr, little_endian);
    used += 2;

    // body_length
    if (!need(4)) { exception = SecurityException("No space for body_length"); return -1; }
    write_u32(out + used, body_len, /*little_endian=*/false);
    used += 4;

    // 数据体
    if (body_len > 0) {
        if (!body_data) {
            exception = SecurityException("serialize_SecureDataBody: body_data is null while body_len > 0");
            return -1;
        }
        if (!need(body_len)) { exception = SecurityException("No space for body data"); return -1; }
        std::memcpy(out + used, body_data, body_len);
        used += body_len;
    }

    return static_cast<int>(used);
}

void SM4GCMGMAC_Transform::compute_sessionkey(
        std::array<uint8_t, 16>& session_key,
        const KeyMaterial_SM4_GCM_GMAC& key_mat,
        const uint32_t session_id)
{
    session_key.fill(0);

    static constexpr char kLabel[] = "SessionKey";   // 10 bytes
    std::vector<uint8_t> source;
    source.reserve(sizeof(kLabel) - 1 + key_mat.master_salt.size() + sizeof(session_id));

    source.insert(source.end(),
                  reinterpret_cast<const uint8_t*>(kLabel),
                  reinterpret_cast<const uint8_t*>(kLabel) + (sizeof(kLabel) - 1));

    source.insert(source.end(), key_mat.master_salt.begin(), key_mat.master_salt.end());

    const uint8_t* sid_ptr = reinterpret_cast<const uint8_t*>(&session_id);
    source.insert(source.end(), sid_ptr, sid_ptr + sizeof(uint32_t));

    const std::vector<uint8_t> key(key_mat.master_sender_key.begin(),
                                   key_mat.master_sender_key.end());

    std::vector<uint8_t> mac_out;
    if (GMHelper::hmac_sm3(key, source, mac_out) && mac_out.size() >= 16)
    {
        std::memcpy(session_key.data(), mac_out.data(), 16);
    }
    else
    {
        const size_t take = std::min<size_t>(16, key.size());
        if (take > 0)
        {
            std::memcpy(session_key.data(), key.data(), take);
        }
    }

    std::fill(source.begin(), source.end(), 0);
    std::fill(mac_out.begin(), mac_out.end(), 0);
}

