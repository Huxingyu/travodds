#include "gtest/gtest.h"
#include "dcps/condition/guardcondition.h"

USING_TRAVODDS_NAMESPACE

TEST(GuardConditionTest, GuardCondition_interface) {
	GuardCondition gc;
	EXPECT_FALSE(gc.get_trigger_value());
	EXPECT_EQ(gc.set_trigger_value(true), RETCODE_OK);
	EXPECT_TRUE(gc.get_trigger_value());
	EXPECT_EQ(gc.set_trigger_value(false), RETCODE_OK);
	EXPECT_FALSE(gc.get_trigger_value());
}