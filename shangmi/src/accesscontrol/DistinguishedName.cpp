#include "travodds_dll.h"
#include <cstring>
#include "common/log/logger.h"
#include "accesscontrol/DistinguishedName.h"

USING_TRAVODDS_NAMESPACE

Attribute::Attribute(
    const DistinguishedName& name)
: value(name.c_str())
, length(name.size())
{
}

Attribute::Attribute(
    const char* name)
: value(name)
{
size_t i = 0;
while (name[i] != 0)
{
    ++i;
}
length = i;
}

bool Attribute::is_set() const noexcept
{
return length > 0;
}

size_t Attribute::cut(
    size_t ini,
    size_t fin /*= 0*/) noexcept
{
    // 如果截取范围超出实际大小，则清空并返回
    if (ini >= length || fin > length)
    {
        return clear();
    }

    // 如果 fin 为 0，表示截取到末尾
    if (fin == 0)
    {
        value += ini;
        length -= ini;
        return ini;
    }

    // 如果 fin 小于等于 ini，范围无效，则清空
    if (fin <= ini)
    {
        return clear();
    }

    // 否则，执行截取
    value += ini;
    size_t aux = length;
    length = fin - ini;
    return aux - length;
}

size_t Attribute::trim_blank_spaces()
{
    size_t i = 0;
    while (i < length && value[i] == ' ')
    {
        i++;
    }
    return cut(i);
}

size_t Attribute::trim_back_blank_spaces()
{
    size_t i = length;
    // 注意：使用 i > 0 而不是 i >= 0，因为 size_t 是无符号的
    while (i > 0 && value[i - 1] == ' ')
    {
        i--;
    }
    return cut(0, i);
}

size_t Attribute::clear() noexcept
{
    value = nullptr;
    size_t aux = length;
    length = 0;
    return aux;
}

size_t Attribute::find(
    char c) const noexcept
{
    size_t i = 0;
    bool in_quotes = false;
    while (i < length)
    {
        if (!in_quotes && value[i] == c)
        {
            return i;
        }

        if (value[i] == '"')
        {
            in_quotes = !in_quotes;
        }
        else if (value[i] == '\\')
        {
            i++;
        }

        i++;
    }
    return i;
}

size_t Attribute::find(
    char c,
    bool& result) const noexcept
{
    auto res = find(c);
    result = found(res);
    return res;
}

bool Attribute::found(
    const size_t& find_result) const noexcept
{
    return find_result < length;
}

bool Attribute::operator ==(
    const Attribute& other) const noexcept
{
    // 逐字符比较
    // 需要考虑'\'转义字符，并忽略'"'引号
    size_t index_this = 0;
    size_t index_other = 0;
    while (true)
    {
        while (
            index_this < this->length
            && (this->value[index_this] == '\\' || this->value[index_this] == '"'))
        {
            index_this++;
        }

        while (
            index_other < other.length
            && (other.value[index_other] == '\\' || other.value[index_other] == '"'))
        {
            index_other++;
        }

        if (index_this == this->length && index_other == other.length)
        {
            return true;
        }

        if (
            !(index_this < this->length)
            || !(index_other < other.length)
            || (this->value[index_this] != other.value[index_other]))
        {
            return false;
        }
        else
        {
            index_this++;
            index_other++;
        }
    }
}

bool Attribute::operator !=(
    const Attribute& other) const noexcept
{
    return !operator ==(other);
}

Attribute Attribute::cut(
    const Attribute& att,
    size_t ini,
    size_t fin /*= 0*/) noexcept
{
    Attribute res {att};
    res.cut(ini, fin);
    return res;
}

std::ostream& operator <<(
    std::ostream& os,
    const Attribute& att)
{
    os.write(att.value, att.length);
    return os;
}

bool DistinguishedNameSpecialized::compare(
    const DistinguishedName& name1,
    const DistinguishedName& name2) noexcept
{
    return DistinguishedNameSpecialized(name1) == DistinguishedNameSpecialized(name2);
}

