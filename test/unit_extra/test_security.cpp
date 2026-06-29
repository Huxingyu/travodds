#ifndef DDSSECURITY
#define DDSSECURITY
#endif

#include "security/securitycommon.h"
#include "security/datatypesupport.h"
#include "security/participantgenericmessage.h"
#include "security/participantmessagesecuredatatypesupport.h"
#include "security/participantmessagedata.h"
#include "security/participantgenericmessagetypesupport.h"
#include "gtest/gtest.h"
#include <cstring>

USING_TRAVODDS_NAMESPACE

#ifdef DDSSECURITY

// ========== SEC_NIL_HANDLE 测试 ==========

TEST(DDSTest, Security_nil_handle_is_zero)
{
    EXPECT_EQ(SEC_NIL_HANDLE, (void*)0);
}

// ========== 全局字符串常量测试 ==========

TEST(DDSTest, Security_global_strings)
{
    EXPECT_EQ(GMCLASSID_SECURITY_AUTH_HANDSHAKE, "dds.sec.auth");
    EXPECT_EQ(GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS, "dds.sec.participant_crypto_tokens");
    EXPECT_EQ(GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS, "dds.sec.datawriter_crypto_tokens");
    EXPECT_EQ(GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS, "dds.sec.datareader_crypto_tokens");
}

// ========== ParticipantSecurityAttributesToMask 测试 ==========

TEST(DDSTest, Security_participant_mask_default)
{
    ParticipantSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    
    ParticipantSecurityAttributesMask mask = ParticipantSecurityAttributesToMask(&attr);
    EXPECT_EQ(mask, 0);
}

TEST(DDSTest, Security_participant_mask_rtps_protected)
{
    ParticipantSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_rtps_protected = true;
    
    ParticipantSecurityAttributesMask mask = ParticipantSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED, 0);
}

TEST(DDSTest, Security_participant_mask_discovery_protected)
{
    ParticipantSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_discovery_protected = true;
    
    ParticipantSecurityAttributesMask mask = ParticipantSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED, 0);
}

TEST(DDSTest, Security_participant_mask_liveliness_protected)
{
    ParticipantSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_liveliness_protected = true;
    
    ParticipantSecurityAttributesMask mask = ParticipantSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED, 0);
}

TEST(DDSTest, Security_participant_mask_all_flags)
{
    ParticipantSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_rtps_protected = true;
    attr.is_discovery_protected = true;
    attr.is_liveliness_protected = true;
    
    ParticipantSecurityAttributesMask mask = ParticipantSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED, 0);
    EXPECT_NE(mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED, 0);
    EXPECT_NE(mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED, 0);
}

// ========== EndpointSecurityAttributesToMask 测试 ==========

TEST(DDSTest, Security_endpoint_mask_default)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_EQ(mask, 0);
}

TEST(DDSTest, Security_endpoint_mask_read_protected)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_read_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED, 0);
}

TEST(DDSTest, Security_endpoint_mask_write_protected)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_write_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED, 0);
}

TEST(DDSTest, Security_endpoint_mask_discovery_protected)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_discovery_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED, 0);
}

TEST(DDSTest, Security_endpoint_mask_liveliness_protected)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_liveliness_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED, 0);
}

TEST(DDSTest, Security_endpoint_mask_submessage_protected)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_submessage_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED, 0);
}

TEST(DDSTest, Security_endpoint_mask_payload_protected)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_payload_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED, 0);
}

TEST(DDSTest, Security_endpoint_mask_key_protected)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_key_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED, 0);
}

TEST(DDSTest, Security_endpoint_mask_all_flags)
{
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    attr.is_read_protected = true;
    attr.is_write_protected = true;
    attr.is_discovery_protected = true;
    attr.is_liveliness_protected = true;
    attr.is_submessage_protected = true;
    attr.is_payload_protected = true;
    attr.is_key_protected = true;
    
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED, 0);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED, 0);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED, 0);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED, 0);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED, 0);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED, 0);
    EXPECT_NE(mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED, 0);
}

// ========== compareBuiltinTopicKey 测试 ==========

TEST(DDSTest, Security_compare_builtin_topic_key_equal)
{
    BuiltinTopicKey_t key1;
    BuiltinTopicKey_t key2;
    memset(&key1, 0, sizeof(key1));
    memset(&key2, 0, sizeof(key2));
    
    EXPECT_EQ(compareBuiltinTopicKey(key1, key2), 0);
}

TEST(DDSTest, Security_compare_builtin_topic_key_less)
{
    BuiltinTopicKey_t key1;
    BuiltinTopicKey_t key2;
    memset(&key1, 0, sizeof(key1));
    memset(&key2, 0, sizeof(key2));
    key2.value[0] = 1;
    
    EXPECT_EQ(compareBuiltinTopicKey(key1, key2), -1);
}

TEST(DDSTest, Security_compare_builtin_topic_key_greater)
{
    BuiltinTopicKey_t key1;
    BuiltinTopicKey_t key2;
    memset(&key1, 0, sizeof(key1));
    memset(&key2, 0, sizeof(key2));
    key1.value[0] = 1;
    
    EXPECT_EQ(compareBuiltinTopicKey(key1, key2), 1);
}

// ========== BuiltinTopicKey_tTypeSupport 测试 ==========

TEST(DDSTest, Security_builtin_key_typesupport_singleton)
{
    auto* ts1 = BuiltinTopicKey_tTypeSupport::get_instance();
    auto* ts2 = BuiltinTopicKey_tTypeSupport::get_instance();
    EXPECT_EQ(ts1, ts2);
}

TEST(DDSTest, Security_builtin_key_create_delete_data)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    ts->delete_data(data);
}

TEST(DDSTest, Security_builtin_key_copy_data)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    BuiltinTopicKey_t* src = static_cast<BuiltinTopicKey_t*>(ts->create_data());
    BuiltinTopicKey_t* dst = static_cast<BuiltinTopicKey_t*>(ts->create_data());
    
    src->value[0] = 42;
    src->value[1] = 100;
    
    ts->copy_data(dst, src);
    EXPECT_EQ(dst->value[0], 42);
    EXPECT_EQ(dst->value[1], 100);
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Security_builtin_key_get_typename)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    EXPECT_STREQ(ts->get_typename(), "BuiltinTopicKey_t");
}

TEST(DDSTest, Security_builtin_key_has_key)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

TEST(DDSTest, Security_builtin_key_get_typeobject)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    EXPECT_EQ(ts->get_typeobject(), nullptr);
}

