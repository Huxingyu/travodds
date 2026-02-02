#include "gtest/gtest.h"
#include "common/sequence/sequence.h"

TEST(SequenceTest, DefaultConstructor) {
    SequenceData<int> seq;
    EXPECT_EQ(seq.length(), 0);
    EXPECT_EQ(seq.maximum(), 0);
    EXPECT_EQ(seq.buffer(), nullptr);
}


TEST(SequenceTest, LengthSetterAndGetter) {
    SequenceData<int> seq;
    EXPECT_TRUE(seq.length(5));
    EXPECT_EQ(seq.length(), 5);
    EXPECT_FALSE(seq.length(-3));
    EXPECT_EQ(seq.length(), 5);
}

TEST(SequenceTest, ConstructorWithMaxAndAccess) {
    SequenceData<int> seq(3);
    EXPECT_EQ(seq.length(), 3);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(seq[i], 0);
    }
    seq[1] = 42;
    EXPECT_EQ(seq[1], 42);
}

TEST(SequenceTest, PushBackIncreasesLengthAndStoresValue) {
    SequenceData<int> seq;
    int v = 7;
    seq.push_back(v);
    EXPECT_EQ(seq.length(), 1);
    EXPECT_EQ(seq[0], 7);
}

TEST(SequenceTest, CopyConstructorAndAssignmentDeepCopy) {
    SequenceData<int> a(2);
    a[0] = 5;
    a[1] = 6;
    SequenceData<int> b = a;
    EXPECT_EQ(b.length(), 2);
    EXPECT_EQ(b[0], 5);
    EXPECT_EQ(b[1], 6);

    SequenceData<int> c;
    c = a; 
    EXPECT_EQ(c.length(), 2);
    EXPECT_EQ(c[0], 5);
    EXPECT_EQ(c[1], 6);

    a[0] = 100;
    EXPECT_EQ(b[0], 5);
    EXPECT_EQ(c[0], 5);
}

TEST(SequenceTest, OutOfRangeAccessThrows) {
    SequenceData<int> seq(1);
    EXPECT_THROW(seq[1], std::out_of_range);
}

TEST(SequenceBaseTest, BufferAndIndexing) {
    SequenceData<int> seq;
    int vl[3] = {10,20,30};
	seq.push_back(vl[0]);
	seq.push_back(vl[1]);
	seq.push_back(vl[2]);
    EXPECT_EQ(seq.maximum(), 3);
    void** buf = seq.buffer();
    ASSERT_NE(buf, nullptr);
    SequenceBase& baseRef = seq;

    for (uint32_t i = 0; i < 3; ++i) {
        void* v = buf[i];
        ASSERT_NE(v, nullptr);
        int* ip = static_cast<int*>(v);
        EXPECT_EQ(*ip, vl[i]);
        EXPECT_EQ(v, baseRef[i]);
    }
}


TEST(SequenceTest, UnloanReleasesAndAllowsReuse) {
    SequenceData<int> seq;
    int v = 9;
    seq.push_back(v);
    EXPECT_EQ(seq.length(), 1);
    EXPECT_NE(seq.buffer(), nullptr);
    seq.unloan();
    EXPECT_EQ(seq.buffer(), nullptr);
    EXPECT_EQ(seq.length(), 0);
    EXPECT_EQ(seq.maximum(), 0);

    seq.push_back(v);
    EXPECT_EQ(seq.length(), 1);
    EXPECT_EQ(seq[0], 9);
}
