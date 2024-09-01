/**
 * \file
 * \brief mulog library tests
 * \author Vladimir Petrigo
 */
#include "mulog.h"

#include <array>
#include <iostream>

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>
#include <CppUTestExt/MockSupport.h>

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

TEST(MulogTests, LogBufferSet)
{
    std::array<char, 128> buffer{};
    const auto ret = mulog_set_log_buffer(buffer.data(), buffer.size());
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
}

TEST_GROUP(MulogTestsWithBuffer)
{
    std::array<char, 128> buffer{};

    void setup() override
    {
        mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    void teardown() override
    {
        mulog_remove_all_outputs();
        mock().clear();
    }

    [[nodiscard]] const char *get_log_buffer() const
    {
        return buffer.begin();
    }
};

TEST(MulogTestsWithBuffer, TestInvalidOutputs)
{
    auto ret = mulog_add_output(NULLPTR);
    CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        ret = mulog_add_output_with_log_level(NULLPTR, MULOG_LOG_LVL_TRACE);
        CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    }
}

TEST(MulogTestsWithBuffer, TestInvalidLogLevels)
{
    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_add_output_with_log_level(
            test_output, static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    }

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_log_level(static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    }

    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        ret = mulog_set_channel_log_level(test_output,
                                          static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    }
}

TEST(MulogTestsWithBuffer, TestSetLogLevelForUnregisteredOutput)
{
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_channel_log_level(multi_output_1, MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
    mock().expectOneCall("test_output").withParameter("buf", get_log_buffer());
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
}

TEST(MulogTestsWithBuffer, TestWithLogBuffer)
{
    const auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING("[DEBUG]: 123", get_log_buffer());

    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("11111111111111111111111111111111111111111111111111111111111111111111111111111111"
                  "111111111111111111111111111111111111111111111111111");
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING("[DEBUG]: "
                         "1111111111111111111111111111111111111111111111111111111111111111111111111"
                         "111111111111111111111111111111111111111111111",
                         get_log_buffer());
    const std::string_view str{get_log_buffer()};
    CHECK_EQUAL(str.size(), buffer.size() - 1);
}

TEST(MulogTestsWithBuffer, TestOutputUnregistering)
{
    auto ret = mulog_add_output(test_output);
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

TEST(MulogTestsWithBuffer, TestMultipleOutputs)
{
    auto ret = mulog_add_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("multi_output_1").withParameter("buf", get_log_buffer());
    mock().expectOneCall("multi_output_2").withParameter("buf", get_log_buffer());
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

TEST(MulogTestsWithBuffer, TestGlobalOutputLogLevel)
{
    auto ret = mulog_add_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_WARNING);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("multi_output_1");
    mock().expectNoCall("multi_output_2");
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("multi_output_1").withParameter("buf", get_log_buffer());
    mock().expectOneCall("multi_output_2").withParameter("buf", get_log_buffer());
    MULOG_LOG_DBG("345");
    mock().checkExpectations();
    mock().expectOneCall("multi_output_1").withParameter("buf", get_log_buffer());
    mock().expectOneCall("multi_output_2").withParameter("buf", get_log_buffer());
    MULOG_LOG_TRACE("678");
    mock().checkExpectations();
}

TEST(MulogTestsWithBuffer, TestDifferentOutputLogLevel)
{
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output_with_log_level(multi_output_2, MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("multi_output_1");
    mock().expectOneCall("multi_output_2").withParameter("buf", get_log_buffer());
    MULOG_LOG_TRACE("123");
    mock().checkExpectations();
    mock().expectOneCall("multi_output_1").withParameter("buf", get_log_buffer());
    mock().expectOneCall("multi_output_2").withParameter("buf", get_log_buffer());
    MULOG_LOG_ERR("345");
    mock().checkExpectations();
}

TEST(MulogTestsWithBuffer, TestPerOutputLogLevel)
{
    const auto ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_ERR("345");
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING("[ERROR]: 345", buffer.data());
}

int main(int argc, char *argv[])
{
    return RUN_ALL_TESTS(argc, argv);
}
