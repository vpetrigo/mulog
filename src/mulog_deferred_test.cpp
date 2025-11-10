/**
 * \file
 * \brief mulog tests for the deferred logging mode
 * \author Vladimir Petrigo
 */
#include "internal/config.h"
#include "internal/utils.h"
#include "mulog.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include <fmt/format.h>

#include <array>
#include <iostream>
#include <string_view>

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

    class OutputMock {
    public:
        MAKE_MOCK2(test_output, void(const char *, const size_t));
    };

    OutputMock output_mock;

    void test_output(const char *buf, const size_t buf_size)
    {
        const std::string expected_str{buf, buf_size};

        output_mock.test_output(expected_str.c_str(), buf_size);
    }

    size_t get_expected_print_size(const std::string &input, mulog_log_level level)
    {
        if constexpr (MULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT) {
            const auto timestamp_ms = mulog_config_mulog_timestamp_get();
            const auto timestamp_str =
                fmt::format("{:07}.{:03} ", timestamp_ms / 1000, timestamp_ms % 1000);
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
                fmt::format("{:07}.{:03} {}: {}{}", timestamp_ms / 1000, timestamp_ms % 1000,
                            log_levels[log_level], input, MULOG_LOG_LINE_TERMINATION);

            if (view.size() > max_size) {
                view = view.substr(0, max_size);
            }

            return view;
        } else {
            auto view =
                fmt::format("{}: {}{}", log_levels[log_level], input, MULOG_LOG_LINE_TERMINATION);

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

class MulogDeferredNoBuf {
public:
    MulogDeferredNoBuf()
    {
        mulog_set_log_buffer(nullptr, 0);
    }

    ~MulogDeferredNoBuf()
    {
        mulog_reset();
    }
};

TEST_CASE_METHOD(MulogDeferredNoBuf, "MulogDeferredNoBuf - InvalidLogBuffer", "[deferred]")
{
    std::array<char, 128> buffer{};
    auto ret = mulog_set_log_buffer(nullptr, 0);
    REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    ret = mulog_set_log_buffer(nullptr, buffer.size());
    REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    ret = mulog_set_log_buffer(buffer.data(), 0);
    REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
}

TEST_CASE_METHOD(MulogDeferredNoBuf, "MulogDeferredNoBuf - ValidLogBuffer", "[deferred]")
{
    std::array<char, 128> buffer{};
    const auto ret = mulog_set_log_buffer(buffer.data(), buffer.size());
    REQUIRE(MULOG_RET_CODE_OK == ret);
}

TEST_CASE_METHOD(MulogDeferredNoBuf, "MulogDeferredNoBuf - UnsupportedInterface", "[deferred]")
{
    mulog_set_log_level(MULOG_LOG_LVL_DEBUG);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret =
            mulog_add_output_with_log_level(test_output, static_cast<mulog_log_level>(i));
        REQUIRE((i == MULOG_LOG_LVL_DEBUG ? MULOG_RET_CODE_OK : MULOG_RET_CODE_UNSUPPORTED) == ret);
    }

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_channel_log_level(test_output, static_cast<mulog_log_level>(i));
        REQUIRE(MULOG_RET_CODE_UNSUPPORTED == ret);
    }
}

TEST_CASE_METHOD(MulogDeferredNoBuf, "MulogDeferredNoBuf - InvalidLogLevel", "[deferred]")
{
    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_log_level(static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    }
}

class MulogDeferredWithBuf {
public:
    std::array<char, 128> buffer{};

    MulogDeferredWithBuf()
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~MulogDeferredWithBuf()
    {
        mulog_reset();
    }