TEST(DDSTest, Security_builtin_key_serialized_size)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    BuiltinTopicKey_t data;
    memset(&data, 0, sizeof(data));
    
    unsigned int size = ts->get_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0);
    
    unsigned int max_size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, max_size);
}

// ========== SequenceNumber_tTypeSupport 测试 ==========

TEST(DDSTest, Security_sequence_number_typesupport_singleton)
{
    auto* ts1 = SequenceNumber_tTypeSupport::get_instance();
    auto* ts2 = SequenceNumber_tTypeSupport::get_instance();
    EXPECT_EQ(ts1, ts2);
}

TEST(DDSTest, Security_sequence_number_create_delete_data)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    ts->delete_data(data);
}

TEST(DDSTest, Security_sequence_number_copy_data)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    SequenceNumber_t* src = static_cast<SequenceNumber_t*>(ts->create_data());
    SequenceNumber_t* dst = static_cast<SequenceNumber_t*>(ts->create_data());
    
    src->low = 12345;
    src->high = 67890;
    
    ts->copy_data(dst, src);
    EXPECT_EQ(dst->low, 12345);
    EXPECT_EQ(dst->high, 67890);
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Security_sequence_number_get_typename)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    EXPECT_STREQ(ts->get_typename(), "SequenceNumber_t");
}

TEST(DDSTest, Security_sequence_number_has_key)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

TEST(DDSTest, Security_sequence_number_serialized_size)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    SequenceNumber_t data;
    memset(&data, 0, sizeof(data));
    
    unsigned int size = ts->get_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0);
}

// ========== MessageIdentityTypeSupport 测试 ==========

TEST(DDSTest, Security_message_identity_typesupport_singleton)
{
    auto* ts1 = MessageIdentityTypeSupport::get_instance();
    auto* ts2 = MessageIdentityTypeSupport::get_instance();
    EXPECT_EQ(ts1, ts2);
}

TEST(DDSTest, Security_message_identity_create_delete_data)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    ts->delete_data(data);
}

TEST(DDSTest, Security_message_identity_copy_data)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    MessageIdentity* src = static_cast<MessageIdentity*>(ts->create_data());
    MessageIdentity* dst = static_cast<MessageIdentity*>(ts->create_data());
    
    src->sequenceNumber.low = 999;
    src->sequenceNumber.high = 888;
    
    ts->copy_data(dst, src);
    EXPECT_EQ(dst->sequenceNumber.low, 999);
    EXPECT_EQ(dst->sequenceNumber.high, 888);
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Security_message_identity_get_typename)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    EXPECT_STREQ(ts->get_typename(), "MessageIdentity");
}

TEST(DDSTest, Security_message_identity_serialized_size)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    MessageIdentity data;
    memset(&data, 0, sizeof(data));
    
    unsigned int size = ts->get_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0);
}

// ========== Property_tTypeSupport 测试 ==========

TEST(DDSTest, Security_property_typesupport_singleton)
{
    auto* ts1 = Property_tTypeSupport::get_instance();
    auto* ts2 = Property_tTypeSupport::get_instance();
    EXPECT_EQ(ts1, ts2);
}

TEST(DDSTest, Security_property_create_delete_data)
{
    auto* ts = Property_tTypeSupport::get_instance();
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    ts->delete_data(data);
}

TEST(DDSTest, Security_property_copy_data)
{
    auto* ts = Property_tTypeSupport::get_instance();
    Property_t* src = static_cast<Property_t*>(ts->create_data());
    Property_t* dst = static_cast<Property_t*>(ts->create_data());
    
    src->name = "test_name";
    src->value = "test_value";
    
    ts->copy_data(dst, src);
    EXPECT_EQ(dst->name, "test_name");
    EXPECT_EQ(dst->value, "test_value");
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Security_property_get_typename)
{
    auto* ts = Property_tTypeSupport::get_instance();
    EXPECT_STREQ(ts->get_typename(), "Property_t");
}

TEST(DDSTest, Security_property_has_key)
{
    auto* ts = Property_tTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

TEST(DDSTest, Security_property_max_serialized_size)
{
    auto* ts = Property_tTypeSupport::get_instance();
    Property_t data;
    data.name = "test";
    data.value = "value";
    
    unsigned int max_size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(max_size, LENGTH_UNLIMITED);
}

// ========== BinaryProperty_tTypeSupport 测试 ==========

TEST(DDSTest, Security_binary_property_typesupport_singleton)
{
    auto* ts1 = BinaryProperty_tTypeSupport::get_instance();
    auto* ts2 = BinaryProperty_tTypeSupport::get_instance();
    EXPECT_EQ(ts1, ts2);
}

TEST(DDSTest, Security_binary_property_create_delete_data)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    ts->delete_data(data);
}

TEST(DDSTest, Security_binary_property_copy_data)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    BinaryProperty_t* src = static_cast<BinaryProperty_t*>(ts->create_data());
    BinaryProperty_t* dst = static_cast<BinaryProperty_t*>(ts->create_data());
    
    src->name = "binary_name";
    src->value = {0x01, 0x02, 0x03, 0xFF};
    
    ts->copy_data(dst, src);
    EXPECT_EQ(dst->name, "binary_name");
    EXPECT_EQ(dst->value.size(), 4u);
    EXPECT_EQ(dst->value[0], 0x01);
    EXPECT_EQ(dst->value[3], 0xFF);
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Security_binary_property_get_typename)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    EXPECT_STREQ(ts->get_typename(), "BinaryProperty_t");
}

TEST(DDSTest, Security_binary_property_max_serialized_size)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    BinaryProperty_t data;
    data.name = "test";
    data.value = {1, 2, 3};
    
    unsigned int max_size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(max_size, LENGTH_UNLIMITED);
}

// ========== DataHolderTypeSupport 测试 ==========

TEST(DDSTest, Security_dataholder_typesupport_singleton)
{
    auto* ts1 = DataHolderTypeSupport::get_instance();
    auto* ts2 = DataHolderTypeSupport::get_instance();
    EXPECT_EQ(ts1, ts2);
}

TEST(DDSTest, Security_dataholder_create_delete_data)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    ts->delete_data(data);
}

TEST(DDSTest, Security_dataholder_copy_data)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder* src = static_cast<DataHolder*>(ts->create_data());
    DataHolder* dst = static_cast<DataHolder*>(ts->create_data());
    
    src->class_id = "test.class.id";
    
    ts->copy_data(dst, src);
    EXPECT_EQ(dst->class_id, "test.class.id");
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Security_dataholder_get_typename)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    EXPECT_STREQ(ts->get_typename(), "DataHolder");
}

TEST(DDSTest, Security_dataholder_has_key)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

// ========== PluginParticipantSecurityAttributes 测试 ==========

