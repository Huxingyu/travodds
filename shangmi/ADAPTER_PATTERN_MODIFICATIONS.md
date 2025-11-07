# 适配器模式修改方案总结文档

## 文档说明

本文档总结了在 `travodds/shangmi-merge` 目录中应用适配器模式（Adapter Pattern）的所有修改方案。这些修改用于解决基类接口与派生类实现之间的类型不匹配问题，同时保持代码的向后兼容性。

**适用场景**：当需要将 `shangmi-merge` 中的实现类适配到标准基类接口时，参考本文档进行修改。

---

## 一、核心设计原则

### 1.1 适配器模式架构

```
基类接口 (Base Interface)
    ↓
适配器函数 (Adapter Function) - 带 override，匹配基类签名
    ↓
实现函数 (_impl) - 保持原有签名和逻辑
```

### 1.2 异常处理原则

- **适配函数**：负责 `SecurityException*`（基类）和 `SMSecurityException`（内部）之间的转换
  - 创建 `SMSecurityException temp_exception`
  - 调用 `_impl` 函数
  - 使用 `temp_exception.to_base_exception(exception)` 转换回基类

- **实现函数（_impl）**：只使用 `SMSecurityException&`，不进行转换
  - 直接使用 `exception = SMSecurityException(...)` 设置异常
  - **禁止**调用 `to_base_exception`

### 1.3 类型转换原则

- **适配函数**：负责基类类型和内部类型之间的转换
  - `IdentityHandle` ? `SMIdentityHandle`
  - `PermissionsHandle` ? `SMPermissionsHandle`
  - `ParticipantCryptoHandle` ? `SMParticipantCryptoHandle`
  - `SharedSecretHandle` ? `SMSharedSecretHandle` / `SMSecretHandle`
  - `SecurityException*` ? `SMSecurityException&`

- **实现函数**：只使用内部类型，不进行转换

---

## 二、已修改的类和函数

### 2.1 SM4GCMGMAC_KeyFactory

#### 2.1.1 register_local_participant