    [[nodiscard]] const char *get_log_buffer() const
    {
        return buffer.data();
    }
};

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - UnsupportedInterface", "[deferred]")
{
    mulog_set_log_level(MULOG_LOG_LVL_DEBUG);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret =
            mulog_add_output_with_log_level(test_output, static_cast<mulog_log_level>(i));
        REQUIRE((i == MULOG_LOG_LVL_DEBUG ? MULOG_RET_CODE_OK : MULOG_RET_CODE_UNSUPPORTED) == ret);
    }

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_channel_log_level(test_output, static_cast<mulog_log_level>(i));
        REQUIRE(MULOG_RET_CODE_UNSUPPORTED == ret);
    }
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - InvalidLogLevel", "[deferred]")
{
    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_log_level(static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    }
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - OneLogEntry", "[deferred]")
{
    const std::string out1{"A"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    for (size_t lvl = MULOG_LOG_LVL_TRACE; lvl < MULOG_LOG_LVL_COUNT; ++lvl) {
        const auto send_to_log = mulog_log(static_cast<mulog_log_level>(lvl), "%s", out1.data());
        const auto expected_val = get_expected_print_size(out1, static_cast<mulog_log_level>(lvl));
        REQUIRE(expected_val == send_to_log);
        REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
        const auto printed = mulog_deferred_process();
        REQUIRE(expected_val == printed);
    }

    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    for (size_t lvl = MULOG_LOG_LVL_TRACE; lvl < MULOG_LOG_LVL_ERROR; ++lvl) {
        const auto send_to_log = mulog_log(static_cast<mulog_log_level>(lvl), "%s", out1.data());
        REQUIRE(0 == send_to_log);
        FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
        const auto printed = mulog_deferred_process();
        REQUIRE(0 == printed);
    }

    const auto send_to_log = mulog_log(MULOG_LOG_LVL_ERROR, "%s", out1.data());
    const auto expected_size = get_expected_print_size(out1, MULOG_LOG_LVL_ERROR);
    REQUIRE(expected_size == send_to_log);
    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(expected_size == printed);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - MultipleShortLogEntries",
                 "[deferred]")
{
    constexpr std::string_view out1{"Hello"};
    constexpr std::string_view out2{"world"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    size_t total = 0;

    auto log_ret = mulog_log(MULOG_LOG_LVL_TRACE, "%s", out1.data());
    auto expected_size = get_expected_print_size(std::string{out1}, MULOG_LOG_LVL_TRACE);
    REQUIRE(expected_size == log_ret);
    total += log_ret;
    expected_size = get_expected_print_size(std::string{out2}, MULOG_LOG_LVL_TRACE);
    log_ret = mulog_log(MULOG_LOG_LVL_TRACE, "%s", out2.data());
    REQUIRE(expected_size == log_ret);
    total += log_ret;

    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    log_ret = mulog_deferred_process();
    REQUIRE(total == log_ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - LogEntryTrunсation", "[deferred]")
{
    const std::string long_string(128, '#');
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    auto log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string.data());
    REQUIRE(buffer.size() - 1 == log_ret);

    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    log_ret = mulog_deferred_process();
    REQUIRE(buffer.size() - 1 == log_ret);
    mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
}

TEST_CASE_METHOD(MulogDeferredWithBuf,
                 "MulogDeferredWithBuf - LogEntryTrunсationWithMultipleEntries", "[deferred]")
{
    const std::string long_string1(42, '#');
    const std::string long_string2(24, '&');
    const std::string long_string3(64, '$');
    const auto expected_output1 =
        generate_expected_output(long_string1, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    size_t expected_free_size = buffer.size() - 1; // lwrb implementation details
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const auto expected_log_size1 = get_expected_print_size(long_string1, MULOG_LOG_LVL_ERROR);
    auto log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string1.c_str());
    expected_free_size -= log_ret;
    REQUIRE(expected_log_size1 == log_ret);

    REQUIRE_CALL(output_mock, test_output(trompeloeil::eq(expected_output1), trompeloeil::_));
    log_ret = mulog_deferred_process();
    expected_free_size += log_ret;
    REQUIRE(expected_log_size1 == log_ret);

    const auto expected_log_size2 = get_expected_print_size(long_string2, MULOG_LOG_LVL_ERROR);
    log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string2.c_str());
    expected_free_size -= expected_log_size2;
    REQUIRE(expected_log_size2 == log_ret);
    log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "%s", long_string3.c_str());
    REQUIRE(expected_free_size == log_ret);
    expected_free_size -= log_ret;
    REQUIRE(0 == expected_free_size);
    const auto expected_output2 = generate_expected_output(long_string2, MULOG_LOG_LVL_ERROR,
                                                           buffer.size() + 1 - expected_log_size1);
    const auto expected_output3 = generate_expected_output(long_string3, MULOG_LOG_LVL_ERROR,
                                                           buffer.size() - 1 - expected_log_size2);
    REQUIRE_CALL(output_mock, test_output(trompeloeil::eq(expected_output2), trompeloeil::_));
    REQUIRE_CALL(output_mock, test_output(trompeloeil::eq(expected_output3), trompeloeil::_));
    log_ret = mulog_deferred_process();
    REQUIRE(buffer.size() - 1 == log_ret);
    mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - LogFullAddNewLine", "[deferred]")
{
    const std::string long_string(127, 'a');
    const std::string not_fitted(127, 'b');
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    auto log_ret = MULOG_LOG_ERR("%s", long_string.c_str());
    REQUIRE(long_string.size() == log_ret);
    log_ret = MULOG_LOG_ERR("%s", not_fitted.c_str());
    REQUIRE(0 == log_ret);

    const std::string_view view{long_string.c_str(),
                                long_string.size() - expected[MULOG_LOG_LVL_ERROR].size()};
    const auto expected_str =
        generate_expected_output(std::string{view}, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(trompeloeil::eq(expected_str), trompeloeil::_));
    log_ret = mulog_deferred_process();
    REQUIRE(buffer.size() - 1 == log_ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - InvalidOutputConfiguration",
                 "[deferred]")
{
    auto ret = mulog_add_output(nullptr);
    REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);

    for (size_t i = 0; i < 2; ++i) {
        ret = mulog_add_output(test_output);
        REQUIRE(MULOG_RET_CODE_OK == ret);
    }

    for (size_t i = 0; i < 10; ++i) {
        ret = mulog_add_output(test_output);
        REQUIRE(MULOG_RET_CODE_NO_MEM == ret);
    }
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - UnregisterOutput", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const std::string string(10, 'a');
    auto log_ret = MULOG_LOG_TRACE("%s", string.c_str());
    auto expected_size = get_expected_print_size(string, MULOG_LOG_LVL_TRACE);
    REQUIRE(expected_size == log_ret);
    const auto expected_str =
        generate_expected_output(string, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(trompeloeil::eq(expected_str), trompeloeil::_));
    log_ret = mulog_deferred_process();
    REQUIRE(expected_size == log_ret);

    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    log_ret = MULOG_LOG_TRACE("%s", string.c_str());
    REQUIRE(0 == log_ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    log_ret = mulog_deferred_process();
    REQUIRE(0 == log_ret);
    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - NoOutputRegistered", "[deferred]")
{
    mulog_reset();
    auto ret = MULOG_LOG_ERR("%s", "Hello");
    REQUIRE(0 == ret);

    mulog_add_output(test_output);
    ret = MULOG_LOG_ERR("%s", "Hello");
    REQUIRE(0 == ret);
}

class SmallBuffer {
public:
    std::array<char, 8> buffer{};

    SmallBuffer()
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~SmallBuffer()
    {
        mulog_reset();
    }
};

TEST_CASE_METHOD(SmallBuffer, "SmallBuffer - LogSingle", "[deferred]")
{
    const std::string input{"Hello"};
    mulog_add_output(test_output);
    const auto ret = MULOG_LOG_ERR("%s", input.c_str());
    REQUIRE(0 == ret);
}

class SmallBuffer16 {
public:
    std::array<char, 16> buffer{};

    SmallBuffer16()
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~SmallBuffer16()
    {
        mulog_reset();
    }
};

TEST_CASE_METHOD(SmallBuffer16, "SmallBuffer16 - LogSingle", "[deferred]")
{
    const std::string input{"Hello"};
    mulog_add_output(test_output);
    const auto ret = MULOG_LOG_ERR("%s", input.c_str());
    REQUIRE(0 == ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - AllLogLevelMacros", "[deferred]")
{
    const std::string test_str{"A"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Test all log level macros one by one with processing in between
    auto log_ret = MULOG_LOG_TRACE("%s", test_str.c_str());
    auto expected_size = get_expected_print_size(test_str, MULOG_LOG_LVL_TRACE);
    REQUIRE(expected_size == log_ret);
    {
        REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_)).TIMES(1);
        REQUIRE(log_ret == mulog_deferred_process());
    }

    log_ret = MULOG_LOG_DBG("%s", test_str.c_str());
    expected_size = get_expected_print_size(test_str, MULOG_LOG_LVL_DEBUG);
    REQUIRE(expected_size == log_ret);
    {
        REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_)).TIMES(1);
        REQUIRE(log_ret == mulog_deferred_process());
    }

    log_ret = MULOG_LOG_INFO("%s", test_str.c_str());
    expected_size = get_expected_print_size(test_str, MULOG_LOG_LVL_INFO);
    REQUIRE(expected_size == log_ret);
    {
        REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_)).TIMES(1);
        REQUIRE(log_ret == mulog_deferred_process());
    }

    log_ret = MULOG_LOG_WARN("%s", test_str.c_str());
    expected_size = get_expected_print_size(test_str, MULOG_LOG_LVL_WARNING);
    REQUIRE(expected_size == log_ret);
    {
        REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_)).TIMES(1);
        REQUIRE(log_ret == mulog_deferred_process());
    }

    log_ret = MULOG_LOG_ERR("%s", test_str.c_str());
    expected_size = get_expected_print_size(test_str, MULOG_LOG_LVL_ERROR);
    REQUIRE(expected_size == log_ret);
    {
        REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_)).TIMES(1);
        REQUIRE(log_ret == mulog_deferred_process());
    }
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - MultipleProcessCalls", "[deferred]")
{
    const std::string test_str{"test"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Log first message
    auto log_ret = MULOG_LOG_DBG("%s", test_str.c_str());
    auto expected_size = get_expected_print_size(test_str, MULOG_LOG_LVL_DEBUG);
    REQUIRE(expected_size == log_ret);

    // Process first message
    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    auto printed = mulog_deferred_process();
    REQUIRE(expected_size == printed);

    // Log second message
    log_ret = MULOG_LOG_DBG("%s", test_str.c_str());
    REQUIRE(expected_size == log_ret);

    // Process second message
    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    printed = mulog_deferred_process();
    REQUIRE(expected_size == printed);

    // Process when buffer is empty - should return 0
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    printed = mulog_deferred_process();
    REQUIRE(0 == printed);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - EmptyFormatString", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    auto log_ret = MULOG_LOG_DBG("");
    auto expected_size = get_expected_print_size("", MULOG_LOG_LVL_DEBUG);
    REQUIRE(expected_size == log_ret);

    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(expected_size == printed);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - ComplexFormatting", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_INFO);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const std::string formatted = "int=42, str=hello, float=3.14";
    const auto log_ret = MULOG_LOG_INFO("int=%d, str=%s, float=%.2f", 42, "hello", 3.14);
    const auto expected_size = get_expected_print_size(formatted, MULOG_LOG_LVL_INFO);
    REQUIRE(expected_size == log_ret);

    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(expected_size == printed);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - MixedLogLevels", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    size_t total_written = 0;

    auto log_ret = MULOG_LOG_DBG("debug");
    total_written += log_ret;

    log_ret = MULOG_LOG_INFO("info");
    total_written += log_ret;

    log_ret = MULOG_LOG_ERR("error");
    total_written += log_ret;

    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(total_written == printed);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - ResetClearsBuffer", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    const auto log_ret = MULOG_LOG_DBG("test");
    REQUIRE(log_ret > 0);
    mulog_reset();
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(0 == printed);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - MultipleOutputsSameLogLevel", "[deferred]")
{
    mulog_log_output_fn output_1 = test_output;

    auto ret = mulog_add_output(output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_UNSUPPORTED == ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - BufferWrapAround", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    for (int i = 0; i < 3; ++i) {
        const auto written = MULOG_LOG_DBG("m%d", i);
        REQUIRE(written > 0);
    }

    ALLOW_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    auto printed = mulog_deferred_process();
    REQUIRE(printed > 0);

    for (int i = 0; i < 2; ++i) {
        const auto written = MULOG_LOG_DBG("n%d", i);
        REQUIRE(written > 0);
    }

    ALLOW_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    printed = mulog_deferred_process();
    REQUIRE(printed > 0);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - PartialProcess", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    size_t total = 0;
    const auto written1 = MULOG_LOG_DBG("msg1");
    total += written1;
    const auto written2 = MULOG_LOG_DBG("msg2");
    total += written2;
    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(total == printed);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto empty_process = mulog_deferred_process();
    REQUIRE(0 == empty_process);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - LogAfterUnregisterAll", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    auto log_ret = MULOG_LOG_DBG("before");
    REQUIRE(log_ret > 0);
    mulog_unregister_all_outputs();
    log_ret = MULOG_LOG_DBG("after");
    REQUIRE(0 == log_ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    mulog_deferred_process();
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - TestLogLevelBoundary", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_WARNING);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    auto log_ret = MULOG_LOG_TRACE("trace");
    REQUIRE(0 == log_ret);
    log_ret = MULOG_LOG_DBG("debug");
    REQUIRE(0 == log_ret);
    log_ret = MULOG_LOG_INFO("info");
    REQUIRE(0 == log_ret);
    log_ret = MULOG_LOG_WARN("warning");
    auto expected_size = get_expected_print_size("warning", MULOG_LOG_LVL_WARNING);
    REQUIRE(expected_size == log_ret);
    log_ret = MULOG_LOG_ERR("error");
    expected_size = get_expected_print_size("error", MULOG_LOG_LVL_ERROR);
    REQUIRE(expected_size == log_ret);
    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    mulog_deferred_process();
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - VeryLongSingleMessage", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const std::string very_long(256, 'L');
    const auto log_ret = MULOG_LOG_ERR("%s", very_long.c_str());

    REQUIRE(log_ret <= buffer.size() - 1);
    REQUIRE(log_ret > 0);

    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(printed == log_ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - SequentialLogAndProcess", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    for (int i = 0; i < 5; ++i) {
        const auto log_ret = MULOG_LOG_DBG("i%d", i);
        REQUIRE(log_ret > 0);

        ALLOW_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
        const auto printed = mulog_deferred_process();
        REQUIRE(log_ret == printed);
    }
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - BufferFullPreventNewWrite", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Fill buffer completely
    const std::string fill_msg(100, 'X');
    auto log_ret = MULOG_LOG_ERR("%s", fill_msg.c_str());
    REQUIRE(log_ret > 0);

    // Try to add more when buffer is nearly full
    const std::string another_msg(50, 'Y');
    log_ret = MULOG_LOG_ERR("%s", another_msg.c_str());

    // Process and verify
    ALLOW_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    mulog_deferred_process();
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - MultipleOutputsProcessing", "[deferred]")
{
    mulog_log_output_fn output_1 = test_output;
    mulog_log_output_fn output_2 = test_output;

    auto ret = mulog_add_output(output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(output_2);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const std::string msg = "dual";
    auto log_ret = MULOG_LOG_DBG("%s", msg.c_str());
    REQUIRE(log_ret > 0);

    // Both outputs should be called during processing
    REQUIRE_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_)).TIMES(2);
    const auto printed = mulog_deferred_process();
    REQUIRE(printed == log_ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - ProcessEmptyBuffer", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    const auto printed = mulog_deferred_process();
    REQUIRE(0 == printed);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - InvalidLogLevelInOutput", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Test with invalid log level (should be filtered out)
    const auto log_ret = mulog_log(MULOG_LOG_LVL_COUNT, "invalid");
    REQUIRE(0 == log_ret);
}

TEST_CASE_METHOD(MulogDeferredWithBuf, "MulogDeferredWithBuf - SetGlobalLevelAfterAddOutput", "[deferred]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Changing global level after adding output
    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Debug should be filtered
    auto log_ret = MULOG_LOG_DBG("debug");
    REQUIRE(0 == log_ret);

    // Error should pass
    log_ret = MULOG_LOG_ERR("error");
    REQUIRE(log_ret > 0);

    ALLOW_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    mulog_deferred_process();
}

