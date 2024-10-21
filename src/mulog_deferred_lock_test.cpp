/**
 * \file
 * \brief
 * \author
 */
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
} // namespace

TEST_GROUP(MulogDeferredLock)
{
    std::array<char, 1024> buffer;

    void setup() override
    {
        mock().disable();
        mulog_set_log_buffer(buffer.data(), buffer.size());
        mock().enable();
    }

    void teardown() override
    {
        mock().disable();
        mulog_reset();
        mock().enable();
        mock().clear();
    }
};

TEST(MulogDeferredLock, SimpleOperations)
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
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_ERROR);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_UNSUPPORTED, ret);
    mock().expectOneCall("mulog_config_mulog_lock").andReturnValue(0);
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_TRACE);
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
    ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    mock().checkExpectations();
    CHECK_EQUAL(MULOG_RET_CODE_UNSUPPORTED, ret);
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

int main(int argc, char **argv)
{
    return RUN_ALL_TESTS(argc, argv);
}
