/**
 * \file
 * \brief mulog library tests
 * \author Vladimir Petrigo
 */
#include "internal/config.h"
#include "internal/utils.h"
#include "mulog.h"

#include <array>
#include <format>
#include <iostream>

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>
#include <CppUTestExt/MockSupport.h>

namespace {
    constexpr std::array log_levels{
        MULOG_TRACE_LVL, MULOG_DEBUG_LVL, MULOG_INFO_LVL, MULOG_WARNING_LVL, MULOG_ERROR_LVL,
    };

    void test_output(const char *buf, const size_t buf_size)
    {
        mock().actualCall("test_output").withParameter("buf", buf);
    }

    void multi_output_1(const char *buf, const size_t buf_size)
    {
        mock()
            .actualCall("multi_output_1")
            .withParameter("buf", buf)
            .withParameter("buf_size", buf_size);
    }

    void multi_output_2(const char *buf, const size_t buf_size)
    {
        mock()
            .actualCall("multi_output_2")
            .withParameter("buf", buf)
            .withParameter("buf_size", buf_size);
    }

    std::string generate_expected_output(const std::string &input, const mulog_log_level log_level,
                                         const size_t max_size)
    {
        if constexpr (MULOG_ENABLE_TIMESTAMP) {
            const auto timestamp_ms = mulog_config_mulog_timestamp_get();
            auto view =
                std::format("{:07}.{:03} {}: {}{}", timestamp_ms / 1000, timestamp_ms % 1000,
                            log_levels[log_level], input, MULOG_LOG_LINE_TERMINATION);

            if (view.size() > max_size) {
                view = view.substr(0, max_size);
            }

            return view;
        } else {
            auto view =
                std::format("{}: {}{}", log_levels[log_level], input, MULOG_LOG_LINE_TERMINATION);

            return view.size() < max_size ? view : view.substr(0, max_size);
        }
    }

    extern "C" bool mulog_config_mulog_lock(void)
    {
        return true;
    }

    extern "C" void mulog_config_mulog_unlock(void)
    {
    }

    extern "C" unsigned long mulog_config_mulog_timestamp_get(void)
    {
        return 42123UL;
    }

    extern "C" void putchar_(int c)
    {
    }
} // namespace

