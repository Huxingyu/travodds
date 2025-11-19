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
#ifndef _SECURITY_AUTHENTICATION_SM4GCMGMAC_TYPES_H_
#define _SECURITY_AUTHENTICATION_SM4GCMGMAC_TYPES_H_

#include <array>
#include <cstdint>
#include <atomic>
#include <optional>
#include "cdr/travoddscdr.h"
#include "cryptography/CryptoTypes.h"
#include "accesscontrol/ParticipantSecurityAttributes.h"

TRAVODDS_NAMESPACE_BEGIN

#define ACKNACK 0x06
#define NACK_FRAG 0x12
#define DATA 0x15 
#define DATA_FRAG 0x16
#define GAP 0x08 
#define HEARTBEAT 0x07
#define HEARTBEAT_FRAG 0x13

#define INFO_TS 0x09
#define INFO_SRC 0x0c
#define INFO_REPLY_IP4 0x0d
#define INFO_DST 0x0e
#define INFO_REPLY 0x0f

#define SEC_BODY 0x30
#define SEC_PREFIX 0x31
#define SEC_POSTFIX 0x32
#define SRTPS_PREFIX 0x33
#define SRTPS_POSTFIX 0x34

using CryptoTransformKind = std::array<uint8_t, 4>;

// 无加密，无认证标签
#define CRYPTO_TRANSFORMATION_KIND_NONE     (CryptoTransformKind{0, 0, 0, 0})

// 无加密, SM4-GMAC 认证 (128位)
#define CRYPTO_TRANSFORMATION_KIND_SM4_GMAC (CryptoTransformKind{0, 0, 0, 10})

// SM4-GCM 认证加密 (128位)
#define CRYPTO_TRANSFORMATION_KIND_SM4_GCM  (CryptoTransformKind {0, 0, 0, 11})

constexpr CryptoTransformKind c_transform_kind_none = CRYPTO_TRANSFORMATION_KIND_NONE;
constexpr CryptoTransformKind c_transform_kind_sm4_gmac = CRYPTO_TRANSFORMATION_KIND_SM4_GMAC;
constexpr CryptoTransformKind c_transform_kind_sm4_gcm = CRYPTO_TRANSFORMATION_KIND_SM4_GCM;

constexpr std::array<uint8_t, 16> c_empty_sm4_key_material =
{ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };

struct KeyMaterial_SM4_GCM_GMAC
{
    // 加密转换类型，指明具体算法 (NONE, SM4-GCM, or SM4-GMAC)
    CryptoTransformKind transformation_kind = c_transform_kind_none;

    // 主盐值，用于密钥派生或作为 GCM 的 IV 一部分。SM4 块大小为 16 字节。
    std::array<uint8_t, 16> master_salt = c_empty_sm4_key_material;

    // 发送方密钥的 ID
    CryptoTransformKeyId sender_key_id = c_transform_kind_none;
    // 发送方主密钥。SM4 密钥长度固定为 16 字节。
    std::array<uint8_t, 16> master_sender_key = c_empty_sm4_key_material;

    // 特定接收方密钥的 ID
    CryptoTransformKeyId receiver_specific_key_id = c_transform_kind_none;
    // 特定接收方的主密钥。SM4 密钥长度固定为 16 字节。
    std::array<uint8_t, 16> master_receiver_specific_key = c_empty_sm4_key_material;
};

struct KeySessionData
{
    uint32_t session_id = (std::numeric_limits<uint32_t>::max)();
    std::array<uint8_t, 16> SessionKey = c_empty_sm4_key_material;
    uint64_t session_block_counter = 0;
};

typedef std::vector<KeyMaterial_SM4_GCM_GMAC> KeyMaterial_SM4_GCM_GMAC_Seq;

struct MessageFramingProfile {
    uint32_t header_fixed = 0;    // 固定头部开销（字节）
    uint32_t header_mutable_length = 0;    // 每接收方额外头开销
    uint32_t encrypt_percentage = 100;  // 加密数据百分比
    uint32_t tail_fixed = 16;   // AEAD TAG，GCM/GMAC 16 字节
    uint32_t tail_mutable_length = 0;    // 每接收方额外尾开销
};

struct EntityKeyHandle
{
    static const char* const class_id_;

