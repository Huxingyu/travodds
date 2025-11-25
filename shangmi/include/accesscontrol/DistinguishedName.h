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
#ifndef _SECURITY_ACCESSCONTROL_DISTINGUISHEDNAME_H_
#define _SECURITY_ACCESSCONTROL_DISTINGUISHEDNAME_H_

#include <array>
#include <string>

TRAVODDS_NAMESPACE_BEGIN
using DistinguishedName = std::string;

struct Attribute
{
    Attribute() = default;

    Attribute(
            const DistinguishedName& name);

    Attribute(
            const char* name);

    bool is_set() const noexcept;

    size_t cut(
            size_t ini,
            size_t fin = 0) noexcept;

    size_t trim_blank_spaces();

    size_t trim_back_blank_spaces();

    size_t clear() noexcept;

    size_t find(
            char c) const noexcept;

    size_t find(
            char c,
            bool& result) const noexcept;

    bool found(
            const size_t& find_result) const noexcept;

    bool operator ==(
            const Attribute& other) const noexcept;

    bool operator !=(
            const Attribute& other) const noexcept;

    static Attribute cut(
            const Attribute& att,
            size_t ini,
            size_t fin = 0) noexcept;

    const char* value {nullptr};
    size_t length {0};
};

std::ostream& operator <<(
        std::ostream& os,
        const Attribute& att);

bool rfc2253_string_compare(
    const DistinguishedName& name1,
    const DistinguishedName& name2);

class DistinguishedNameSpecialized
{
public:

    static bool compare(
            const DistinguishedName& name1,
            const DistinguishedName& name2) noexcept;

    enum class ErrorCase
    {
        OK,
        EMPTY,
        MAX_VALUE,
        NO_TYPE_VALUE_FORMAT,
    };

    using Type = Attribute;
    using Value = Attribute;

    DistinguishedNameSpecialized(
            const DistinguishedName& name) noexcept;

    ErrorCase find_and_add_type_values(
            const Attribute& input) noexcept;

    void add_type_values(
            const Attribute& type,
            const Attribute& value) noexcept;

    size_t size() const noexcept;

    Value get_attribute(
            const Type& type) const noexcept;

    bool operator ==(
            const DistinguishedNameSpecialized& other) const noexcept;

protected:

    static constexpr size_t MAX_VALUES_ = 20;

    std::array<Attribute, MAX_VALUES_> types_ {};
    std::array<Attribute, MAX_VALUES_> values_ {};
    bool ok_ {false};
    size_t size_ {0};
};
TRAVODDS_NAMESPACE_END

#endif // _SECURITY_ACCESSCONTROL_DISTINGUISHEDNAME_H_