**适配函数签名**：
```cpp
ParticipantCryptoHandle register_local_participant(
    IdentityHandle participant_identity,
    PermissionsHandle participant_permissions,
    const PropertySeq* participant_properties,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
std::shared_ptr<SMParticipantCryptoHandle> register_local_participant_impl(
    const SMIdentityHandle& participant_identity,
    const SMPermissionsHandle& participant_permissions,
    const PropertySeq& participant_properties,
    const ParticipantSecurityAttributes& participant_security_attributes,
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数接收 `PropertySeq*`，转换为 `PropertySeq&` 传递给 `_impl`
- 适配函数使用默认的 `ParticipantSecurityAttributes`
- 异常转换在适配函数中处理

#### 2.1.2 register_matched_remote_participant

**适配函数签名**：
```cpp
ParticipantCryptoHandle register_matched_remote_participant(
    IdentityHandle remote_participant_identity,
    ParticipantCryptoHandle local_participant_crypto_handle,
    SharedSecretHandle sharesecret,  // 注意：使用 SharedSecretHandle，不是 SMSharedSecretHandle
    PermissionsHandle remote_participant_permissions,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
std::shared_ptr<SMParticipantCryptoHandle> register_matched_remote_participant_impl(
    const SMIdentityHandle& remote_participant_identity,
    const SMParticipantCryptoHandle& local_participant_crypto_handle,
    const SMSecretHandle& sharesecret,  // 注意：使用 SMSecretHandle
    const SMPermissionsHandle& remote_participant_permissions,
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数参数使用 `SharedSecretHandle`（匹配基类接口）
- 适配函数中转换为 `SMSecretHandle` 传递给 `_impl`
- `_impl` 函数使用 `SMSharedSecretHelper::find_data_value(sharesecret, ...)` 访问数据

#### 2.1.3 register_local_datareader

**适配函数签名**：
```cpp
DatareaderCryptoHandle register_local_datareader(
    ParticipantCryptoHandle participant_crypto,
    const PropertySeq* datareader_properties,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
SMDatareaderCryptoHandle* register_local_datareader_impl(
    SMParticipantCryptoHandle& participant_crypto,
    const PropertySeq& datareader_properties,
    const EndpointSecurityAttributes& datareader_security_attributes,
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数使用默认的 `EndpointSecurityAttributes`
- `_impl` 函数中 `PropertySeq` 使用引用，不是指针

#### 2.1.4 register_local_datawriter

**适配函数签名**：
```cpp
DatawriterCryptoHandle register_local_datawriter(
    ParticipantCryptoHandle participant_crypto,
    const PropertySeq* datawriter_properties,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
SMDatawriterCryptoHandle* register_local_datawriter_impl(
    SMParticipantCryptoHandle& participant_crypto,
    const PropertySeq& datawriter_properties,
    const EndpointSecurityAttributes& datawriter_sec_attr,
    SMSecurityException& exception);
```

**关键修改点**：
- 使用 `datawriter_sec_attr.is_submessage_protected` 和 `datawriter_sec_attr.is_payload_protected`
- 不再从 `plugin_attrs` 推断保护属性

#### 2.1.5 register_matched_remote_datareader

**适配函数签名**：
```cpp
DatareaderCryptoHandle register_matched_remote_datareader(
    DatawriterCryptoHandle local_datawriter_crypto_handle,
    ParticipantCryptoHandle remote_participant_crypto,
    SharedSecretHandle shared_secret,  // 注意：使用 SharedSecretHandle
    bool relay_only,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
SMDatareaderCryptoHandle* register_matched_remote_datareader_impl(
    SMDatawriterCryptoHandle& local_datawriter_crypto_handle,
    SMParticipantCryptoHandle& remote_participant_crypto,
    const SMSharedSecretHandle& shared_secret,  // 注意：使用 SMSharedSecretHandle
    const bool relay_only,
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数参数使用 `SharedSecretHandle`，转换为 `SMSharedSecretHandle` 传递给 `_impl`
- `_impl` 函数参数使用 `SMSharedSecretHandle&`（不是 `SMSecretHandle`）

#### 2.1.6 register_matched_remote_datawriter

**适配函数签名**：
```cpp
DatawriterCryptoHandle register_matched_remote_datawriter(
    DatareaderCryptoHandle local_datareader_crypto_handle,
    ParticipantCryptoHandle remote_participant_crypto,
    SharedSecretHandle shared_secret,  // 注意：使用 SharedSecretHandle
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
SMDatawriterCryptoHandle* register_matched_remote_datawriter_impl(
    SMDatareaderCryptoHandle& local_datareader_crypto_handle,
    SMParticipantCryptoHandle& remote_participant_crypto,
    const SMSharedSecretHandle& shared_secret,  // 注意：使用 SMSharedSecretHandle
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数参数使用 `SharedSecretHandle`，转换为 `SMSharedSecretHandle` 传递给 `_impl`
- `_impl` 函数参数使用 `SMSharedSecretHandle&`（不是 `SMSecretHandle`）

#### 2.1.7 unregister_participant

**适配函数签名**：
```cpp
bool unregister_participant(
    ParticipantCryptoHandle participant_crypto_handle,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
bool unregister_participant_impl(
    std::shared_ptr<SMParticipantCryptoHandle>& participant_crypto_handle,
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数需要从 `ParticipantCryptoHandle` 获取 `shared_ptr`
- 使用 `SM4GCMGMAC_ParticipantCryptoHandle::narrow()` 和 `shared_from_this()`

#### 2.1.8 unregister_datawriter

**适配函数签名**：
```cpp
bool unregister_datawriter(
    DatawriterCryptoHandle datawriter_crypto_handle,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
bool unregister_datawriter_impl(
    std::shared_ptr<SMDatawriterCryptoHandle>& datawriter_crypto_handle,
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数需要从 `DatawriterCryptoHandle` 获取 `shared_ptr`
- 类型转换：`SMDatawriterCryptoHandle* sm_handle = reinterpret_cast<SMDatawriterCryptoHandle*>(datawriter_crypto_handle)`

#### 2.1.9 unregister_datareader

**适配函数签名**：
```cpp
bool unregister_datareader(
    DatareaderCryptoHandle datareader_crypto_handle,
    SecurityException* exception) override;
```

**实现函数签名**：
```cpp
bool unregister_datareader_impl(
    std::shared_ptr<SMDatareaderCryptoHandle>& datareader_crypto_handle,
    SMSecurityException& exception);
```

**关键修改点**：
- 适配函数需要从 `DatareaderCryptoHandle` 获取 `shared_ptr`
- 类型转换：`SMDatareaderCryptoHandle* sm_handle = reinterpret_cast<SMDatareaderCryptoHandle*>(datareader_crypto_handle)`

---

## 三、类型转换规则

### 3.1 Handle 类型转换

| 基类类型 | 内部类型 | 转换方法 |
|---------|---------|---------|
| `IdentityHandle` | `SMIdentityHandle&` | `reinterpret_cast<const SMIdentityHandle&>(identity)` |
| `PermissionsHandle` | `SMPermissionsHandle&` | `reinterpret_cast<const SMPermissionsHandle&>(permissions)` |
| `ParticipantCryptoHandle` | `SMParticipantCryptoHandle&` | `reinterpret_cast<SMParticipantCryptoHandle&>(handle)` |
| `DatawriterCryptoHandle` | `SMDatawriterCryptoHandle&` | `reinterpret_cast<SMDatawriterCryptoHandle&>(handle)` |
| `DatareaderCryptoHandle` | `SMDatareaderCryptoHandle&` | `reinterpret_cast<SMDatareaderCryptoHandle&>(handle)` |
| `SharedSecretHandle` | `SMSecretHandle&` 或 `SMSharedSecretHandle&` | 见下方说明 |

### 3.2 SharedSecretHandle 转换规则

**重要规则**：
- `register_matched_remote_participant`：`SharedSecretHandle` → `SMSecretHandle`（因为需要使用 `SMSharedSecretHelper::find_data_value`）
- `register_matched_remote_datareader`：`SharedSecretHandle` → `SMSharedSecretHandle`
- `register_matched_remote_datawriter`：`SharedSecretHandle` → `SMSharedSecretHandle`

**转换代码**：
```cpp
// 对于 register_matched_remote_participant
const SMSecretHandle& sm_secret = reinterpret_cast<const SMSecretHandle&>(sharesecret);

// 对于 register_matched_remote_datareader/datawriter
const SMSharedSecretHandle& sm_shared_secret = reinterpret_cast<const SMSharedSecretHandle&>(shared_secret);
```

### 3.3 HandleImpl 的 narrow 调用

**重要**：`HandleImpl::narrow()` 的参数应该是引用，不是解引用：

```cpp
// 正确
SM4GCMGMAC_ParticipantCryptoHandle& handle = 
    SM4GCMGMAC_ParticipantCryptoHandle::narrow(participant_crypto);

// 错误
SM4GCMGMAC_ParticipantCryptoHandle& handle = 
    SM4GCMGMAC_ParticipantCryptoHandle::narrow(*participant_crypto);  // 不要解引用
```

### 3.4 shared_from_this() 调用

**重要**：`shared_from_this()` 应该从 `HandleImpl` 对象调用，不是从指针：

```cpp
// 正确
std::shared_ptr<SMParticipantCryptoHandle> sp = handle.shared_from_this();

// 错误
std::shared_ptr<SMParticipantCryptoHandle> sp = handle->shared_from_this();  // 不要用 ->
```

### 3.5 PropertySeq 使用

**重要**：在 `_impl` 函数中，`PropertySeq` 使用引用，不是指针：

```cpp
// 正确
if (!participant_properties.empty())
{
    for (auto it = participant_properties.begin(); it != participant_properties.end(); ++it)
    {
        // ...
    }
}

// 错误
if (participant_properties)  // 不要检查指针
{
    for (auto it = participant_properties->begin(); it != participant_properties->end(); ++it)
    {
        // ...
    }
}
```

---

## 四、异常处理模板

### 4.1 适配函数异常处理模板

```cpp
ReturnType AdapterFunction(
    ParamType1 param1,
    ParamType2 param2,
    SecurityException* exception)  // 基类接口
{
    // 1. 参数检查
    if (param1 == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return ERROR_VALUE;
    }

    // 2. 类型转换
    const SMType1& sm_param1 = reinterpret_cast<const SMType1&>(param1);
    const SMType2& sm_param2 = reinterpret_cast<const SMType2&>(param2);
    
    // 3. 创建临时异常
    SMSecurityException temp_exception;
    
    // 4. 调用实现函数
    SMReturnType result = impl_function(sm_param1, sm_param2, temp_exception);
    
    // 5. 转换异常并返回
    if (result == nullptr || result == ERROR_VALUE)
    {
        temp_exception.to_base_exception(exception);
        return ERROR_VALUE;
    }
    
    // 6. 转换返回值
    return reinterpret_cast<ReturnType>(result);
}
```

### 4.2 实现函数异常处理模板

```cpp
SMReturnType ImplFunction(
    const SMType1& param1,
    const SMType2& param2,
    SMSecurityException& exception)  // 内部类型
{
    // 1. 参数检查
    if (param1.nil() || param2.nil())
    {
        exception = SMSecurityException("Invalid parameter");  // 直接赋值，不转换
        return nullptr;
    }

    // 2. 业务逻辑
    try
    {
        // ... 业务逻辑 ...
    }
    catch (const std::exception& e)
    {
        exception = SMSecurityException(e.what());  // 直接赋值
        return nullptr;
    }

    // 3. 成功返回
    return result;
}
```

---

## 五、常见问题和解决方案

### 5.1 问题：`narrow()` 调用失败

**症状**：编译错误或运行时错误

**原因**：对指针解引用后再调用 `narrow()`

**解决**：
```cpp
// 错误
SM4GCMGMAC_ParticipantCryptoHandle& handle = 
    SM4GCMGMAC_ParticipantCryptoHandle::narrow(*participant_crypto);

// 正确
SM4GCMGMAC_ParticipantCryptoHandle& handle = 
    SM4GCMGMAC_ParticipantCryptoHandle::narrow(participant_crypto);
```

### 5.2 问题：`shared_from_this()` 调用失败

**症状**：编译错误或运行时 `bad_weak_ptr` 异常

**原因**：从指针调用 `shared_from_this()`

**解决**：
```cpp
// 错误
std::shared_ptr<SMParticipantCryptoHandle> sp = handle->shared_from_this();

// 正确
std::shared_ptr<SMParticipantCryptoHandle> sp = handle.shared_from_this();
```

### 5.3 问题：PropertySeq 使用错误

**症状**：编译错误或运行时错误

**原因**：在 `_impl` 函数中将引用当作指针使用

**解决**：
```cpp
// 错误
if (participant_properties)
{
    for (auto it = participant_properties->begin(); ...)
}

// 正确
if (!participant_properties.empty())
{
    for (auto it = participant_properties.begin(); ...)
}
```

### 5.4 问题：异常转换在实现函数中

**症状**：编译错误或逻辑错误

**原因**：在 `_impl` 函数中调用 `to_base_exception`

**解决**：
```cpp
// 错误
if (error)
{
    SMSecurityException sm_ex("Error");
    sm_ex.to_base_exception(exception);  // 不要在 _impl 中转换
    return nullptr;
}

// 正确
if (error)
{
    exception = SMSecurityException("Error");  // 直接赋值
    return nullptr;
}
```

### 5.5 问题：SharedSecretHandle 类型混淆

**症状**：编译错误或运行时错误

**原因**：混淆了 `SMSecretHandle` 和 `SMSharedSecretHandle`

**解决**：
- `register_matched_remote_participant_impl` 使用 `SMSecretHandle`
- `register_matched_remote_datareader_impl` 和 `register_matched_remote_datawriter_impl` 使用 `SMSharedSecretHandle`

---

## 六、检查清单

在应用适配器模式时，请检查以下项目：

### 6.1 适配函数检查

- [ ] 函数签名匹配基类接口（包括 `override` 关键字）
- [ ] 参数类型使用基类类型（如 `SecurityException*`）
- [ ] 创建 `SMSecurityException temp_exception`
- [ ] 调用 `_impl` 函数后使用 `temp_exception.to_base_exception(exception)`
- [ ] 返回值类型转换正确

### 6.2 实现函数检查

- [ ] 函数名以 `_impl` 结尾
- [ ] 参数类型使用内部类型（如 `SMSecurityException&`）
- [ ] 异常设置使用 `exception = SMSecurityException(...)`
- [ ] **没有**调用 `to_base_exception`
- [ ] `PropertySeq` 使用引用，不是指针
- [ ] `narrow()` 调用参数正确（不解引用）
- [ ] `shared_from_this()` 调用正确（不从指针调用）

### 6.3 类型转换检查

- [ ] `SharedSecretHandle` 转换规则正确
- [ ] Handle 类型转换使用 `reinterpret_cast`
- [ ] 返回值类型转换正确

---

## 七、相关文件

### 7.1 已修改的文件

- `travodds/shangmi-merge/include/cryptography/SM4GCMGMAC_KeyFactory.h`
- `travodds/shangmi-merge/src/cryptography/SM4GCMGMAC_KeyFactory.cpp`

### 7.2 参考文件

- `travodds/include/security/cryptokeyfactory.h` - 基类接口定义
- `travodds/shangmi-merge/include/SharedSecretHandle.h` - SharedSecret 类型定义
- `travodds/shangmi-merge/include/Handle.h` - Handle 类型定义
- `travodds/shangmi-merge/include/SecurityException.h` - SMSecurityException 定义

---

## 八、后续工作

### 8.1 需要应用相同模式的类

- `SM4GCMGMAC_KeyExchange` - 部分已完成
- `SM4GCMGMAC_Transform` - 部分已完成
- `PKIGM` - 部分已完成
- `Permissions` - 部分已完成

### 8.2 注意事项

- 每个类可能有不同的类型转换规则
- 某些函数可能不需要适配（如果签名已匹配）
- 保持实现函数的原有逻辑不变

---

## 九、版本信息

- **文档版本**：1.0
- **创建日期**：2025-01-XX
- **最后更新**：2025-01-XX
- **适用代码库**：travodds-build/travodds/shangmi-merge

---

## 十、联系方式

如有问题或需要更新文档，请联系开发团队。



