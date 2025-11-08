/**
 * \file
 * \brief
 * \author
 */
#include "internal/utils.h"
#include "mulog.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include <array>
#include <cstdarg>

namespace {
    class ApiMock {
    public:
        MAKE_MOCK0(mulog_config_mulog_lock, bool());
        MAKE_MOCK0(mulog_config_mulog_unlock, void());
        MAKE_MOCK2(test_output, void(const char *, const size_t));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
        MAKE_MOCK4(__wrap_vsnprintf_, int(char *, size_t, const char *, va_list));
#pragma GCC diagnostic pop
        MAKE_MOCK1(__wrap_lwrb_get_full, size_t(const void *));
        MAKE_MOCK1(__wrap_lwrb_get_linear_block_read_length, size_t(const void *));
    };

    ApiMock api;

    void test_output(const char *buf, const size_t buf_size)
    {
        api.test_output(buf, buf_size);
    }

    extern "C" bool mulog_config_mulog_lock(void)
    {
        return api.mulog_config_mulog_lock();
    }

    extern "C" void mulog_config_mulog_unlock(void)
    {
        api.mulog_config_mulog_unlock();
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
        int value = api.__wrap_vsnprintf_(s, count, fmt, ap);

        if (value < 0) {
            return value;
        }

        value = __real_vsnprintf_(s, count, fmt, ap);

        return value;
    }

    extern "C" int __wrap_snprintf_(char *s, size_t count, const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        int value = api.__wrap_vsnprintf_(s, count, fmt, ap);
        va_end(ap);

        if (value < 0) {
            return value;
        }

        va_list ap2;

        va_start(ap2, fmt);
        value = __real_vsnprintf_(s, count, fmt, ap);
        va_end(ap2);

        return value;
    }

    extern "C" size_t __wrap_lwrb_get_full(const void *buff)
    {
        return api.__wrap_lwrb_get_full(buff);
    }

    extern "C" size_t __wrap_lwrb_get_linear_block_read_length(const void *buff)
    {
        return api.__wrap_lwrb_get_linear_block_read_length(buff);
    }
} // namespace

class MulogDeferredLock {
public:
    std::array<char, 1024> buffer{};

    MulogDeferredLock()
    {
        REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
        REQUIRE_CALL(api, mulog_config_mulog_unlock());
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~MulogDeferredLock()
    {
        REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
        REQUIRE_CALL(api, mulog_config_mulog_unlock());
        mulog_reset();
    }
};

TEST_CASE_METHOD(MulogDeferredLock, "MulogDeferredLock - SimpleOperations", "[deferred][lock]")
{
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_LOCK_FAILED == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_UNSUPPORTED == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_LOCK_FAILED == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_LOCK_FAILED == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_UNSUPPORTED == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_LOCK_FAILED == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    ret = mulog_unregister_output(test_output);
    REQUIRE(MULOG_RET_CODE_LOCK_FAILED == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    mulog_unregister_all_outputs();

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    FORBID_CALL(api, mulog_config_mulog_unlock());
    mulog_unregister_all_outputs();

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    FORBID_CALL(api, mulog_config_mulog_unlock());
    auto log_ret = mulog_log(MULOG_LOG_LVL_ERROR, "Hello %s", "Temp");
    REQUIRE(0 == log_ret);

    REQUIRE_CALL(api, __wrap_lwrb_get_full(trompeloeil::_)).RETURN(0UL);
    log_ret = mulog_deferred_process();
    REQUIRE(0 == log_ret);
}

// TEST_CASE_METHOD(MulogDeferredLock, "MulogDeferredLock - MockLog", "[deferred][lock]")
// {
//     REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
//     REQUIRE_CALL(api, mulog_config_mulog_unlock());
//     auto ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
//     REQUIRE(MULOG_RET_CODE_OK == ret);
//     REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
//     REQUIRE_CALL(api, mulog_config_mulog_unlock());
//     ret = mulog_add_output(test_output);
//     REQUIRE(MULOG_RET_CODE_OK == ret);
//
//     REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
//     REQUIRE_CALL(api, mulog_config_mulog_unlock());
//     FORBID_CALL(api, __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_));
//     REQUIRE_CALL(api, __wrap_vnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_)).RETURN(-1);
//     FORBID_CALL(api, test_output(trompeloeil::_, trompeloeil::_));
//     auto log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
//     REQUIRE(0 == log_ret);
//
//     REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
//     REQUIRE_CALL(api, mulog_config_mulog_unlock());
//     REQUIRE_CALL(api, __wrap_snprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_)).RETURN(1);
//     REQUIRE_CALL(api, __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_)).RETURN(-1);
//     FORBID_CALL(api, test_output(trompeloeil::_, trompeloeil::_));
//     log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
//     REQUIRE(-1 == log_ret);
// }

TEST_CASE_METHOD(MulogDeferredLock, "MulogDeferredLock - MockLogDeferredProcess", "[deferred][lock]")
{
    REQUIRE_CALL(api, __wrap_lwrb_get_full(trompeloeil::_)).RETURN(100UL);
    REQUIRE_CALL(api, __wrap_lwrb_get_linear_block_read_length(trompeloeil::_)).RETURN(200UL);
    auto ret = mulog_deferred_process();
    REQUIRE(100UL == ret);

    REQUIRE_CALL(api, __wrap_lwrb_get_full(trompeloeil::_)).RETURN(300UL);
    REQUIRE_CALL(api, __wrap_lwrb_get_linear_block_read_length(trompeloeil::_)).RETURN(300UL);
    ret = mulog_deferred_process();
    REQUIRE(300UL == ret);
}
