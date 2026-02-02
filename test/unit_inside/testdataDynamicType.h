#ifndef testdataDynamicType_H
#define testdataDynamicType_H

#include "xtypes/dynamictype.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace _DynamicType {

namespace TestModule {

extern const TRAVODDS::DynamicType* BasicData;

extern const TRAVODDS::DynamicType* KeyData;

extern const TRAVODDS::DynamicType* SequenceData;

extern const TRAVODDS::DynamicType* TestEnum;

extern const TRAVODDS::DynamicType* NestedStruct;

extern const TRAVODDS::DynamicType* Matrix4x4;

extern const TRAVODDS::DynamicType* FixedString;

extern const TRAVODDS::DynamicType* Measurements;

extern const TRAVODDS::DynamicType* ShortBuffer;

extern const TRAVODDS::DynamicType* CharBuffer;

extern const TRAVODDS::DynamicType* ByteBuffer;

extern const TRAVODDS::DynamicType* FilterableData;

} // namespace TestModule

} // namespace _DynamicType

#ifdef __cplusplus
}
#endif
#endif /* testdataDynamicTypeDynamicType_H */
