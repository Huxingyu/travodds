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

#ifndef DYNAMIC_DATA_H
#define DYNAMIC_DATA_H

#include "agidds_dll.h"
#include "xtypes/dynamictype.h"
#include "xtypes/memberdescriptor.h"
#include "type/basictype.h"
#include <string>
#include <wchar.h>
#include <map>
#include <vector>

AGIDDS_NAMESPACE_BEGIN

typedef std::vector<class DynamicData*> DynamicDataSeq;

/**
 * @brief DynamicData类的每个对象表示由DynamicData对象的DynamicType表示的类型的相应对象。
 * 
 * @details DynamicData对象具有引用语义；然而，有一个equals操作允许它们进行深度比较。
 */
class DynamicData {
public:
    /**
     * @brief 构造函数
     * 
     * @param type DynamicType对象
     */
    DynamicData(DynamicType& type);

    /**
     * @brief 拷贝构造函数
     * 
     * @param other 另一个DynamicData对象   
     */

    DynamicData(const DynamicData& other);

    /**
     * @brief 复制构造函数
     * 
     * @param other 另一个DynamicData对象
     */

    DynamicData& operator=(const DynamicData& other);

    /**
     * @brief 析构函数
     */ 
    ~DynamicData();

    /**
     * @brief 判断两个DynamicData对象是否相等
     * 
     * @param other 另一个DynamicData对象
     * @return bool 相等返回true，否则返回false
     */
    bool equals(const DynamicData& other) const;

    /**
     * @brief 获取成员ID通过名称
     * 
     * @param name 成员名称
     * @return MemberId 成员ID
     */
    MemberId get_member_id_by_name(const std::string& name) const;

    /**
     * @brief 获取成员ID通过索引
     * 
     * @param index 成员索引
     * @return MemberId 成员ID
     */
    MemberId get_member_id_at_index(size_t index) const;

    /**
     * @brief 清除成员值
     * 
     * @param id 成员ID
     */
    void clear_value(MemberId id);

    /**
     * @brief 清除所有成员值
     */
    void clear_all_values();

    /**
     * @brief 清除非关键成员值
     */
    void clear_nonkey_values();

    /**
     * @brief 克隆数据样本
     * 
     * @return DynamicData 克隆的数据样本
     */
    DynamicData* clone() const;

    /**
     * @brief 获取项目数量
     * 
     * @return size_t 项目数量
     */
    size_t get_item_count() const;

    /**
     * @brief 获取项目数量
     * 
     * @return size_t 项目数量
     */
    size_t get_memberid_item_count(MemberId id) const;

    /**
     * @brief 借用值
     * 
     * @param id 成员ID
     * @return DynamicData 借用的值
     */
    DynamicData* loan_value(MemberId id);

    /**
     * @brief 归还借用的值
     * 
     * @param data 借用的值
     * @return int 成功返回0，失败返回RETCODE_PRECONDITION_NOT_MET
     */
    int return_loaned_value(DynamicData& data);

    /**
     * @brief 获取类型
     * 
     * @return DynamicType 类型
     */
    DynamicType& get_type() const;

    typedef std::vector<long>                   Int32Seq;    
    typedef std::vector<unsigned long>          UInt32Seq;    
    typedef std::vector<short>                  Int16Seq;    
    typedef std::vector<unsigned short>         UInt16Seq;    
    typedef std::vector<long long>              Int64Seq;    
    typedef std::vector<unsigned long long>     UInt64Seq;    
    typedef std::vector<float>                  Float32Seq;    
    typedef std::vector<double>                 Float64Seq;    
    typedef std::vector<long double>            Float128Seq;    
    typedef std::vector<char>                   CharSeq;    
    typedef std::vector<wchar_t>                WcharSeq;    
    typedef std::vector<char>                   BooleanSeq;    
    typedef std::vector<octet>                  ByteSeq;
 
    typedef std::vector<std::string>            StringSeq;    
    typedef std::vector<std::wstring>           WstringSeq;