    // 析构时引用的辅助异常对象
    SecurityException* exception_ = {nullptr};

    // 插件安全选项
    PluginEndpointSecurityAttributesMask EndpointPluginAttributes = 0;
    // 本地加密句柄（LocalCryptoHandle）主密钥的存储区，远程加密句柄（RemoteCryptoHandles）中不使用
    KeyMaterial_SM4_GCM_GMAC_Seq EntityKeyMaterial;
    // 父参与者（Parent Participant）主密钥的密钥ID，以及指向相关加密句柄的指针
    CryptoTransformKeyId Participant_master_key_id = c_transformKeyIdZero;
    std::weak_ptr<SMParticipantCryptoHandle> Parent_participant;

    // （正向）接收方特定密钥 - 本质上持有写入方（writer）的主密钥
    KeyMaterial_SM4_GCM_GMAC_Seq Entity2RemoteKeyMaterial;
    // （反向）接收方特定密钥 - 本质上持有远程读取方（remote readers）的主密钥
    KeyMaterial_SM4_GCM_GMAC_Seq Remote2EntityKeyMaterial;
    // 用于加密CryptoToken的密钥材料副本（从父参与者继承）
    KeyMaterial_SM4_GCM_GMAC Participant2ParticipantKxKeyMaterial;
    // 消息级帧参数
    MessageFramingProfile submsg_framing{};

    // 用于存储当前会话密钥，以及判断何时需要更新会话密钥的数据
    KeySessionData Sessions[2];
    uint64_t max_blocks_per_session = 0;
    std::mutex mutex_; // 互斥锁，用于保证线程安全
};

typedef HandleImpl<EntityKeyHandle> SM4GCMGMAC_WriterCryptoHandle;
typedef HandleImpl<EntityKeyHandle> SM4GCMGMAC_ReaderCryptoHandle;
typedef HandleImpl<EntityKeyHandle> SM4GCMGMAC_EntityCryptoHandle;

/**
 * @struct BuffersContainer
 *
 * @brief 保存缓存读取状态的结构
 */
struct BuffersContainer
{
    BuffersContainer(const SerializedBuffer* src, unsigned int num, bool l_endian);
    BuffersContainer();
    /** @brief Buffer for source data */
    const SerializedBuffer* src_buffer;
    /** @brief The buffers number */
    unsigned int buffers_num;
    /** @brief The current using buffer */
    unsigned int cur_buffer_index;
    /** @brief The current buffer offset */
    unsigned int cur_buffer_offset;
    /** @brief true to little endian of source data */
    bool little_endian;
};

enum class PskProtectionKind {
    NONE = 0,           // 不做任何PSK保护
    SIGN_ONLY,          // 仅签名/验签（HMAC-SM3）
    ENCRYPT_ONLY,       // 仅加密（SM4-GCM）
    SIGN_AND_ENCRYPT    // 先签名再加密
};

struct ParticipantKeyHandle
{
    static const char* const class_id_;

    SMSecurityException* exception_ = {nullptr};

    // 插件安全选项
    PluginParticipantSecurityAttributesMask ParticipantPluginAttributes = 0;
    // 本地加密句柄（LocalCryptoHandle）主密钥的存储区，远程加密句柄（RemoteCryptoHandles）中不使用
    KeyMaterial_SM4_GCM_GMAC ParticipantKeyMaterial;
    // （正向）接收方特定密钥 - 本质上持有写入方（writer）的主密钥
    std::vector<KeyMaterial_SM4_GCM_GMAC> Participant2ParticipantKeyMaterial;
    // 用于加密CryptoToken的密钥材料（从父参与者继承）
    std::vector<KeyMaterial_SM4_GCM_GMAC> Participant2ParticipantKxKeyMaterial;
    // （反向）接收方特定密钥 - 本质上持有远程读取方（remote readers）的主密钥
    std::vector<KeyMaterial_SM4_GCM_GMAC> RemoteParticipant2ParticipantKeyMaterial;
    // 所有已匹配写入方（Writers）的加密句柄指针列表
    std::vector<std::shared_ptr<SMDatawriterCryptoHandle>> Writers;
    // 所有已匹配读取方（Readers）的加密句柄指针列表
    std::vector<std::shared_ptr<SMDatareaderCryptoHandle>> Readers;
    // 消息级帧参数
    MessageFramingProfile msg_framing{};

