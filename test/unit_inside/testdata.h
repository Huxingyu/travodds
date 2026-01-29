#ifndef testdata_H
#define testdata_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "cdr/travoddscdr.h"

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-compare"

#if defined(DDS_TYPE_EXPORT) && defined(_WIN32)
#define DDS_TYPE_API __declspec(dllexport)
#else
#define DDS_TYPE_API 
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace TestModule {

class DDS_TYPE_API BasicData {
public:
    bool operator<(const BasicData& rhs) const {
        return false;
    }
    int id;
    std::string name;
};

class DDS_TYPE_API KeyData {
public:
    bool operator<(const KeyData& rhs) const {
        return false;
    }
    int userId;// @key 
    std::string data;
};

class DDS_TYPE_API SequenceData {
public:
    bool operator<(const SequenceData& rhs) const {
        return false;
    }
    std::vector<int> numbers;
};

enum TestEnum {
    VALUE_A,
    VALUE_B,
    VALUE_C,
    VALUE_D,
    VALUE_E,
};

class DDS_TYPE_API NestedStruct {
public:
    bool operator<(const NestedStruct& rhs) const {
        return false;
    }
    int nestedId;
    double nestedValue;
    char nestedFlag;
    std::string nestedName;
    int nestedArray[2];
};

typedef int Matrix4x4[4][4];

typedef std::string FixedString;

typedef double Measurements[10];

typedef short ShortBuffer[8];

typedef char CharBuffer[32];

typedef unsigned char ByteBuffer[64];

class DDS_TYPE_API FilterableData {
public:
    bool operator<(const FilterableData& rhs) const {
        return false;
    }
    short shortField;
    unsigned short ushortField;
    int longField;
    unsigned int ulongField;
    long long longlongField;
    unsigned long long ulonglongField;
    float floatField;
    double doubleField;
    char boolField;
    char charField;
    unsigned char byteField;
    std::string stringField;
    TestModule::FixedString fixedStringField;
    TestModule::TestEnum enumField;
    TestModule::NestedStruct nestedField;
    int longArray[10];
    double doubleArray[5];
    char boolArray[3];
    std::string stringArray[4];
    int matrix2x3[2][3];
    float floatMatrix[3][3];
    TestModule::Matrix4x4 matrix4x4;
    std::vector<int> seqLongField;
    std::vector<std::string> seqStringField;
    TestModule::ShortBuffer shortBuffer;
    TestModule::CharBuffer charBuffer;
    TestModule::ByteBuffer byteBuffer;
    TestModule::Measurements measurements;
    TestModule::TestEnum enumArray[5];
    TestModule::NestedStruct structArray[3];
};

} // namespace TestModule

#ifdef __cplusplus
}
#endif
#endif /* testdata_H */