TEST_GROUP(MulogTests)
{
    int unused_{}; // this value is here to prevent clang-format from breaking formatting

    void teardown() override
    {
        mulog_unregister_all_outputs();
        mock().clear();
    }
};

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
        mulog_reset();
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
    const std::string input1{"123"};
    const std::string input2{
        "11111111111111111111111111111111111111111111111111111111111111111111111111111111"
        "111111111111111111111111111111111111111111111111111"};
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG(input1.c_str());
    mock().checkExpectations();
    auto expected = generate_expected_output(input1, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    CHECK_EQUAL_C_STRING(expected.c_str(), get_log_buffer());

    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("11111111111111111111111111111111111111111111111111111111111111111111111111111111"
                  "111111111111111111111111111111111111111111111111111");
    mock().checkExpectations();
    expected = generate_expected_output(input2, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    CHECK_EQUAL_C_STRING(expected.c_str(), get_log_buffer());
    CHECK_EQUAL(expected.size(), buffer.size() - 1);
}

TEST(MulogTestsWithBuffer, TestOutputUnregistering)
{
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_unregister_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    MULOG_LOG_DBG("123");
    mock().checkExpectations();
    ret = mulog_unregister_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
    ret = mulog_unregister_output(NULLPTR);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
    ret = mulog_unregister_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
}

TEST(MulogTestsWithBuffer, TestMultipleOutputs)
{
    const std::string test_str{"123"};
    const auto expected =
        generate_expected_output(test_str, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    auto ret = mulog_add_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock()
        .expectOneCall("multi_output_1")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    mock()
        .expectOneCall("multi_output_2")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    MULOG_LOG_DBG(test_str.c_str());
    mock().checkExpectations();
    ret = mulog_unregister_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_unregister_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
    ret = mulog_unregister_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_unregister_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
}

TEST(MulogTestsWithBuffer, TestGlobalOutputLogLevel)
{
    const std::string test_str1{"123"};
    const std::string test_str2{"345"};
    const std::string test_str3{"678"};
    auto ret = mulog_add_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(multi_output_2);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_WARNING);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("multi_output_1");
    mock().expectNoCall("multi_output_2");
    MULOG_LOG_DBG(test_str1.c_str());
    mock().checkExpectations();
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    auto expected = generate_expected_output(test_str2, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    mock()
        .expectOneCall("multi_output_1")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    mock()
        .expectOneCall("multi_output_2")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    MULOG_LOG_DBG(test_str2.c_str());
    mock().checkExpectations();
    expected = generate_expected_output(test_str3, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    mock()
        .expectOneCall("multi_output_1")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    mock()
        .expectOneCall("multi_output_2")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    MULOG_LOG_TRACE(test_str3.c_str());
    mock().checkExpectations();
}

TEST(MulogTestsWithBuffer, TestDifferentOutputLogLevel)
{
    const std::string test_str1{"123"};
    const std::string test_str2{"345"};
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(multi_output_1);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output_with_log_level(multi_output_2, MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    auto expected = generate_expected_output(test_str1, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    mock().expectNoCall("multi_output_1");
    mock()
        .expectOneCall("multi_output_2")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    MULOG_LOG_TRACE(test_str1.c_str());
    mock().checkExpectations();
    expected = generate_expected_output(test_str2, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    mock()
        .expectOneCall("multi_output_1")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    mock()
        .expectOneCall("multi_output_2")
        .withParameter("buf", get_log_buffer())
        .withParameter("buf_size", expected.size());
    MULOG_LOG_ERR(test_str2.c_str());
    mock().checkExpectations();
}

TEST(MulogTestsWithBuffer, TestPerOutputLogLevel)
{
    const std::string test_str1{"123"};
    const std::string test_str2{"345"};
    const auto ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    MULOG_LOG_DBG(test_str1.c_str());
    mock().checkExpectations();
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_ERR(test_str2.c_str());
    mock().checkExpectations();
    const auto expected =
        generate_expected_output(test_str2, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    CHECK_EQUAL_C_STRING(expected.c_str(), buffer.data());
}

TEST(MulogTestsWithBuffer, TestTooManyOutputs)
{
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    for (size_t i = 0; i < 100; ++i) {
        ret = mulog_add_output(test_output);
        CHECK_EQUAL(MULOG_RET_CODE_NO_MEM, ret);
    }
}

TEST(MulogTestsWithBuffer, TestIncorrectLogLevel)
{
    constexpr std::string_view output{"Hello world"};
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectNoCall("test_output");
    auto chars_written = mulog_log(MULOG_LOG_LVL_COUNT, output.data());
    const auto expected =
        generate_expected_output(std::string{output}, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    CHECK(chars_written == 0);
    mock().checkExpectations();

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        mock().expectOneCall("test_output").withParameter("buf", get_log_buffer());
        chars_written = mulog_log(static_cast<mulog_log_level>(i), output.data());
        CHECK_EQUAL(expected.size(), chars_written);
        mock().checkExpectations();
    }
}

TEST(MulogTestsWithBuffer, TestDeferredLogUnsupported)
{
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    auto ret_int = mulog_deferred_process();
    CHECK_EQUAL(MULOG_RET_CODE_UNSUPPORTED, ret_int);
}

TEST_GROUP(Mulog4ByteBuffer)
{
    std::array<char, 4> buffer{};

    void setup() override
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    void teardown() override
    {
        mulog_unregister_all_outputs();
        mock().clear();
    }
};

TEST(Mulog4ByteBuffer, SingleLog)
{
    const std::string input{"Hello world"};
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    const auto expected = generate_expected_output(input, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG(input.c_str());
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING(expected.c_str(), buffer.data());
}

TEST_GROUP(Mulog16ByteBuffer)
{
    std::array<char, 16> buffer{};

    void setup() override
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    void teardown() override
    {
        mulog_unregister_all_outputs();
        mock().clear();
    }
};

TEST(Mulog16ByteBuffer, SingleLog)
{
    const std::string input{"Hello world"};
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    const auto expected = generate_expected_output(input, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG(input.c_str());
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING(expected.c_str(), buffer.data());
}

TEST_GROUP(Mulog41ByteBuffer)
{
    std::array<char, 41> buffer{};

    void setup() override
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    void teardown() override
    {
        mulog_unregister_all_outputs();
        mock().clear();
    }
};

TEST(Mulog41ByteBuffer, SingleLog)
{
    const std::string input{"Hello world"};
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    const auto expected = generate_expected_output(input, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    mock().expectOneCall("test_output").withParameter("buf", buffer.data());
    MULOG_LOG_DBG(input.c_str());
    mock().checkExpectations();
    CHECK_EQUAL_C_STRING(expected.c_str(), buffer.data());
}

int main(int argc, char *argv[])
{
    return RUN_ALL_TESTS(argc, argv);
}
