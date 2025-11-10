/**
 * \file
 * \brief mulog library tests
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
#include <string>

namespace {
    constexpr std::array log_levels{
        MULOG_TRACE_LVL, MULOG_DEBUG_LVL, MULOG_INFO_LVL, MULOG_WARNING_LVL, MULOG_ERROR_LVL,
    };

    class OutputMock {
    public:
        MAKE_MOCK2(test_output, void(const char *, const size_t));
        MAKE_MOCK2(multi_output_1, void(const char *, const size_t));
        MAKE_MOCK2(multi_output_2, void(const char *, const size_t));
    };

    OutputMock output_mock;

    void test_output(const char *buf, const size_t buf_size)
    {
        output_mock.test_output(buf, buf_size);
    }

    void multi_output_1(const char *buf, const size_t buf_size)
    {
        output_mock.multi_output_1(buf, buf_size);
    }

    void multi_output_2(const char *buf, const size_t buf_size)
    {
        output_mock.multi_output_2(buf, buf_size);
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

TEST_CASE("MulogTests - TestNoLogBuffer", "[mulog]")
{
    std::array<char, 128> buffer{};
    auto ret = mulog_set_log_buffer(nullptr, 0);

    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("123");

    ret = mulog_set_log_buffer(nullptr, 123123);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("123");

    ret = mulog_set_log_buffer(buffer.data(), 0);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("123");

    mulog_unregister_all_outputs();
}

TEST_CASE("MulogTests - LogBufferSet", "[mulog]")
{
    std::array<char, 128> buffer{};
    const auto ret = mulog_set_log_buffer(buffer.data(), buffer.size());
    REQUIRE(MULOG_RET_CODE_OK == ret);
}

class MulogTestsWithBuffer {
public:
    std::array<char, 128> buffer{};

    MulogTestsWithBuffer()
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~MulogTestsWithBuffer()
    {
        mulog_reset();
    }

    [[nodiscard]] const char *get_log_buffer() const
    {
        return buffer.begin();
    }
};

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestInvalidOutputs", "[mulog]")
{
    auto ret = mulog_add_output(nullptr);
    REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        ret = mulog_add_output_with_log_level(nullptr, MULOG_LOG_LVL_TRACE);
        REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    }
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestInvalidLogLevels", "[mulog]")
{
    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_add_output_with_log_level(
            test_output, static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    }

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        const auto ret = mulog_set_log_level(static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    }

    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        ret = mulog_set_channel_log_level(test_output,
                                          static_cast<mulog_log_level>(MULOG_LOG_LVL_COUNT + i));
        REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
    }
}

TEST_CASE_METHOD(MulogTestsWithBuffer,
                 "MulogTestsWithBuffer - TestSetLogLevelForUnregisteredOutput", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_channel_log_level(multi_output_1, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), trompeloeil::_));
    MULOG_LOG_DBG("123");
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestWithLogBuffer", "[mulog]")
{
    const auto ret = mulog_add_output(test_output);
    const std::string input1{"123"};
    const std::string input2{
        "11111111111111111111111111111111111111111111111111111111111111111111111111111111"
        "111111111111111111111111111111111111111111111111111"};
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("%s", input1.c_str());
    auto expected = generate_expected_output(input1, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE(expected == std::string(get_log_buffer()));

    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("11111111111111111111111111111111111111111111111111111111111111111111111111111111"
                  "111111111111111111111111111111111111111111111111111");
    expected = generate_expected_output(input2, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE(expected == std::string(get_log_buffer()));
    REQUIRE(expected.size() == buffer.size() - 1);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestOutputUnregistering", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("123");
    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("123");
    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
    ret = mulog_unregister_output(nullptr);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
    ret = mulog_unregister_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestMultipleOutputs", "[mulog]")
{
    const std::string test_str{"123"};
    const auto expected =
        generate_expected_output(test_str, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    auto ret = mulog_add_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(multi_output_2);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(output_mock, multi_output_1(get_log_buffer(), expected.size()));
    REQUIRE_CALL(output_mock, multi_output_2(get_log_buffer(), expected.size()));
    MULOG_LOG_DBG("%s", test_str.c_str());
    ret = mulog_unregister_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_unregister_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
    ret = mulog_unregister_output(multi_output_2);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_unregister_output(multi_output_2);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestGlobalOutputLogLevel", "[mulog]")
{
    const std::string test_str1{"123"};
    const std::string test_str2{"345"};
    const std::string test_str3{"678"};
    auto ret = mulog_add_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(multi_output_2);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_WARNING);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    FORBID_CALL(output_mock, multi_output_1(trompeloeil::_, trompeloeil::_));
    FORBID_CALL(output_mock, multi_output_2(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("%s", test_str1.c_str());
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    auto expected = generate_expected_output(test_str2, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, multi_output_1(get_log_buffer(), expected.size()));
    REQUIRE_CALL(output_mock, multi_output_2(get_log_buffer(), expected.size()));
    MULOG_LOG_DBG("%s", test_str2.c_str());
    expected = generate_expected_output(test_str3, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    REQUIRE_CALL(output_mock, multi_output_1(get_log_buffer(), expected.size()));
    REQUIRE_CALL(output_mock, multi_output_2(get_log_buffer(), expected.size()));
    MULOG_LOG_TRACE("%s", test_str3.c_str());
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestDifferentOutputLogLevel",
                 "[mulog]")
{
    const std::string test_str1{"123"};
    const std::string test_str2{"345"};
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output_with_log_level(multi_output_2, MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    auto expected = generate_expected_output(test_str1, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    FORBID_CALL(output_mock, multi_output_1(trompeloeil::_, trompeloeil::_));
    REQUIRE_CALL(output_mock, multi_output_2(get_log_buffer(), expected.size()));
    MULOG_LOG_TRACE("%s", test_str1.c_str());
    expected = generate_expected_output(test_str2, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    REQUIRE_CALL(output_mock, multi_output_1(get_log_buffer(), expected.size()));
    REQUIRE_CALL(output_mock, multi_output_2(get_log_buffer(), expected.size()));
    MULOG_LOG_ERR("%s", test_str2.c_str());
    mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestPerOutputLogLevel", "[mulog]")
{
    const std::string test_str1{"123"};
    const std::string test_str2{"345"};
    const auto ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("%s", test_str1.c_str());
    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_ERR("%s", test_str2.c_str());
    const auto expected =
        generate_expected_output(test_str2, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    REQUIRE(expected == std::string(buffer.data()));
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestTooManyOutputs", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    for (size_t i = 0; i < 100; ++i) {
        ret = mulog_add_output(test_output);
        REQUIRE(MULOG_RET_CODE_NO_MEM == ret);
    }
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestIncorrectLogLevel", "[mulog]")
{
    constexpr std::string_view output{"Hello world"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    auto chars_written = mulog_log(MULOG_LOG_LVL_COUNT, output.data());
    const auto expected =
        generate_expected_output(std::string{output}, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    REQUIRE(chars_written == 0);

    for (size_t i = 0; i < MULOG_LOG_LVL_COUNT; ++i) {
        REQUIRE_CALL(output_mock, test_output(get_log_buffer(), trompeloeil::_));
        chars_written = mulog_log(static_cast<mulog_log_level>(i), output.data());
        REQUIRE(expected.size() == chars_written);
    }
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestDeferredLogUnsupported",
                 "[mulog]")
{
    const auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    const auto ret_int = mulog_deferred_process();
    REQUIRE(MULOG_RET_CODE_UNSUPPORTED == ret_int);
}

class Mulog4ByteBuffer {
public:
    std::array<char, 4> buffer{};

    Mulog4ByteBuffer()
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~Mulog4ByteBuffer()
    {
        mulog_reset();
    }
};

TEST_CASE_METHOD(Mulog4ByteBuffer, "Mulog4ByteBuffer - SingleLog", "[mulog]")
{
    const std::string input{"Hello world"};
    const auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    const auto expected = generate_expected_output(input, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("%s", input.c_str());
    REQUIRE(expected == std::string(buffer.data()));
}

class Mulog16ByteBuffer {
public:
    std::array<char, 16> buffer{};

    Mulog16ByteBuffer()
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~Mulog16ByteBuffer()
    {
        mulog_reset();
    }
};

TEST_CASE_METHOD(Mulog16ByteBuffer, "Mulog16ByteBuffer - SingleLog", "[mulog]")
{
    const std::string input{"Hello world"};
    const auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    const auto expected = generate_expected_output(input, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("%s", input.c_str());
    REQUIRE(expected == std::string(buffer.data()));
}

class Mulog41ByteBuffer {
public:
    std::array<char, 41> buffer{};

    Mulog41ByteBuffer()
    {
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~Mulog41ByteBuffer()
    {
        mulog_reset();
    }
};

TEST_CASE_METHOD(Mulog41ByteBuffer, "Mulog41ByteBuffer - SingleLog", "[mulog]")
{
    const std::string input{"Hello world"};
    const auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    const auto expected = generate_expected_output(input, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("%s", input.c_str());
    REQUIRE(expected == std::string(buffer.data()));
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestAllLogLevelMacros", "[mulog]")
{
    const std::string test_str{"test"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    auto expected = generate_expected_output(test_str, MULOG_LOG_LVL_TRACE, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_TRACE("%s", test_str.c_str());

    expected = generate_expected_output(test_str, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_DBG("%s", test_str.c_str());

    expected = generate_expected_output(test_str, MULOG_LOG_LVL_INFO, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_INFO("%s", test_str.c_str());

    expected = generate_expected_output(test_str, MULOG_LOG_LVL_WARNING, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_WARN("%s", test_str.c_str());

    expected = generate_expected_output(test_str, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_ERR("%s", test_str.c_str());
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestLogLevelFiltering", "[mulog]")
{
    const std::string test_str{"test"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_set_log_level(MULOG_LOG_LVL_INFO);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_TRACE("%s", test_str.c_str());
    MULOG_LOG_DBG("%s", test_str.c_str());

    auto expected = generate_expected_output(test_str, MULOG_LOG_LVL_INFO, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_INFO("%s", test_str.c_str());

    expected = generate_expected_output(test_str, MULOG_LOG_LVL_WARNING, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_WARN("%s", test_str.c_str());

    expected = generate_expected_output(test_str, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_ERR("%s", test_str.c_str());
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestChannelLogLevelUpdate", "[mulog]")
{
    const std::string test_str{"test"};
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_TRACE("%s", test_str.c_str());
    MULOG_LOG_DBG("%s", test_str.c_str());
    MULOG_LOG_INFO("%s", test_str.c_str());
    MULOG_LOG_WARN("%s", test_str.c_str());

    const auto expected = generate_expected_output(test_str, MULOG_LOG_LVL_ERROR, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_ERR("%s", test_str.c_str());
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestGlobalLogLevelChangeAffectsAll", "[mulog]")
{
    const std::string test_str{"test"};
    auto ret = mulog_add_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(multi_output_2);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_set_log_level(MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    FORBID_CALL(output_mock, multi_output_1(trompeloeil::_, trompeloeil::_));
    FORBID_CALL(output_mock, multi_output_2(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_TRACE("%s", test_str.c_str());

    const auto expected = generate_expected_output(test_str, MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, multi_output_1(get_log_buffer(), expected.size()));
    REQUIRE_CALL(output_mock, multi_output_2(get_log_buffer(), expected.size()));
    MULOG_LOG_DBG("%s", test_str.c_str());
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestResetClearsOutputs", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    REQUIRE_CALL(output_mock, test_output(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("before reset");

    mulog_reset();

    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("after reset");
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestUnregisterAllOutputs", "[mulog]")
{
    auto ret = mulog_add_output(multi_output_1);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    ret = mulog_add_output(multi_output_2);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    REQUIRE_CALL(output_mock, multi_output_1(buffer.data(), trompeloeil::_));
    REQUIRE_CALL(output_mock, multi_output_2(buffer.data(), trompeloeil::_));
    MULOG_LOG_DBG("before unregister");

    mulog_unregister_all_outputs();

    FORBID_CALL(output_mock, multi_output_1(trompeloeil::_, trompeloeil::_));
    FORBID_CALL(output_mock, multi_output_2(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("after unregister");
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestEmptyFormatString", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const auto expected = generate_expected_output("", MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_DBG("");
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestComplexFormatting", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const std::string formatted = "int=42, str=hello, float=3.14";
    const auto expected = generate_expected_output(formatted, MULOG_LOG_LVL_INFO, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_INFO("int=%d, str=%s, float=%.2f", 42, "hello", 3.14);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestLogWithoutOutput", "[mulog]")
{
    // No output registered
    FORBID_CALL(output_mock, test_output(trompeloeil::_, trompeloeil::_));
    MULOG_LOG_DBG("no output");
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestMultipleUnregisterSameOutput", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestSetChannelLogLevelNotFound", "[mulog]")
{
    // Try to set log level for non-existent output
    auto ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_NOT_FOUND == ret);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestAllLogLevelsFiltered", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_set_log_level(MULOG_LOG_LVL_COUNT);
    REQUIRE(MULOG_RET_CODE_INVALID_ARG == ret);
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestVeryLongMessage", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    const std::string very_long(256, 'L');
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), trompeloeil::_));
    MULOG_LOG_DBG("%s", very_long.c_str());
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestBufferBoundary", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Message that exactly fits the buffer
    const std::string exact_fit(buffer.size() - 30, 'X');
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), trompeloeil::_));
    MULOG_LOG_DBG("%s", exact_fit.c_str());
}

TEST_CASE_METHOD(MulogTestsWithBuffer, "MulogTestsWithBuffer - TestSetChannelAfterGlobalChange", "[mulog]")
{
    auto ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_set_log_level(MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_DEBUG);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    // Debug should now work because channel level is DEBUG
    const auto expected = generate_expected_output("test", MULOG_LOG_LVL_DEBUG, buffer.size() - 1);
    REQUIRE_CALL(output_mock, test_output(get_log_buffer(), expected.size()));
    MULOG_LOG_DBG("test");
}
