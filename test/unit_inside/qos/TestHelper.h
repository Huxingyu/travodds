#ifndef testhelper_h__
#define testhelper_h__

#include "qos/qos.h"
#include "dcps/publish/datawriter.h"
#include "dcps/subscribe/datareader.h"
#include "gtest/gtest.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

class TestHelper {
public:

    static bool waitmatch(DataWriter* writer, unsigned int count, int timeout = 10);

    static bool waitmatch(DataReader* reader, unsigned int count, int timeout = 10);
};

#endif // testhelper_h__