    // 用于存储当前会话密钥，以及判断何时需要更新会话密钥的数据
    KeySessionData Session;
    uint64_t max_blocks_per_session = {0};

    struct PskConfig {
        bool enabled = false;                     // is_rtps_psk_protected
        PskProtectionKind kind = PskProtectionKind::NONE; // SIGN_ONLY / ENCRYPT_ONLY / SIGN_AND_ENCRYPT
    } psk_cfg_;

    struct PskDeriveResult {
        std::vector<uint8_t> master_sender_key;  // 用于SM4-GCM加密/认证
        std::vector<uint8_t> master_salt;        // 用于派生IV/nonce
    };

    std::optional<PskDeriveResult> psk_keys_;        // PSK 派生的主密钥族缓存

    // PSK发送序列号（用于SM4-GCM nonce）
    mutable std::atomic<uint64_t> psk_send_seq{0};
    mutable std::mutex mutex_; // 互斥锁，用于保证线程安全
};

typedef HandleImpl<ParticipantKeyHandle> SM4GCMGMAC_ParticipantCryptoHandle;

/**
 * @struct  CryptoTransformIdentifier
 *
 * @brief   转换标识的数据结构.
 */
struct CryptoTransformIdentifier
{
    /** @brief	转换类型. */
    CryptoTransformKind transformation_kind;
    /** @brief	转换的id. */
    CryptoTransformKeyId transformation_key_id;
};

struct SubMsgHeader
{
    /** @brief   数据类型. */
    unsigned char msg_kind;
    /** @brief   字节顺序标识. */
    unsigned char flags;
    /** @brief   相距下一数据头的长度. */
    unsigned short octets_to_next_header;
};

/**
 * @struct  SecureDataHeader
 *
 * @brief   加密后的数据头，大端序列化
 */
struct CryptoHeader
{
    /** @brief  转换的标识*/
    CryptoTransformIdentifier transform_identifier;
    /** @brief  与后缀共同组成IV */
    unsigned char session_id[4];
    /** @brief  IV后缀 */
    unsigned char initialization_vector_suffix[8];
};

/**
 * @struct   SecureDataBody
 *
 * @brief    信息主体数据结构，大端序列化
 */
struct CryptoContent
{
    /** @brief  数据的类型，无效、加密、签名 */
    enum bodyKind { INVALID, ENCRYPT, SIGN };
    /** @brief   数据主体类型的标识. */
    bodyKind body_flag;
    /** @brief   子消息头 */
    SubMsgHeader header;
    /** @brief   数据长度. */
    unsigned int body_length;
    /** @brief   数据指针. */
    unsigned char* secure_data;
    /** @brief   包含数据内容的缓存，记录在反序列化时的内容 */
    BuffersContainer data_buffers;
};

/**
 * @struct   ReceiverSpecificMAC
 *
 * @brief    接收端详细签名信息数据结构，大端序列化
 */
struct ReceiverSpecificMAC
{
    /** @brief	标识. */
    CryptoTransformKeyId receiver_mac_key_id;
    /** @brief   详细签名信息. */
    unsigned char  receiver_mac[16];
};

/**
 * @struct  SecureDataTag
 *
 * @brief   信息尾部数据结构，大端序列化
 */
struct CryptoFooter
{
    /** @brief  签名信息. */
    unsigned char common_mac[16];
    /** @brief  其他接收者详细签名信息. */
    std::vector<ReceiverSpecificMAC> receiver_specific_macs;
};

struct SecurePrefixSubMsg : public SubMsgHeader
{
    /** @brief   数据头. */
    CryptoHeader sec_data_header;
};

struct SecurePostfixSubMsg : public SubMsgHeader
{
    /** @brief   数据尾部. */
    CryptoFooter sec_data_tag;
};

struct SecureRTPSPrefixSubMsg : public SubMsgHeader
{
    /** @brief   RTPS数据报文头部. */
    unsigned char rtps_header[20];
    /** @brief   加密数据头部. */
    CryptoHeader sec_data_header;
};

typedef SecurePostfixSubMsg SecureRTPSPostfixSubMsg;

TRAVODDS_NAMESPACE_END
#endif // _SECURITY_AUTHENTICATION_SM4GCMGMAC_TYPES_H_