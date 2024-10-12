/**
 * \file
 * \brief mulog tests for the deferred logging mode
 * \author Vladimir Petrigo
 */

#include "internal/config.h"
#include "internal/utils.h"
#include "mulog.h"

#include <array>
#include <format>
#include <iostream>
#include <string_view>

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>
#include <CppUTestExt/MockSupport.h>

namespace {
    constexpr std::array expected{
        std::string_view{MULOG_TRACE_LVL ": "}, std::string_view{MULOG_DEBUG_LVL ": "},
        std::string_view{MULOG_INFO_LVL ": "},  std::string_view{MULOG_WARNING_LVL ": "},
        std::string_view{MULOG_ERROR_LVL ": "},
    };
    constexpr std::array log_levels{
        MULOG_TRACE_LVL, MULOG_DEBUG_LVL, MULOG_INFO_LVL, MULOG_WARNING_LVL, MULOG_ERROR_LVL,
    };
    constexpr std::string_view line_termination{MULOG_LOG_LINE_TERMINATION};

    void test_output(const char *buf, const size_t buf_size)
    {
        const std::string buf_{buf, buf_size};

        mock().actualCall("test_output").withStringParameter("buf", buf_.c_str());
    }

    size_t get_expected_print_size(const std::string &input, mulog_log_level level)
    {
        if constexpr (MULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT) {
            const auto timestamp_ms = mulog_config_mulog_timestamp_get();
            const auto timestamp_str =
                std::format("{:07}.{:03} ", timestamp_ms / 1000, timestamp_ms % 1000);
            return timestamp_str.size() + input.size() + expected[level].size() +
                   line_termination.size();
        } else {
            return input.size() + expected[level].size() + line_termination.size();
        }
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

    extern "C" unsigned long mulog_config_mulog_timestamp_get(void)
    {
        return 42123UL;
    }

    extern "C" void putchar_(int c)
    {
    }
} // namespace

TEST_GROUP(MulogDeferredNoBuf)
{
    int unused_{}; // this value is here to prevent clang-format from breaking formatting

    void teardown() override
    {
        mulog_reset();
        mock().clear();
    }
};

TEST(MulogDeferredNoBuf, InvalidLogBuffer)
{
    std::array<char, 128> buffer{};
    auto ret = mulog_set_log_buffer(NULLPTR, 0);
    CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    ret = mulog_set_log_buffer(NULLPTR, buffer.size());
    CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    ret = mulog_set_log_buffer(buffer.data(), 0);
    CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
}

TEST(MulogDeferredNoBuf, ValidLogBuffer)
{
    std::array<char, 128> buffer{};
    const auto ret = mulog_set_log_buffer(buffer.data(), buffer.size());
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
}

TEST(MulogDeferredNoBuf, UnsupportedInterface)
{
    mulog_set_log_level(MULOG_LOG_LVL_DEBUG);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret =
            mulog_add_output_with_log_level(test_output, static_cast<mulog_log_level>(i));
        CHECK_EQUAL(i == MULOG_LOG_LVL_DEBUG ? MULOG_RET_CODE_OK : MULOG_RET_CODE_UNSUPPORTED, ret);
    }

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_channel_log_level(test_output, static_cast<mulog_log_level>(i));
        CHECK_EQUAL(MULOG_RET_CODE_UNSUPPORTED, ret);
    }
}

TEST(MulogDeferredNoBuf, InvalidLogLevel)
{
    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_log_level(static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    }
}

TEST_GROUP(MulogDeferredWithBuf)
{
    std::array<char, 128> buffer{};

    void setup() override
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    void teardown() override
    {
        mulog_reset();
        mock().clear();
    }

    [[nodiscard]] const char *get_log_buffer() const
    {
        return buffer.data();
    }
};

TEST(MulogDeferredWithBuf, UnsupportedInterface)
{
    mulog_set_log_level(MULOG_LOG_LVL_DEBUG);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret =
            mulog_add_output_with_log_level(test_output, static_cast<mulog_log_level>(i));
        CHECK_EQUAL(i == MULOG_LOG_LVL_DEBUG ? MULOG_RET_CODE_OK : MULOG_RET_CODE_UNSUPPORTED, ret);
    }

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_channel_log_level(test_output, static_cast<mulog_log_level>(i));
        CHECK_EQUAL(MULOG_RET_CODE_UNSUPPORTED, ret);
    }
}

TEST(MulogDeferredWithBuf, InvalidLogLevel)
{
    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_log_level(static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);
    }
}