TEST(DDSTest, Security_plugin_participant_default)
{
    PluginParticipantSecurityAttributes attr;
    EXPECT_FALSE(attr.is_rtps_encrypted);
    EXPECT_FALSE(attr.is_discovery_encrypted);
    EXPECT_FALSE(attr.is_liveliness_encrypted);
    EXPECT_FALSE(attr.is_rtps_origin_authenticated);
    EXPECT_FALSE(attr.is_discovery_origin_authenticated);
    EXPECT_FALSE(attr.is_liveliness_origin_authenticated);
}

TEST(DDSTest, Security_plugin_participant_from_mask)
{
    PluginParticipantSecurityAttributesMask mask = 
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED |
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED;
    
    PluginParticipantSecurityAttributes attr(mask);
    EXPECT_TRUE(attr.is_rtps_encrypted);
    EXPECT_TRUE(attr.is_discovery_encrypted);
    EXPECT_FALSE(attr.is_liveliness_encrypted);
}

TEST(DDSTest, Security_plugin_participant_mask_roundtrip)
{
    PluginParticipantSecurityAttributes attr;
    attr.is_rtps_encrypted = true;
    attr.is_discovery_origin_authenticated = true;
    
    PluginParticipantSecurityAttributesMask mask = attr.mask();
    EXPECT_NE(mask & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED, 0);
    EXPECT_NE(mask & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED, 0);
}

// ========== PluginEndpointSecurityAttributes 测试 ==========

TEST(DDSTest, Security_plugin_endpoint_mask)
{
    PluginEndpointSecurityAttributes attr;
    attr.is_submessage_encrypted = true;
    attr.is_payload_encrypted = true;
    
    PluginEndpointSecurityAttributesMask mask = attr.mask();
    EXPECT_NE(mask & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED, 0);
    EXPECT_NE(mask & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED, 0);
    EXPECT_NE(mask & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID, 0);
}

// ========== SecurityException 测试 ==========

TEST(DDSTest, Security_exception_struct)
{
    SecurityException ex;
    ex.code = -1;
    ex.minor_code = 42;
    strncpy(ex.message, "test error", sizeof(ex.message) - 1);
    ex.message[sizeof(ex.message) - 1] = '\0';
    
    EXPECT_EQ(ex.code, -1);
    EXPECT_EQ(ex.minor_code, 42);
    EXPECT_STREQ(ex.message, "test error");
}

// ========== DDSSecurityPluginInstance 测试 ==========

TEST(DDSTest, Security_plugin_instance_struct)
{
    DDSSecurityPluginInstance instance;
    memset(&instance, 0, sizeof(instance));
    
    instance.authentication = (void*)1;
    instance.access_control = (void*)2;
    instance.max_retry_number = 5;
    
    EXPECT_EQ(instance.authentication, (void*)1);
    EXPECT_EQ(instance.access_control, (void*)2);
    EXPECT_EQ(instance.max_retry_number, 5u);
}

// ========== DataTags 测试 ==========

TEST(DDSTest, Security_data_tags)
{
    DataTags_t tags;
    Tag_t tag;
    tag.name = "tag1";
    tag.value = "value1";
    tags.tags.push_back(tag);
    
    EXPECT_EQ(tags.tags.size(), 1u);
    EXPECT_EQ(tags.tags[0].name, "tag1");
    EXPECT_EQ(tags.tags[0].value, "value1");
}

// ========== 新增测试：覆盖更多代码路径 ==========

TEST(DDSTest, Security_participant_mask_only_valid_flag)
{
    // 测试当只有IS_VALID标志时，结果应该为0
    ParticipantSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    // 不设置任何保护标志，只有IS_VALID
    ParticipantSecurityAttributesMask mask = ParticipantSecurityAttributesToMask(&attr);
    EXPECT_EQ(mask, 0);
}

TEST(DDSTest, Security_endpoint_mask_only_valid_flag)
{
    // 测试当只有IS_VALID标志时，结果应该为0
    EndpointSecurityAttributes attr;
    memset(&attr, 0, sizeof(attr));
    EndpointSecurityAttributesMask mask = EndpointSecurityAttributesToMask(&attr);
    EXPECT_EQ(mask, 0);
}

TEST(DDSTest, Security_plugin_participant_all_flags)
{
    PluginParticipantSecurityAttributesMask mask = 
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED |
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED |
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED |
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED |
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED |
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED;
    
    PluginParticipantSecurityAttributes attr(mask);
    EXPECT_TRUE(attr.is_rtps_encrypted);
    EXPECT_TRUE(attr.is_discovery_encrypted);
    EXPECT_TRUE(attr.is_liveliness_encrypted);
    EXPECT_TRUE(attr.is_rtps_origin_authenticated);
    EXPECT_TRUE(attr.is_discovery_origin_authenticated);
    EXPECT_TRUE(attr.is_liveliness_origin_authenticated);
}

TEST(DDSTest, Security_plugin_endpoint_all_flags)
{
    PluginEndpointSecurityAttributesMask mask = 
        PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED |
        PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED;
    
    PluginEndpointSecurityAttributes attr;
    attr.is_submessage_encrypted = true;
    attr.is_submessage_origin_authenticated = false;
    attr.is_payload_encrypted = true;
    EXPECT_TRUE(attr.is_submessage_encrypted);
    EXPECT_TRUE(attr.is_payload_encrypted);
}

TEST(DDSTest, Security_plugin_endpoint_default)
{
    PluginEndpointSecurityAttributes attr;
    EXPECT_FALSE(attr.is_submessage_encrypted);
    EXPECT_FALSE(attr.is_payload_encrypted);
    EXPECT_FALSE(attr.is_submessage_origin_authenticated);
    EXPECT_EQ(attr.mask() & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID, PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID);
}

// ========== DataHolder 序列化/反序列化测试 ==========

TEST(DDSTest, Security_dataholder_serialize_deserialize)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    
    DataHolder src;
    src.class_id = "test.class.id";
    Property_t prop;
    prop.name = "prop_name";
    prop.value = "prop_value";
    src.properties.push_back(prop);
    
    BinaryProperty_t bin_prop;
    bin_prop.name = "bin_prop";
    bin_prop.value = {0x01, 0x02, 0x03};
    src.binary_properties.push_back(bin_prop);
    
    // 获取序列化大小
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    // 序列化
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    // 反序列化
    DataHolder dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    // 验证
    EXPECT_EQ(dst.class_id, src.class_id);
    EXPECT_EQ(dst.properties.size(), 1u);
    EXPECT_EQ(dst.properties[0].name, "prop_name");
    EXPECT_EQ(dst.properties[0].value, "prop_value");
    EXPECT_EQ(dst.binary_properties.size(), 1u);
    EXPECT_EQ(dst.binary_properties[0].name, "bin_prop");
    EXPECT_EQ(dst.binary_properties[0].value.size(), 3u);
    
    delete[] buffer;
}

