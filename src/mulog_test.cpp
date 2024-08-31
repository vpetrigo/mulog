/**
 * \file
 * \brief mulog library tests
 * \author Vladimir Petrigo
 */
#include "mulog.h"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>
#include <CppUTestExt/MockSupport.h>

#include <array>
#include <iostream>

namespace {
    void test_output(const char *buf)
    {
        mock().actualCall("test_output").withParameter("buf", buf);
    }

    void multi_output_1(const char *buf)
    {
        mock().actualCall("multi_output_1").withParameter("buf", buf);
    }

    void multi_output_2(const char *buf)
    {
        mock().actualCall("multi_output_2").withParameter("buf", buf);
    }

    extern "C" void putchar_(int c)
    {
    }
} // namespace

// clang-format off

TEST_GROUP(MulogTests)
{
    void teardown() override{
        mulog_remove_all_outputs();
        mock().clear();
    }
};

// clang-format on

TEST(MulogTests, TestNoLogBuffer)
{
    std::array<char, 128> buffer{};
    auto ret = mulog_set_log_buffer(NULLPTR, 0);

    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_set_log_buffer(NULLPTR, 123123);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_set_log_buffer(buffer.data(), 0);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
}

TEST(MulogTests, TestWithLogBuffer)
{
    std::array<char, 128> buffer{};
    auto ret = mulog_set_log_buffer(buffer.data(), buffer.size());

    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING("[DEBUG]: 123", buffer.data());

    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("11111111111111111111111111111111111111111111111111111111111111111111111111111111"
                  "111111111111111111111111111111111111111111111111111");
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING("[DEBUG]: "
                         "1111111111111111111111111111111111111111111111111111111111111111111111111"
                         "111111111111111111111111111111111111111111111",
                         buffer.data());
    const std::string_view str{buffer.data()};
    CHECK_EQUAL(str.size(), buffer.size() - 1);
}

TEST(MulogTests, TestOutputUnregistering)
{
    std::array<char, 128> buffer{};
    auto ret = mulog_set_log_buffer(buffer.data(), buffer.size());

    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_remove_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_remove_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
}

TEST(MulogTests, TestMultipleOutputs)
{
    std::array<char, 128> buffer{};
    mulog_set_log_buffer(buffer.data(), buffer.size());
    auto ret = mulog_add_output(multi_output_1);

    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("multi_output_1").withParameter("buf", buffer.data());
    mock().expectOneCall("multi_output_2").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_remove_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_remove_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
    ret = mulog_remove_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_remove_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
}

int main(int argc, char *argv[])
{
    return RUN_ALL_TESTS(argc, argv);
}