    class TypeValue {
    public:
        TypeValue() {
            boolean_value = false;
            byte_value = 0;
            int16_value = 0;
            uint16_value = 0;
            int32_value = 0;
            uint32_value = 0;
            int64_value = 0;
            uint64_value = 0;
            float32_value = 0.0f;
            float64_value = 0.0;
            float128_value = 0.0L;
            charvalue = '\0';
            wchar_value = L'\0';
            complex_value = nullptr;
        }
        TypeValue(const TypeValue& other) {
            boolean_value = other.boolean_value;
            byte_value = other.byte_value;
            int16_value = other.int16_value;
            uint16_value = other.uint16_value;
            int32_value = other.int32_value;
            uint32_value = other.uint32_value;
            int64_value = other.int64_value;
            uint64_value = other.uint64_value;
            float32_value = other.float32_value;
            float64_value = other.float64_value;
            float128_value = other.float128_value;
            charvalue = other.charvalue;
            wchar_value = other.wchar_value;
            string_value = other.string_value;
            wstring_value = other.wstring_value;
            int32_values = other.int32_values;
            uint32_values = other.uint32_values;
            int16_values = other.int16_values;
            uint16_values = other.uint16_values;
            int64_values = other.int64_values;
            uint64_values = other.uint64_values;
            float32_values = other.float32_values;
            float64_values = other.float64_values;
            float128_values = other.float128_values;
            boolean_values = other.boolean_values;
            char8_values = other.char8_values;
            char16_values = other.char16_values;
            byte_values = other.byte_values;
            string_values = other.string_values;
            wstring_values = other.wstring_values;
            complex_value = (other.complex_value ? new DynamicData(*other.complex_value) : nullptr);
            complex_values.reserve(other.complex_values.size());
            for (const auto& value : other.complex_values) {
                complex_values.push_back(new DynamicData(*value));
            }
        }
        TypeValue& operator=(const TypeValue& other) {
            if (this != &other) {
                boolean_value = other.boolean_value;
                byte_value = other.byte_value;
                int16_value = other.int16_value;
                uint16_value = other.uint16_value;
                int32_value = other.int32_value;
                uint32_value = other.uint32_value;
                int64_value = other.int64_value;
                uint64_value = other.uint64_value;
                float32_value = other.float32_value;
                float64_value = other.float64_value;
                float128_value = other.float128_value;
                charvalue = other.charvalue;
                wchar_value = other.wchar_value;
                string_value = other.string_value;
                wstring_value = other.wstring_value;
                int32_values = other.int32_values;
                uint32_values = other.uint32_values;
                int16_values = other.int16_values;
                uint16_values = other.uint16_values;
                int64_values = other.int64_values;
                uint64_values = other.uint64_values;
                float32_values = other.float32_values;
                float64_values = other.float64_values;
                float128_values = other.float128_values;
                boolean_values = other.boolean_values;
                char8_values = other.char8_values;
                char16_values = other.char16_values;
                byte_values = other.byte_values;
                string_values = other.string_values;
                wstring_values = other.wstring_values;

                if (complex_value) {
                    delete complex_value; 
                    complex_value = nullptr; 
                }
                
                if (other.complex_value) {
                    complex_value = new DynamicData(*other.complex_value);
                }

                // 清除之前的复杂类型值
                for (auto& value : complex_values) {
                    delete value;
                }
                complex_values.clear();
                complex_values.reserve(other.complex_values.size());
                
                for (const auto& value : other.complex_values) {
                    complex_values.push_back(new DynamicData(*value));
                }
            }
            return *this;
        }
        ~TypeValue() {
            if (complex_value) {
                delete complex_value; 
                complex_value = nullptr; 
            }
            for (auto& value : complex_values) {
                delete value;
            }
            complex_values.clear();
        }
    public:
        // 基本数据类型的值
        bool boolean_value;
        octet byte_value;
        short int16_value;
        unsigned short uint16_value;
        long int32_value;
        unsigned long uint32_value;
        long long int64_value;
        unsigned long long uint64_value;
        float float32_value;
        double float64_value;
        long double float128_value;
        char charvalue;
        wchar_t wchar_value;
        // 字符串的值
        std::string string_value;
        std::wstring wstring_value;
        // sequence以及数组的值
        Int32Seq int32_values;
        UInt32Seq uint32_values;
        Int16Seq int16_values;
        UInt16Seq uint16_values;
        Int64Seq int64_values;
        UInt64Seq uint64_values;
        Float32Seq float32_values;
        Float64Seq float64_values;
        Float128Seq float128_values;
        BooleanSeq boolean_values;
        CharSeq char8_values;
        WcharSeq char16_values;
        ByteSeq byte_values;
        StringSeq string_values;
        WstringSeq wstring_values;
        // 复杂类型的值
        DynamicData* complex_value;
        DynamicDataSeq complex_values;
    };