TEST(DDSTest, Security_dataholder_serialize_empty)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    
    DataHolder src;
    src.class_id = "empty";
    
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    DataHolder dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    EXPECT_EQ(dst.class_id, "empty");
    EXPECT_EQ(dst.properties.size(), 0u);
    EXPECT_EQ(dst.binary_properties.size(), 0u);
    
    delete[] buffer;
}

TEST(DDSTest, Security_dataholder_max_serialized_size)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test";
    
    unsigned int max_size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(max_size, LENGTH_UNLIMITED);
}

TEST(DDSTest, Security_dataholder_get_typeobject)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    EXPECT_EQ(ts->get_typeobject(), nullptr);
}

TEST(DDSTest, Security_dataholder_make_key)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test";
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(&data, handle, false);
    // 由于has_key()返回false，应该成功
    EXPECT_EQ(ret, 0);
}

// ========== ParticipantGenericMessage 序列化/反序列化测试 ==========

TEST(DDSTest, Security_participant_generic_message_serialize_deserialize)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    
    ParticipantGenericMessage src;
    src.messageIdentity.sourceGuid.value[0] = 1;
    src.messageIdentity.sequenceNumber.low = 100;
    src.messageIdentity.sequenceNumber.high = 200;
    src.relatedMessageIdentity.sourceGuid.value[0] = 2;
    src.relatedMessageIdentity.sequenceNumber.low = 300;
    src.relatedMessageIdentity.sequenceNumber.high = 400;
    src.destinationParticipantKey.value[0] = 3;
    src.destinationEndpointKey.value[0] = 4;
    src.sourceEndpointKey.value[0] = 5;
    src.messageClassId = "dds.sec.auth.request";
    
    DataHolder msg_data;
    msg_data.class_id = "msg_data";
    src.messageData.push_back(msg_data);
    
    // 获取序列化大小
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    // 序列化
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    // 反序列化
    ParticipantGenericMessage dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    // 验证
    EXPECT_EQ(dst.messageClassId, "dds.sec.auth.request");
    EXPECT_EQ(dst.messageData.size(), 1u);
    EXPECT_EQ(dst.messageData[0].class_id, "msg_data");
    
    delete[] buffer;
}

TEST(DDSTest, Security_participant_generic_message_max_serialized_size)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    
    unsigned int max_size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(max_size, LENGTH_UNLIMITED);
}

TEST(DDSTest, Security_participant_generic_message_get_typename)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    EXPECT_STREQ(ts->get_typename(), "ParticipantGenericMessage");
}

TEST(DDSTest, Security_participant_generic_message_has_key)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

TEST(DDSTest, Security_participant_generic_message_get_typeobject)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    EXPECT_EQ(ts->get_typeobject(), nullptr);
}

TEST(DDSTest, Security_participant_generic_message_make_key)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(&data, handle, false);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_participant_generic_message_copy_data)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage* src = static_cast<ParticipantGenericMessage*>(ts->create_data());
    ParticipantGenericMessage* dst = static_cast<ParticipantGenericMessage*>(ts->create_data());
    
    src->messageClassId = "copy_test_msg";
    src->messageIdentity.sequenceNumber.low = 999;
    
    ts->copy_data(dst, src);
    EXPECT_EQ(dst->messageClassId, "copy_test_msg");
    EXPECT_EQ(dst->messageIdentity.sequenceNumber.low, 999);
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

// ========== Property 序列化/反序列化测试 ==========

TEST(DDSTest, Security_property_serialize_deserialize)
{
    auto* ts = Property_tTypeSupport::get_instance();
    
    Property_t src;
    src.name = "test_property";
    src.value = "test_value";
    
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    Property_t dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    EXPECT_EQ(dst.name, "test_property");
    EXPECT_EQ(dst.value, "test_value");
    
    delete[] buffer;
}

// ========== BinaryProperty 序列化/反序列化测试 ==========

TEST(DDSTest, Security_binary_property_serialize_deserialize)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    
    BinaryProperty_t src;
    src.name = "binary_prop";
    src.value = {0xDE, 0xAD, 0xBE, 0xEF};
    
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    BinaryProperty_t dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    EXPECT_EQ(dst.name, "binary_prop");
    EXPECT_EQ(dst.value.size(), 4u);
    EXPECT_EQ(dst.value[0], 0xDE);
    EXPECT_EQ(dst.value[3], 0xEF);
    
    delete[] buffer;
}

// ========== MessageIdentity 序列化/反序列化测试 ==========

TEST(DDSTest, Security_message_identity_serialize_deserialize)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    
    MessageIdentity src;
    src.sourceGuid.value[0] = 42;
    src.sourceGuid.value[1] = 100;
    src.sequenceNumber.low = 12345;
    src.sequenceNumber.high = 67890;
    
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    MessageIdentity dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    EXPECT_EQ(dst.sourceGuid.value[0], 42);
    EXPECT_EQ(dst.sourceGuid.value[1], 100);
    EXPECT_EQ(dst.sequenceNumber.low, 12345);
    EXPECT_EQ(dst.sequenceNumber.high, 67890);
    
    delete[] buffer;
}

// ========== SequenceNumber 序列化/反序列化测试 ==========

TEST(DDSTest, Security_sequence_number_serialize_deserialize)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    
    SequenceNumber_t src;
    src.low = 99999;
    src.high = 88888;
    
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    SequenceNumber_t dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    EXPECT_EQ(dst.low, 99999);
    EXPECT_EQ(dst.high, 88888);
    
    delete[] buffer;
}

// ========== BuiltinTopicKey 序列化/反序列化测试 ==========

TEST(DDSTest, Security_builtin_key_serialize_deserialize)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    
    BuiltinTopicKey_t src;
    memset(&src, 0, sizeof(src));
    src.value[0] = 0x12345678;
    src.value[1] = 0xABCDEF00;
    src.value[2] = 0x11223344;
    src.value[3] = 0x55667788;
    
    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    BuiltinTopicKey_t dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    EXPECT_EQ(dst.value[0], 0x12345678);
    EXPECT_EQ(dst.value[1], 0xABCDEF00);
    EXPECT_EQ(dst.value[2], 0x11223344);
    EXPECT_EQ(dst.value[3], 0x55667788);
    delete[] buffer;
}

// ========== compareBuiltinTopicKey 更多测试 ==========

