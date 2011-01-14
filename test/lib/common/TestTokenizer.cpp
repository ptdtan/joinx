#include "common/Tokenizer.hpp"

#include <stdexcept>
#include <string>
#include <gtest/gtest.h>

using namespace std;


TEST(TestTokenizer, extract) {
    string input("123\tnot\t-456");
    uint32_t unsignedValue;
    int32_t signedValue;
    string stringValue;

    Tokenizer t(input);
    ASSERT_FALSE(t.eof());

    ASSERT_TRUE(t.extract(unsignedValue));
    ASSERT_EQ(123u, unsignedValue);

    ASSERT_FALSE(t.extract(unsignedValue));
    ASSERT_FALSE(t.extract(signedValue));

    ASSERT_TRUE(t.extract(stringValue));
    ASSERT_EQ("not", stringValue);

    ASSERT_TRUE(t.extract(signedValue));
    ASSERT_EQ(-456, signedValue);
}

TEST(TestTokenizer, rewind) {
    string input("1\t2\t3");
    int n;

    Tokenizer t(input);
    ASSERT_FALSE(t.eof());

    for (int i = 1; i <=3; ++i) {
        ASSERT_TRUE(t.extract(n));
        ASSERT_EQ(i, n);
    }

    t.rewind();

    for (int i = 1; i <=3; ++i) {
        ASSERT_TRUE(t.extract(n));
        ASSERT_EQ(i, n);
    }
}

TEST(TestTokenizer, advance) {
    string input("1\t2\t3\t4\tfive");
    int n;
    string s;

    Tokenizer t(input);
    ASSERT_FALSE(t.eof());

    ASSERT_TRUE(t.advance());
    ASSERT_TRUE(t.extract(n));
    ASSERT_EQ(2, n);

    ASSERT_EQ(2, t.advance(2));
    ASSERT_TRUE(t.extract(s));
    ASSERT_EQ("five", s);
    ASSERT_TRUE(t.eof());
}

TEST(TestTokenizer, nullFields) {
    string input(",1,,3,");
    string s;


    Tokenizer t(input, ',');
    ASSERT_FALSE(t.eof());

    string expected[5] = { "", "1", "", "3", "" };
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(t.extract(s));
        ASSERT_EQ(expected[i], s);
        ASSERT_FALSE(t.eof());
    }
    t.extract(s);
    ASSERT_EQ("", s);
    ASSERT_TRUE(t.eof());
}