    ReturnCode_t get_int32_value(long& value, MemberId id);
    ReturnCode_t set_int32_value(MemberId id, long value);
    ReturnCode_t get_uint32_value(unsigned long& value, MemberId id);
    ReturnCode_t set_uint32_value(MemberId id, unsigned long value);
    ReturnCode_t get_int16_value(short& value, MemberId id);
    ReturnCode_t set_int16_value(MemberId id, short value);
    ReturnCode_t get_uint16_value(unsigned short& value, MemberId id);
    ReturnCode_t set_uint16_value(MemberId id, unsigned short value);
    ReturnCode_t get_int64_value(long long& value, MemberId id);
    ReturnCode_t set_int64_value(MemberId id, long long value);
    ReturnCode_t get_uint64_value(unsigned long long& value, MemberId id);
    ReturnCode_t set_uint64_value(MemberId id, unsigned long long value);
    ReturnCode_t get_float32_value(float& value, MemberId id);
    ReturnCode_t set_float32_value(MemberId id, float value);
    ReturnCode_t get_float64_value(double& value, MemberId id);
    ReturnCode_t set_float64_value(MemberId id, double value);
    ReturnCode_t get_float128_value(long double& value, MemberId id);
    ReturnCode_t set_float128_value(MemberId id, long double value);
    ReturnCode_t get_char8_value(char& value, MemberId id);
    ReturnCode_t set_char8_value(MemberId id, char value);
    ReturnCode_t get_char16_value(wchar_t& value, MemberId id);
    ReturnCode_t set_char16_value(MemberId id, wchar_t value);
    ReturnCode_t get_byte_value(octet& value, MemberId id);
    ReturnCode_t set_byte_value(MemberId id, octet value);
    ReturnCode_t get_boolean_value(bool& value, MemberId id);
    ReturnCode_t set_boolean_value(MemberId id, bool value);
    ReturnCode_t get_string_value(std::string& value, MemberId id);
    ReturnCode_t set_string_value(MemberId id, const std::string& value);
    ReturnCode_t get_wstring_value(std::wstring& value, MemberId id);
    ReturnCode_t set_wstring_value(MemberId id, const std::wstring& value);
    ReturnCode_t get_complex_value(DynamicData& value, MemberId id);
    ReturnCode_t set_complex_value(MemberId id, const DynamicData& value);
    ReturnCode_t get_complex_values(DynamicDataSeq& value, MemberId id);
    ReturnCode_t set_complex_values(MemberId id, const DynamicDataSeq& value);
    ReturnCode_t get_int32_values(Int32Seq& value, MemberId id);
    ReturnCode_t set_int32_values(MemberId id, const Int32Seq& value);
    ReturnCode_t get_uint32_values(UInt32Seq& value, MemberId id);
    ReturnCode_t set_uint32_values(MemberId id, const UInt32Seq& value);
    ReturnCode_t get_int16_values(Int16Seq& value, MemberId id);
    ReturnCode_t set_int16_values(MemberId id, const Int16Seq& value);
    ReturnCode_t get_uint16_values(UInt16Seq& value, MemberId id);
    ReturnCode_t set_uint16_values(MemberId id, const UInt16Seq& value);
    ReturnCode_t get_int64_values(Int64Seq& value, MemberId id);
    ReturnCode_t set_int64_values(MemberId id, const Int64Seq& value);
    ReturnCode_t get_uint64_values(UInt64Seq& value, MemberId id);
    ReturnCode_t set_uint64_values(MemberId id, const UInt64Seq& value);
    ReturnCode_t get_float32_values(Float32Seq& value, MemberId id);
    ReturnCode_t set_float32_values(MemberId id, const Float32Seq& value);
    ReturnCode_t get_float64_values(Float64Seq& value, MemberId id);
    ReturnCode_t set_float64_values(MemberId id, const Float64Seq& value);
    ReturnCode_t get_float128_values(Float128Seq& value, MemberId id);
    ReturnCode_t set_float128_values(MemberId id, const Float128Seq& value);
    ReturnCode_t get_char8_values(CharSeq& value, MemberId id);
    ReturnCode_t set_char8_values(MemberId id, const CharSeq& value);
    ReturnCode_t get_char16_values(WcharSeq& value, MemberId id);
    ReturnCode_t set_char16_values(MemberId id, const WcharSeq& value);
    ReturnCode_t get_byte_values(ByteSeq& value, MemberId id);
    ReturnCode_t set_byte_values(MemberId id, const ByteSeq& value);
    ReturnCode_t get_boolean_values(BooleanSeq& value, MemberId id);
    ReturnCode_t set_boolean_values(MemberId id, const BooleanSeq& value);
    ReturnCode_t get_string_values(StringSeq& value, MemberId id);
    ReturnCode_t set_string_values(MemberId id, const StringSeq& value);
    ReturnCode_t get_wstring_values(WstringSeq& value, MemberId id);
    ReturnCode_t set_wstring_values(MemberId id, const WstringSeq& value);

private:
    ReturnCode_t initialize_values();

private:
    /** @brief 只读属性，该DynamicData关联的类型。*/