TEST(DDSTest, Security_compare_builtin_topic_key_different_bytes)
{
    BuiltinTopicKey_t key1;
    BuiltinTopicKey_t key2;
    memset(&key1, 0, sizeof(key1));
    memset(&key2, 0, sizeof(key2));
    
    key1.value[5] = 1;
    key2.value[5] = 2;
    
    EXPECT_EQ(compareBuiltinTopicKey(key1, key2), -1);
}

TEST(DDSTest, Security_compare_builtin_topic_key_equal_all)
{
    BuiltinTopicKey_t key1;
    BuiltinTopicKey_t key2;
    memset(&key1, 0xFF, sizeof(key1));
    memset(&key2, 0xFF, sizeof(key2));
    
    EXPECT_EQ(compareBuiltinTopicKey(key1, key2), 0);
}

// ========== ParticipantMessageSecureDataTypeSupport 测试 ==========

TEST(DDSTest, Security_participant_message_secure_get_instance)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    // 验证单例
    ParticipantMessageSecureDataTypeSupport* ts2 = ParticipantMessageSecureDataTypeSupport::get_instance();
    EXPECT_EQ(ts, ts2);
}

TEST(DDSTest, Security_participant_message_secure_create_delete_data)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ParticipantMessageSecureData* msg = static_cast<ParticipantMessageSecureData*>(data);
    memset(msg->m_kind, 0, sizeof(msg->m_kind));
    memcpy(msg->m_kind, "TEST", 4);
    msg->m_extraData.clear();
    
    ts->delete_data(data);
}

TEST(DDSTest, Security_participant_message_secure_copy_data)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    
    void* src = ts->create_data();
    void* dst = ts->create_data();
    
    ParticipantMessageSecureData* srcMsg = static_cast<ParticipantMessageSecureData*>(src);
    memcpy(srcMsg->m_kind, "KIND", 4);
    srcMsg->m_extraData.assign({'h', 'e', 'l', 'l', 'o'});
    
    int ret = ts->copy_data(dst, src);
    EXPECT_EQ(ret, 0);
    
    ParticipantMessageSecureData* dstMsg = static_cast<ParticipantMessageSecureData*>(dst);
    EXPECT_EQ(memcmp(dstMsg->m_kind, "KIND", 4), 0);
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Security_participant_message_secure_serialize_deserialize)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    
    void* data = ts->create_data();
    ParticipantMessageSecureData* msg = static_cast<ParticipantMessageSecureData*>(data);
    memcpy(msg->m_kind, "ABCD", 4);
    msg->m_extraData.assign({'e', 'x', 't', 'r', 'a', '1'});
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    EXPECT_GT(size, 0u);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    ParticipantMessageSecureData* dstMsg = static_cast<ParticipantMessageSecureData*>(dst);
    EXPECT_EQ(memcmp(dstMsg->m_kind, "ABCD", 4), 0);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Security_participant_message_secure_max_size)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    unsigned int maxSize = ts->get_max_serialized_data_size(nullptr, 0);
    EXPECT_EQ(maxSize, LENGTH_UNLIMITED);
}

TEST(DDSTest, Security_participant_message_secure_has_key)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

TEST(DDSTest, Security_participant_message_secure_get_typename)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    const char* name = ts->get_typename();
    EXPECT_STREQ(name, "ParticipantMessageSecureData");
}

TEST(DDSTest, Security_participant_message_secure_serialize_key)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    
    void* data = ts->create_data();
    ParticipantMessageSecureData* msg = static_cast<ParticipantMessageSecureData*>(data);
    memcpy(msg->m_kind, "KEY1", 4);
    msg->m_extraData.clear();
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
}

TEST(DDSTest, Security_participant_message_secure_make_key)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    
    void* data = ts->create_data();
    ParticipantMessageSecureData* msg = static_cast<ParticipantMessageSecureData*>(data);
    memcpy(msg->m_kind, "MK12", 4);
    msg->m_extraData.clear();
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(data, handle, false);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
}

TEST(DDSTest, Security_participant_message_secure_get_instancehandle)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    
    void* data = ts->create_data();
    ParticipantMessageSecureData* msg = static_cast<ParticipantMessageSecureData*>(data);
    memcpy(msg->m_kind, "IH34", 4);
    msg->m_extraData.clear();
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = 100;
    
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
    
    ts->delete_data(data);
}

TEST(DDSTest, Security_participant_message_secure_get_typeobject)
{
    ParticipantMessageSecureDataTypeSupport* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    TypeObject* typeObj = ts->get_typeobject();
    // get_typeobject may return nullptr if type object is not implemented
    // Just verify the call doesn't crash
    (void)typeObj;
}

// ========== Additional datatypesupport.cpp coverage tests ==========

TEST(DDSTest, Security_builtin_topic_key_get_max_serialized_data_size)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    BuiltinTopicKey_t data;
    memset(&data, 0, sizeof(data));
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0u);
}

TEST(DDSTest, Security_builtin_topic_key_get_instancehandle)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    BuiltinTopicKey_t data;
    memset(&data, 0, sizeof(data));
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(&data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Security_sequence_number_get_max_serialized_data_size)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    SequenceNumber_t data;
    data.low = 0; data.high = 0;
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0u);
}

TEST(DDSTest, Security_sequence_number_get_instancehandle)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    SequenceNumber_t data;
    data.low = 1; data.high = 0;
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(&data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Security_message_identity_get_max_serialized_data_size)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    MessageIdentity data;
    memset(&data, 0, sizeof(data));
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0u);
}

TEST(DDSTest, Security_message_identity_get_instancehandle)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    MessageIdentity data;
    memset(&data, 0, sizeof(data));
    char buffer[200] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 200;
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(&data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Security_property_get_max_serialized_data_size)
{
    auto* ts = Property_tTypeSupport::get_instance();
    Property_t data;
    data.name = "test"; data.value = "val";
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, LENGTH_UNLIMITED);
}

TEST(DDSTest, Security_property_get_instancehandle)
{
    auto* ts = Property_tTypeSupport::get_instance();
    Property_t data;
    data.name = "n"; data.value = "v";
    char buffer[200] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 200;
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(&data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Security_binary_property_get_max_serialized_data_size)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    BinaryProperty_t data;
    data.name = "bin_test";
    data.value.push_back(0x01);
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, LENGTH_UNLIMITED);
}

TEST(DDSTest, Security_binary_property_get_instancehandle)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    BinaryProperty_t data;
    data.name = "n"; data.value.push_back(0x42);
    char buffer[200] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 200;
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(&data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Security_data_holder_get_max_serialized_data_size)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, LENGTH_UNLIMITED);
}