TEST(MulogDeferredWithBuf, OneLogEntry)
{
    const std::string out1{"A"};
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    for (size_t lvl = MULOG_LOG_LVL_TRACE; lvl < MULOG_LOG_LVL_COUNT; ++lvl) {
        const auto send_to_log = mulog_log(static_cast<mulog_log_level>(lvl), "%s", out1.data());
        const auto expected = get_expected_print_size(out1, static_cast<mulog_log_level>(lvl));
        CHECK_EQUAL(expected, send_to_log);
        mock().expectOneCall("test_output").ignoreOtherParameters();
        const auto printed = mulog_deferred_process();
        mock().checkExpectations();
        CHECK_EQUAL(expected, printed);
    }

    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    for (size_t lvl = MULOG_LOG_LVL_TRACE; lvl < MULOG_LOG_LVL_ERROR; ++lvl) {
        const auto send_to_log = mulog_log(static_cast<mulog_log_level>(lvl), "%s", out1.data());
        CHECK_EQUAL(0, send_to_log);
        mock().expectNoCall("test_output");
        const auto printed = mulog_deferred_process();
        mock().checkExpectations();
        CHECK_EQUAL(0, printed);
    }

    const auto send_to_log = mulog_log(MULOG_LOG_LVL_ERROR, "%s", out1.data());
    const auto expected_size = get_expected_print_size(out1, MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(expected_size, send_to_log);
    mock().expectOneCall("test_output").ignoreOtherParameters();
    const auto printed = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(expected_size, printed);
}

TEST(MulogDeferredWithBuf, MultipleShortLogEntries)
{
    constexpr std::string_view out1{"Hello"};
    constexpr std::string_view out2{"world"};
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    size_t total = 0;

    auto log_ret = mulog_log(MULOG_LOG_LVL_TRACE, "%s", out1.data());
    auto expected_size = get_expected_print_size(std::string{out1}, MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(expected_size, log_ret);
    total += log_ret;
    expected_size = get_expected_print_size(std::string{out2}, MULOG_LOG_LVL_TRACE);
    log_ret = mulog_log(MULOG_LOG_LVL_TRACE, "%s", out2.data());
    CHECK_EQUAL(expected_size, log_ret);
    total += log_ret;

    mock().expectOneCall("test_output").ignoreOtherParameters();
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(total, log_ret);
}

TEST(MulogDeferredWithBuf, LogEntryTrunсation)
{
    const std::string long_string(128, '#');
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    auto log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string.data());
    CHECK_EQUAL(buffer.size() - 1, log_ret);

    mock().expectOneCall("test_output").ignoreOtherParameters();
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(buffer.size() - 1, log_ret);
}

TEST(MulogDeferredWithBuf, LogEntryTrunсationWithMultipleEntries)
{
    const std::string long_string1(42, '#');
    const std::string long_string2(24, '&');
    const std::string long_string3(64, '$');
    const auto expected_output1 =
        generate_expected_output(long_string1, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    size_t expected_free_size = buffer.size() - 1; // lwrb implementation details
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    const auto expected_log_size1 = get_expected_print_size(long_string1, MULOG_LOG_LVL_ERROR);
    auto log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string1.c_str());
    expected_free_size -= log_ret;
    CHECK_EQUAL(expected_log_size1, log_ret);

    mock().expectOneCall("test_output").withStringParameter("buf", expected_output1.c_str());
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    expected_free_size += log_ret;
    CHECK_EQUAL(expected_log_size1, log_ret);

    const auto expected_log_size2 = get_expected_print_size(long_string2, MULOG_LOG_LVL_ERROR);
    log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string2.c_str());
    expected_free_size -= expected_log_size2;
    CHECK_EQUAL(expected_log_size2, log_ret);
    log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string3.c_str());
    CHECK_EQUAL(expected_free_size, log_ret);
    expected_free_size -= log_ret;

    const auto expected_output2 = generate_expected_output(long_string2, MULOG_LOG_LVL_ERROR,
                                                           buffer.size() + 1 - expected_log_size1);
    const auto expected_output3 = generate_expected_output(long_string3, MULOG_LOG_LVL_ERROR,
                                                           buffer.size() - 1 - expected_log_size2);
    mock().expectOneCall("test_output").withStringParameter("buf", expected_output2.c_str());
    mock().expectOneCall("test_output").withStringParameter("buf", expected_output3.c_str());
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(buffer.size() - 1, log_ret);
}

TEST(MulogDeferredWithBuf, LogFullAddNewLine)
{
    const std::string long_string(127, 'a');
    const std::string not_fitted(127, 'b');
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    auto log_ret = MULOG_LOG_ERR("%s", long_string.c_str());
    CHECK_EQUAL(long_string.size(), log_ret);
    log_ret = MULOG_LOG_ERR("%s", not_fitted.c_str());
    CHECK_EQUAL(0, log_ret);

    const std::string_view view{long_string.c_str(),
                                long_string.size() - expected[MULOG_LOG_LVL_ERROR].size()};
    const auto expected_str =
        generate_expected_output(std::string{view}, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    mock().expectOneCall("test_output").withStringParameter("buf", expected_str.c_str());
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(buffer.size() - 1, log_ret);
}

TEST(MulogDeferredWithBuf, InvalidOutputConfiguration)
{
    auto ret = mulog_add_output(NULLPTR);
    CHECK_EQUAL(MULOG_RET_CODE_INVALID_ARG, ret);

    for (size_t i = 0; i < 2; ++i) {
        ret = mulog_add_output(test_output);
        CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    }

    for (size_t i = 0; i < 10; ++i) {
        ret = mulog_add_output(test_output);
        CHECK_EQUAL(MULOG_RET_CODE_NO_MEM, ret);
    }
}

TEST(MulogDeferredWithBuf, UnregisterOutput)
{
    auto ret = mulog_add_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    const std::string string(10, 'a');
    auto log_ret = MULOG_LOG_TRACE("%s", string.c_str());
    auto expected_size = get_expected_print_size(string, MULOG_LOG_LVL_TRACE);
    CHECK_EQUAL(expected_size, log_ret);
    const auto expected_str =
        generate_expected_output(string, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    mock().expectOneCall("test_output").withStringParameter("buf", expected_str.c_str());
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(expected_size, log_ret);

    ret = mulog_unregister_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    log_ret = MULOG_LOG_TRACE("%s", string.c_str());
    CHECK_EQUAL(0, log_ret);
    mock().expectNoCall("test_output");
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(0, log_ret);
    ret = mulog_unregister_output(test_output);
    CHECK_EQUAL(MULOG_RET_CODE_NOT_FOUND, ret);
}

int main(const int argc, char *argv[])
{
    return RUN_ALL_TESTS(argc, argv);
}