    DynamicType& type_;

    /**
     * @brief 该DynamicData的值。
     * 
     * @details 该类定义的许多属性和操作都涉及样本内的值，这些值通过名称、成员ID或索引进行标识。
     *          样本中的值构成要素及其有效访问方式取决于样本类型：
     * 
     *          - ​聚合类型对象：值对应类型的成员，可通过名称、成员ID或索引访问。
     *          - 序列或字符串类型对象：值对应集合中的元素，必须通过索引访问。索引与成员ID的映射关系未作规定。
     *          - ​映射类型对象：值对应映射中的值。映射键会隐式转换为字符串，因此可通过名称查找；也可通过索引访问，但顺序未作规定。
     *          - 数组类型对象：值对应数组元素，必须通过索引访问。索引与成员ID的映射关系未作规定。多维数组元素按IDL规范指定的顺序"扁平化"为一维数组后访问。
     *          - ​位掩码类型对象：值对应位掩码中的标志（均为布尔类型）。命名标志可通过名称访问；任意有效位可通过索引访问。名称/索引与成员ID的映射关系未作规定。
     *          - ​枚举或基本类型对象：不包含内部值。但样本自身值可通过以下方式表示：用nil或空字符串表示"名称"、用MEMBER_ID_INVALID表示"ID"、用索引0表示"索引"。
     * 
     *          需注意此处索引始终相对于特定DynamicData对象中的其他值。尽管聚合类型的成员定义有明确顺序，
     *          但数据样本内部或跨样本的索引顺序并不固定。具体表现为：聚合类型成员在特定数据样本中的索引可能与其在类型定义中的索引不同，
     *          也可能与其他数据样本中的索引不一致。造成不一致的原因包括：
     *          
     *          - 样本生产者使用的类型版本可能与消费者存在差异（增减成员）
     *          - 可选成员可能无值而被省略，导致后续成员索引前移
     *          - 非可选成员可能被省略（语义上等同于取默认值），实现层可自主决定是否省略（如节省空间）
     *          - 某些数据表示无需（或出于性能考虑不宜）保持成员顺序
     */

    std::map<MemberId, TypeValue> value_;
    /** @brief 对应每个值的的描述。 */
    std::map<MemberId, MemberDescriptor> descriptor_;
};

AGIDDS_NAMESPACE_END

#endif // DYNAMIC_DATA_H
