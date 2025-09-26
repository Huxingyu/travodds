#ifndef _SECURITY_AUTHENTICATION_SM4GCMGMAC_TYPES_H_
#define _SECURITY_AUTHENTICATION_SM4GCMGMAC_TYPES_H_

#include <array>
#include <cstdint>
#include "cryptography/CryptoTypes.h"
#include "accesscontrol/ParticipantSecurityAttributes.h"

TRAVODDS_NAMESPACE_BEGIN

// 无加密，无认证标签
#define CRYPTO_TRANSFORMATION_KIND_NONE              { {0, 0, 0, 0} }

// 无加密, SM4-GMAC 认证 (128位)
#define CRYPTO_TRANSFORMATION_KIND_SM4_GMAC          { {0, 0, 0, 10} }

// SM4-GCM 认证加密 (128位)
#define CRYPTO_TRANSFORMATION_KIND_SM4_GCM           { {0, 0, 0, 11} }

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
    std::weak_ptr<ParticipantCryptoHandle> Parent_participant;

    // （正向）接收方特定密钥 - 本质上持有写入方（writer）的主密钥
    KeyMaterial_SM4_GCM_GMAC_Seq Entity2RemoteKeyMaterial;
    // （反向）接收方特定密钥 - 本质上持有远程读取方（remote readers）的主密钥
    KeyMaterial_SM4_GCM_GMAC_Seq Remote2EntityKeyMaterial;
    // 用于加密CryptoToken的密钥材料副本（从父参与者继承）
    // KeyMaterial_SM4_GCM_GMAC Participant2ParticipantKxKeyMaterial;

    // 用于存储当前会话密钥，以及判断何时需要更新会话密钥的数据
    KeySessionData Sessions[2];
    uint64_t max_blocks_per_session = 0;
    std::mutex mutex_; // 互斥锁，用于保证线程安全
};

typedef HandleImpl<EntityKeyHandle> SM4GCMGMAC_WriterCryptoHandle;
typedef HandleImpl<EntityKeyHandle> SM4GCMGMAC_ReaderCryptoHandle;
typedef HandleImpl<EntityKeyHandle> SM4GCMGMAC_EntityCryptoHandle;

struct ParticipantKeyHandle
{
    static const char* const class_id_;

    SecurityException* exception_ = {nullptr};

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
    std::vector<std::shared_ptr<DatawriterCryptoHandle>> Writers;
    // 所有已匹配读取方（Readers）的加密句柄指针列表
    std::vector<std::shared_ptr<DatareaderCryptoHandle>> Readers;

    // 用于存储当前会话密钥，以及判断何时需要更新会话密钥的数据
    KeySessionData Session;
    uint64_t max_blocks_per_session = {0};
    std::mutex mutex_; // 互斥锁，用于保证线程安全
};

typedef HandleImpl<ParticipantKeyHandle> SM4GCMGMAC_ParticipantCryptoHandle;

TRAVODDS_NAMESPACE_END
#endif // _SECURITY_AUTHENTICATION_SM4GCMGMAC_TYPES_H_