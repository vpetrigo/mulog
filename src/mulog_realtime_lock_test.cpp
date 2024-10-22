/**
 * \file
 * \brief
 * \author
 */
#include "internal/utils.h"
#include "mulog.h"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>
#include <CppUTestExt/MockSupport.h>

#include <array>

namespace {
    void test_output(const char *buf, const size_t buf_size)
    {
        const std::string buf_{buf, buf_size};

        mock().actualCall("test_output").withStringParameter("buf", buf_.c_str());
    }

    extern "C" bool mulog_config_mulog_lock(void)
    {
        mock().actualCall("mulog_config_mulog_lock");
        return mock().intReturnValue();
    }

    extern "C" void mulog_config_mulog_unlock(void)
    {
        mock().actualCall("mulog_config_mulog_unlock");
    }

    extern "C" unsigned long mulog_config_mulog_timestamp_get(void)
    {
        return 42123UL;
    }

    extern "C" void putchar_(int c)
    {
    }

    extern "C" int __real_vsnprintf_(char *s, size_t count, const char *fmt, va_list ap);

    extern "C" int __wrap_vsnprintf_(char *s, size_t count, const char *fmt, va_list ap)
    {
        UNUSED(s);
        UNUSED(count);
        UNUSED(fmt);
        UNUSED(ap);
        int value = mock().actualCall("vsnprintf").returnIntValue();

        if (value < 0) {
            return value;
        }

        value = __real_vsnprintf_(s, count, fmt, ap);

        return value;
    }

    extern "C" int __wrap_snprintf_(char *s, size_t count, const char *fmt, ...)
    {
        UNUSED(fmt);
        int value = mock().actualCall("snprintf").returnIntValue();

        if (value < 0) {
            return value;
        }

        va_list ap;

        va_start(ap, fmt);
        value = __real_vsnprintf_(s, count, fmt, ap);
        va_end(ap);

        return value;
    }
} // namespace

TEST_GROUP(MulogRealtimeLock)
{
    std::array<char, 1024> buffer;

    void setup() override
    {
        mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
        mock().expectOneCall("mulog_config_mulog_unlock");
        mulog_set_log_buffer(buffer.data(), buffer.size());
        mock().checkExpectations();
    }

    void teardown() override
    {
        mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
        mock().expectOneCall("mulog_config_mulog_unlock");
        mulog_reset();
        mock().clear();
    }
};

TEST(MulogRealtimeLock, SimpleOperations)
{
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_LOCK_FAILED, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    ret = mulog_add_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    ret = mulog_add_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_LOCK_FAILED, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_ERROR);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_TRACE);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_LOCK_FAILED, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    ret = mulog_unregister_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    ret = mulog_unregister_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_LOCK_FAILED, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_LOCK_FAILED, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    ret = mulog_unregister_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    ret = mulog_unregister_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_LOCK_FAILED, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    mulog_unregister_all_outputs();
    mock().checkExpectations();

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    mock().expectNoCall("mulog_config_mulog_unlock");
    mulog_unregister_all_outputs();
    mock().checkExpectations();

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    mock().expectNoCall("mulog_config_mulog_unlock");
    auto log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "Hello %s", "Temp");
    mock().checkExpectations();
    CHECK_EQUAL(0, log_ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    mock().expectNoCall("mulog_config_mulog_unlock");
    log_ret = mulog_deferred_process();
    mock().checkExpectations();
    CHECK_EQUAL(0, log_ret);
}

TEST(MulogRealtimeLock, MockLogWithLogMessageError)
{
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    ret = mulog_add_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("vsnprintf").andReturnValue(-1);
    mock().expectNCalls(2, "snprintf").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    const auto log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    mock().checkExpectations();
    CHECK_EQUAL(-1, log_ret);
}

TEST(MulogRealtimeLock, MockLog)
{
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    ret = mulog_add_output(test_output);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_OK, ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectNoCall("vsnprintf");
    mock().expectOneCall("snprintf").andReturnValue(-1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    mock().expectNoCall("test_output");
    auto log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    mock().checkExpectations();
    CHECK_EQUAL(-1, log_ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    mock().expectNoCall("vsnprintf");
    mock().expectOneCall("snprintf").andReturnValue(1);
    mock().expectOneCall("snprintf").andReturnValue(-1);
    mock().expectNoCall("test_output");
    log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    mock().checkExpectations();
    CHECK_EQUAL(-1, log_ret);

    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(1);
    mock().expectOneCall("mulog_config_mulog_unlock");
    mock().expectOneCall("snprintf").andReturnValue(1);
    mock().expectOneCall("snprintf").andReturnValue(1);
    mock().expectOneCall("vsnprintf").andReturnValue(1);
    mock().expectOneCall("snprintf").andReturnValue(-1);
    mock().expectNoCall("test_output");
    log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    mock().checkExpectations();
    CHECK_EQUAL(-1, log_ret);
}

int main(int argc, char **argv)
{
    return RUN_ALL_TESTS(argc, argv);
}