TEST(DDSTest, Security_data_holder_get_instancehandle)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test";
    char buffer[200] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 200;
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(&data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Security_participant_generic_message_get_max_serialized_data_size)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, LENGTH_UNLIMITED);
}

TEST(DDSTest, Security_participant_generic_message_get_instancehandle)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    char buffer[500] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 500;
    CdrSerializer serializer(&sbuf);
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->get_instancehandle(&data, &serializer, handle, false);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Security_participant_message_secure_get_max_serialized_data_size)
{
    auto* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    ParticipantMessageSecureData data;
    memcpy(data.m_kind, "TEST", 4);
    data.m_extraData.clear();
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, LENGTH_UNLIMITED);
}

// ========== Comprehensive serialize/deserialize with data ==========

TEST(DDSTest, Security_builtin_topic_key_full_roundtrip)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    BuiltinTopicKey_t src;
    src.value[0] = 1; src.value[1] = 2; src.value[2] = 3;

    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = size;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);

    BuiltinTopicKey_t dst;
    memset(&dst, 0, sizeof(dst));
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dst.value[0], 1);
    EXPECT_EQ(dst.value[1], 2);
    EXPECT_EQ(dst.value[2], 3);
    delete[] buffer;
}

TEST(DDSTest, Security_sequence_number_full_roundtrip)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    SequenceNumber_t src;
    src.low = 42; src.high = 7;

    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = size;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);

    SequenceNumber_t dst;
    memset(&dst, 0, sizeof(dst));
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dst.low, 42);
    EXPECT_EQ(dst.high, 7);
    delete[] buffer;
}

TEST(DDSTest, Security_message_identity_full_roundtrip)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    MessageIdentity src;
    src.sourceGuid.value[0] = 10; src.sourceGuid.value[1] = 20;
    src.sequenceNumber.low = 100; src.sequenceNumber.high = 5;

    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    char* buffer = new char[size];
    memset(buffer, 0, size);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = size;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);

    MessageIdentity dst;
    memset(&dst, 0, sizeof(dst));
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dst.sourceGuid.value[0], 10);
    EXPECT_EQ(dst.sequenceNumber.low, 100);
    delete[] buffer;
}

TEST(DDSTest, Security_binary_property_full_roundtrip)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    BinaryProperty_t src;
    src.name = "binary_key";
    src.value.push_back(0xDE);
    src.value.push_back(0xAD);

    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    char* buffer = new char[size + 10];
    memset(buffer, 0, size + 10);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = size + 10;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);

    BinaryProperty_t dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dst.name, "binary_key");
    EXPECT_EQ(dst.value.size(), 2u);
    EXPECT_EQ(dst.value[0], 0xDE);
    delete[] buffer;
}

TEST(DDSTest, Security_data_holder_full_roundtrip_with_data)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder src;
    src.class_id = "dds.sec.auth";
    Property_t prop;
    prop.name = "prop1"; prop.value = "val1";
    src.properties.push_back(prop);
    BinaryProperty_t bprop;
    bprop.name = "bprop1"; bprop.value.push_back(0x01);
    src.binary_properties.push_back(bprop);

    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = size + 100;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);

    DataHolder dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dst.class_id, "dds.sec.auth");
    EXPECT_EQ(dst.properties.size(), 1u);
    EXPECT_EQ(dst.properties[0].name, "prop1");
    delete[] buffer;
}

TEST(DDSTest, Security_participant_generic_message_full_roundtrip_with_data)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage src;
    src.messageIdentity.sourceGuid.value[0] = 1;
    src.messageIdentity.sequenceNumber.low = 42;
    src.relatedMessageIdentity.sourceGuid.value[0] = 2;
    src.destinationParticipantKey.value[0] = 3;
    src.destinationEndpointKey.value[0] = 4;
    src.sourceEndpointKey.value[0] = 5;
    src.messageClassId = "dds.sec.auth";
    DataHolder dh;
    dh.class_id = "test_holder";
    src.messageData.push_back(dh);

    unsigned int size = ts->get_serialized_data_size(&src, 0);
    EXPECT_GT(size, 0u);
    char* buffer = new char[size + 200];
    memset(buffer, 0, size + 200);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = size + 200;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&src, &serializer, 0);
    EXPECT_EQ(ret, 0);

    ParticipantGenericMessage dst;
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(&dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dst.messageClassId, "dds.sec.auth");
    EXPECT_EQ(dst.messageData.size(), 1u);
    delete[] buffer;
}

// ========== BuiltinTopicKey MakeKey and serialize_key tests ==========

TEST(DDSTest, Security_builtin_topic_key_make_key)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    BuiltinTopicKey_t data;
    memset(&data, 0, sizeof(data));
    data.value[0] = 1;
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(&data, handle, false);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_builtin_topic_key_serialize_key)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    BuiltinTopicKey_t data;
    memset(&data, 0, sizeof(data));
    data.value[0] = 2;
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_builtin_topic_key_get_typeobject)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    TypeObject* obj = ts->get_typeobject();
    EXPECT_EQ(obj, nullptr);
}

TEST(DDSTest, Security_builtin_topic_key_has_key)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

TEST(DDSTest, Security_builtin_topic_key_get_typename)
{
    auto* ts = BuiltinTopicKey_tTypeSupport::get_instance();
    const char* name = ts->get_typename();
    EXPECT_STREQ(name, "BuiltinTopicKey_t");
}

// ========== SequenceNumber MakeKey and serialize_key tests ==========

TEST(DDSTest, Security_sequence_number_make_key)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    SequenceNumber_t data;
    data.low = 42; data.high = 7;
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(&data, handle, false);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_sequence_number_serialize_key)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    SequenceNumber_t data;
    data.low = 100; data.high = 50;
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_sequence_number_get_typeobject)
{
    auto* ts = SequenceNumber_tTypeSupport::get_instance();
    TypeObject* obj = ts->get_typeobject();
    EXPECT_EQ(obj, nullptr);
}

// ========== MessageIdentity MakeKey and serialize_key tests ==========

TEST(DDSTest, Security_message_identity_make_key)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    MessageIdentity data;
    memset(&data, 0, sizeof(data));
    data.sourceGuid.value[0] = 1;
    data.sequenceNumber.low = 42;
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(&data, handle, false);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_message_identity_serialize_key)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    MessageIdentity data;
    memset(&data, 0, sizeof(data));
    data.sourceGuid.value[0] = 2;
    data.sequenceNumber.low = 100;
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_message_identity_get_typeobject)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    TypeObject* obj = ts->get_typeobject();
    EXPECT_EQ(obj, nullptr);
}