DistinguishedNameSpecialized::DistinguishedNameSpecialized(
    const DistinguishedName& name) noexcept
{
    // 通过递归函数进行构造
    auto res = find_and_add_type_values(name);

    switch (res)
    {
        case ErrorCase::MAX_VALUE:
            REC_LOG_ERROR(
                SECURITY,
                "DistinguishedName " << name << " 拥有的键值对属性超过了允许的最大数量。");
            break;

        case ErrorCase::EMPTY:
            REC_LOG_ERROR(
                SECURITY,
                "DistinguishedName " << name << " 包含一个空字段。");
            break;

        case ErrorCase::NO_TYPE_VALUE_FORMAT:
            REC_LOG_ERROR(
                SECURITY,
                "DistinguishedName " << name << " 格式不正确。");
            break;

        default:
            break;
    }
}

DistinguishedNameSpecialized::ErrorCase DistinguishedNameSpecialized::find_and_add_type_values(
    const Attribute& input) noexcept
{
    Attribute rest = input;
    rest.trim_blank_spaces();
    // 如果输入已处理完毕，则停止
    if (!rest.is_set())
    {
        return ErrorCase::EMPTY;
    }

    // 如果此对象已满，则失败（遵循快速策略，不抛出异常）
    if (size() >= MAX_VALUES_)
    {
        return ErrorCase::MAX_VALUE;
    }

    //////////////////////////////////////////
    // 变量定义
    bool found = false;
    size_t index = 0;

    //////////////////////////////////////////
    // 查找第一个 '='
    index = rest.find('=', found);
    if (!found)
    {
        return ErrorCase::NO_TYPE_VALUE_FORMAT;
    }
    Attribute type = Attribute::cut(rest, 0, index);
    rest.cut(index + 1);

    //////////////////////////////////////////
    // 查找第一个 ','
    index = rest.find(',', found);
    if (!found)
    {
        // 找到最后一个元素，添加并结束
        add_type_values(type, rest);
        return ErrorCase::OK;
    }

    // 如果不是最后一个，则继续
    Attribute value = Attribute::cut(rest, 0, index);
    rest.cut(index + 1);

    // 添加键值对
    add_type_values(type, value);

    //////////////////////////////////////////
    // 继续添加键值对
    return find_and_add_type_values(rest);
}

void DistinguishedNameSpecialized::add_type_values(
    const Attribute& type,
    const Attribute& value) noexcept
{
    // 注意：先存储再修剪空格，可以避免一次拷贝

    //////////////////////////////////////////
    // 存储新的键和值
    types_[size_] = type;
    values_[size_] = value;

    //////////////////////////////////////////
    // 修剪首尾空格
    types_[size_].trim_blank_spaces();
    types_[size_].trim_back_blank_spaces();
    values_[size_].trim_blank_spaces();
    values_[size_].trim_back_blank_spaces();

    size_++;
}

size_t DistinguishedNameSpecialized::size() const noexcept
{
    return size_;
}

DistinguishedNameSpecialized::Value DistinguishedNameSpecialized::get_attribute(
    const Type& type) const noexcept
{
    for (size_t i = 0; i < size_; ++i)
    {
        if (!types_[i].is_set())
        {
            // 如果没有更多类型，则结束
            break;
        }
        else if (types_[i] == type)
        {
            return values_[i];
        }
    }
    return Value();
}

bool DistinguishedNameSpecialized::operator ==(
    const DistinguishedNameSpecialized& other) const noexcept
{
    if (this->size() != other.size())
    {
        return false;
    }

    for (size_t i = 0; i < size_; ++i)
    {
        if (other.get_attribute(this->types_[i]) != this->values_[i])
        {
            return false;
        }
    }

    // 如果大小相同且 A 中所有键都在 B 中，则 B 中所有键也都在 A 中
    return true;
}

bool rfc2253_string_compare(
    const DistinguishedName& name1,
    const DistinguishedName& name2)
{
    return DistinguishedNameSpecialized::compare(name1, name2);
}