TEST(DDSTest, Security_message_identity_has_key)
{
    auto* ts = MessageIdentityTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

// ========== Property_t MakeKey and serialize_key tests ==========

TEST(DDSTest, Security_property_make_key)
{
    auto* ts = Property_tTypeSupport::get_instance();
    Property_t data;
    data.name = "test_name";
    data.value = "test_value";
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(&data, handle, false);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_property_serialize_key)
{
    auto* ts = Property_tTypeSupport::get_instance();
    Property_t data;
    data.name = "key_name";
    data.value = "key_value";
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_property_get_typeobject)
{
    auto* ts = Property_tTypeSupport::get_instance();
    TypeObject* obj = ts->get_typeobject();
    EXPECT_EQ(obj, nullptr);
}

// ========== BinaryProperty_t MakeKey and serialize_key tests ==========

TEST(DDSTest, Security_binary_property_make_key)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    BinaryProperty_t data;
    data.name = "binary_name";
    data.value.push_back(0x01);
    data.value.push_back(0x02);
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(&data, handle, false);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_binary_property_serialize_key)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    BinaryProperty_t data;
    data.name = "bin_key";
    data.value.push_back(0xAA);
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
}

TEST(DDSTest, Security_binary_property_get_typeobject)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    TypeObject* obj = ts->get_typeobject();
    EXPECT_EQ(obj, nullptr);
}

TEST(DDSTest, Security_binary_property_has_key)
{
    auto* ts = BinaryProperty_tTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

// ========== DataHolder MakeKey and serialize_key tests ==========

TEST(DDSTest, Security_dataholder_serialize_key)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
}

// ========== ParticipantGenericMessage MakeKey and serialize_key tests ==========

TEST(DDSTest, Security_participant_generic_message_serialize_key)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "test_class";
    
    char buffer[200] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 200;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_key(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
}

// ========== DataHolder with properties and binary_properties tests ==========

TEST(DDSTest, Security_dataholder_with_properties)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    
    Property_t prop;
    prop.name = "prop1";
    prop.value = "value1";
    data.properties.push_back(prop);
    
    Property_t prop2;
    prop2.name = "prop2";
    prop2.value = "value2";
    data.properties.push_back(prop2);
    
    // Serialize
    char buffer[512] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 512;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    // Deserialize
    CdrDeserializer deserializer(&sbuf);
    DataHolder data2;
    ret = ts->deserialize_data(&data2, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(data2.class_id, "test_class");
    EXPECT_EQ(data2.properties.size(), 2u);
    EXPECT_EQ(data2.properties[0].name, "prop1");
    EXPECT_EQ(data2.properties[0].value, "value1");
}

TEST(DDSTest, Security_dataholder_with_binary_properties)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    
    BinaryProperty_t bprop;
    bprop.name = "bprop1";
    bprop.value.push_back(0x01);
    bprop.value.push_back(0x02);
    bprop.value.push_back(0x03);
    data.binary_properties.push_back(bprop);
    
    // Serialize
    char buffer[512] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 512;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    // Deserialize
    CdrDeserializer deserializer(&sbuf);
    DataHolder data2;
    ret = ts->deserialize_data(&data2, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(data2.class_id, "test_class");
    EXPECT_EQ(data2.binary_properties.size(), 1u);
    EXPECT_EQ(data2.binary_properties[0].name, "bprop1");
    EXPECT_EQ(data2.binary_properties[0].value.size(), 3u);
}

TEST(DDSTest, Security_dataholder_with_both_properties)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    
    Property_t prop;
    prop.name = "prop1";
    prop.value = "value1";
    data.properties.push_back(prop);
    
    BinaryProperty_t bprop;
    bprop.name = "bprop1";
    bprop.value.push_back(0xAA);
    bprop.value.push_back(0xBB);
    data.binary_properties.push_back(bprop);
    
    // Serialize
    char buffer[512] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 512;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    // Deserialize
    CdrDeserializer deserializer(&sbuf);
    DataHolder data2;
    ret = ts->deserialize_data(&data2, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(data2.class_id, "test_class");
    EXPECT_EQ(data2.properties.size(), 1u);
    EXPECT_EQ(data2.binary_properties.size(), 1u);
}

// ========== ParticipantGenericMessage with full data tests ==========

TEST(DDSTest, Security_participant_generic_message_full)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "dds.sec.auth";
    
    // Initialize messageIdentity
    for (int i = 0; i < 4; ++i) {
        data.messageIdentity.sourceGuid.value[i] = i + 1;
    }
    data.messageIdentity.sequenceNumber.high = 0;
    data.messageIdentity.sequenceNumber.low = 1;
    
    // Initialize relatedMessageIdentity
    for (int i = 0; i < 4; ++i) {
        data.relatedMessageIdentity.sourceGuid.value[i] = i + 2;
    }
    data.relatedMessageIdentity.sequenceNumber.high = 0;
    data.relatedMessageIdentity.sequenceNumber.low = 2;
    
    // Initialize destinationParticipantKey
    for (int i = 0; i < 4; ++i) {
        data.destinationParticipantKey.value[i] = i + 3;
    }
    
    // Initialize destinationEndpointKey
    for (int i = 0; i < 4; ++i) {
        data.destinationEndpointKey.value[i] = i + 4;
    }
    
    // Initialize sourceEndpointKey
    for (int i = 0; i < 4; ++i) {
        data.sourceEndpointKey.value[i] = i + 5;
    }
    
    DataHolder holder;
    holder.class_id = "test_holder";
    Property_t prop;
    prop.name = "test_prop";
    prop.value = "test_value";
    holder.properties.push_back(prop);
    data.messageData.push_back(holder);
    
    // Serialize
    char buffer[1024] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1024;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    // Deserialize
    CdrDeserializer deserializer(&sbuf);
    ParticipantGenericMessage data2;
    memset(&data2, 0, sizeof(data2));
    ret = ts->deserialize_data(&data2, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(data2.messageClassId, "dds.sec.auth");
    EXPECT_EQ(data2.messageIdentity.sequenceNumber.low, 1);
    EXPECT_EQ(data2.relatedMessageIdentity.sequenceNumber.low, 2);
    EXPECT_EQ(data2.messageData.size(), 1u);
}

// ========== DataHolder get_serialized_data_size tests ==========

TEST(DDSTest, Security_dataholder_get_serialized_size)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test";
    
    Property_t prop;
    prop.name = "p1";
    prop.value = "v1";
    data.properties.push_back(prop);
    
    BinaryProperty_t bprop;
    bprop.name = "bp1";
    bprop.value.push_back(0x01);
    data.binary_properties.push_back(bprop);
    
    unsigned int size = ts->get_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0u);
}

TEST(DDSTest, Security_dataholder_get_max_serialized_size)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, LENGTH_UNLIMITED);
}

// ========== ParticipantGenericMessage get_serialized_data_size tests ==========

TEST(DDSTest, Security_participant_generic_message_get_serialized_size)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "test";
    
    unsigned int size = ts->get_serialized_data_size(&data, 0);
    EXPECT_GT(size, 0u);
}

TEST(DDSTest, Security_participant_generic_message_get_max_serialized_size)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    
    unsigned int size = ts->get_max_serialized_data_size(&data, 0);
    EXPECT_EQ(size, LENGTH_UNLIMITED);
}

// ========== Error path tests for coverage ==========

TEST(DDSTest, Security_generic_message_serialize_error_path)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "test_class_with_long_name_for_buffer_overflow";
    DataHolder dh;
    dh.class_id = "data_holder_class_long";
    Property_t prop;
    prop.name = "property_name_long";
    prop.value = "property_value_long";
    data.messageData.push_back(dh);
    
    // Small buffer to trigger error paths
    char buffer[10] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 10;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Security_generic_message_deserialize_error_path)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    
    char buffer[5] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 5;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Security_dataholder_serialize_error_path)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    Property_t prop;
    prop.name = "name";
    prop.value = "value";
    data.properties.push_back(prop);
    
    char buffer[5] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 5;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Security_dataholder_deserialize_error_path)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    
    char buffer[3] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 3;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Security_participant_message_secure_serialize_error_path)
{
    auto* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    ParticipantMessageSecureData data;
    memset(&data, 0, sizeof(data));
    strncpy(data.m_kind, "TEST", sizeof(data.m_kind) - 1);
    data.m_extraData = {0x01, 0x02};
    
    char buffer[5] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 5;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Security_participant_message_secure_deserialize_error_path)
{
    auto* ts = ParticipantMessageSecureDataTypeSupport::get_instance();
    ParticipantMessageSecureData data;
    memset(&data, 0, sizeof(data));
    
    char buffer[3] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 3;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Security_generic_message_make_key_error_path)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "test";
    
    InstanceHandle_t handle;
    ReturnCode_t ret = ts->MakeKey(&data, handle, false);
    // This should succeed but covers the MakeKey path
    EXPECT_EQ(ret, RETCODE_OK);
}

// Test DataHolder serialization with small buffer to cover error paths
TEST(DDSTest, Security_dataholder_serialize_small_buffer)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    
    char buffer[0];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 0;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_data(&data, &cdr, 0);
    EXPECT_EQ(ret, -1);
}

// Test DataHolder deserialization with small buffer to cover error paths
TEST(DDSTest, Security_dataholder_deserialize_small_buffer)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    
    char buffer[0];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 0;
    CdrDeserializer des(&sbuf);
    
    int ret = ts->deserialize_data(&data, &des, 0);
    EXPECT_EQ(ret, -1);
}

// Test ParticipantGenericMessage serialization with small buffer
TEST(DDSTest, Security_generic_message_serialize_small_buffer)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "test";
    
    char buffer[0];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 0;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_data(&data, &cdr, 0);
    EXPECT_EQ(ret, -1);
}

// Test ParticipantGenericMessage deserialization with small buffer
TEST(DDSTest, Security_generic_message_deserialize_small_buffer)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    
    char buffer[0];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 0;
    CdrDeserializer des(&sbuf);
    
    int ret = ts->deserialize_data(&data, &des, 0);
    EXPECT_EQ(ret, -1);
}

// Test DataHolder with properties serialization error path - buffer large enough for class_id but not properties
TEST(DDSTest, Security_dataholder_with_properties_serialize_error)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    Property_t prop;
    prop.name = "key"; prop.value = "val";
    data.properties.push_back(prop);
    
    // Buffer large enough for class_id string + length (about 15 bytes) but not for property
    char buffer[20];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 20;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_data(&data, &cdr, 0);
    EXPECT_EQ(ret, -1);
}

// Test DataHolder with binary_properties serialization error path
TEST(DDSTest, Security_dataholder_with_binary_properties_serialize_error)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    data.class_id = "test_class";
    BinaryProperty_t prop;
    prop.name = "bin_key";
    prop.value.resize(100);
    data.binary_properties.push_back(prop);
    
    char buffer[20];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 20;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_data(&data, &cdr, 0);
    EXPECT_EQ(ret, -1);
}

// Test DataHolder with properties deserialization error path
TEST(DDSTest, Security_dataholder_with_properties_deserialize_error)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    
    // Buffer with valid class_id and property count but truncated property data
    char buffer[20] = {0};
    buffer[0] = 10; // string length
    buffer[4] = 1;  // property count = 1
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 20;
    CdrDeserializer des(&sbuf);
    
    int ret = ts->deserialize_data(&data, &des, 0);
    EXPECT_EQ(ret, -1);
}

// Test DataHolder with binary_properties deserialization error path
TEST(DDSTest, Security_dataholder_with_binary_properties_deserialize_error)
{
    auto* ts = DataHolderTypeSupport::get_instance();
    DataHolder data;
    
    // Buffer with valid class_id, 0 properties, but 1 binary_property with truncated data
    char buffer[24] = {0};
    buffer[0] = 10; // string length
    buffer[4] = 0;  // property count = 0
    buffer[8] = 1;  // binary_property count = 1
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 24;
    CdrDeserializer des(&sbuf);
    
    int ret = ts->deserialize_data(&data, &des, 0);
    EXPECT_EQ(ret, -1);
}

// Test ParticipantGenericMessage with messageData serialization error path
TEST(DDSTest, Security_generic_message_with_data_serialize_error)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "test";
    data.messageData.resize(1);
    
    char buffer[1];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_data(&data, &cdr, 0);
    EXPECT_EQ(ret, -1);
}

// Test ParticipantGenericMessage serialization error path - buffer large enough for early fields but not messageData
TEST(DDSTest, Security_generic_message_with_data_serialize_error_v2)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    data.messageClassId = "test";
    data.messageData.resize(1);
    
    // Buffer large enough for header fields but not for messageData
    char buffer[100];
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_data(&data, &cdr, 0);
    EXPECT_EQ(ret, -1);
}

// Test ParticipantGenericMessage deserialization error path - truncated buffer
TEST(DDSTest, Security_generic_message_with_data_deserialize_error)
{
    auto* ts = ParticipantGenericMessageTypeSupport::get_instance();
    ParticipantGenericMessage data;
    memset(&data, 0, sizeof(data));
    
    // Buffer with valid header but truncated messageData
    char buffer[100] = {0};
    buffer[8] = 1;  // messageData count = 1
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrDeserializer des(&sbuf);
    
    int ret = ts->deserialize_data(&data, &des, 0);
    EXPECT_EQ(ret, -1);
}

#endif
