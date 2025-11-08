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
    class API {
    public:
        MAKE_MOCK0(mulog_config_mulog_lock, bool(void));
        MAKE_MOCK0(mulog_config_mulog_unlock, void(void));
        MAKE_MOCK2(test_output, void(const char *, const size_t));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
        MAKE_MOCK4(__wrap_vsnprintf_, int(char *, size_t, const char *, va_list));
#pragma GCC diagnostic pop
    };

    API api;

    void test_output(const char *buf, const size_t buf_size)
    {
        const std::string buf_{buf, buf_size};

        api.test_output(buf_.c_str(), buf_.size());
    }

    extern "C" {
    bool mulog_config_mulog_lock(void)
    {
        return api.mulog_config_mulog_lock();
    }

    void mulog_config_mulog_unlock(void)
    {
        api.mulog_config_mulog_unlock();
    }

    unsigned long mulog_config_mulog_timestamp_get(void)
    {
        return 42123UL;
    }

    void putchar_(int c)
    {
    }

    int __real_vsnprintf_(char *s, size_t count, const char *fmt, va_list ap);

    int __wrap_vsnprintf_(char *s, size_t count, const char *fmt, va_list ap)
    {
        int value = api.__wrap_vsnprintf_(s, count, fmt, ap);

        if (value < 0) {
            return value;
        }

        value = __real_vsnprintf_(s, count, fmt, ap);

        return value;
    }

    int __wrap_snprintf_(char *s, size_t count, const char *fmt, ...)
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
    }
} // namespace

class MulogRealtime {
public:
    MulogRealtime()
    {
        REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
        REQUIRE_CALL(api, mulog_config_mulog_unlock());
        mulog_set_log_buffer(buffer.data(), buffer.size());
    }

    ~MulogRealtime()
    {
        REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
        REQUIRE_CALL(api, mulog_config_mulog_unlock());
        mulog_reset();
    }

private:
    std::array<char, 1024> buffer;
};

TEST_CASE_METHOD(MulogRealtime, "Simple Operations", "[realtime]")
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
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_LOCK_FAILED == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(false);
    ret = mulog_set_channel_log_level(test_output, MULOG_LOG_LVL_TRACE);
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
    ret = mulog_add_output_with_log_level(test_output, MULOG_LOG_LVL_ERROR);
    REQUIRE(MULOG_RET_CODE_OK == ret);
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

    log_ret = mulog_deferred_process();
    REQUIRE(MULOG_RET_CODE_UNSUPPORTED == log_ret);
}

TEST_CASE_METHOD(MulogRealtime, "Log with log message error", "[realtime]")
{
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api,
                 __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_))
        .RETURN(-1);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    const auto log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    REQUIRE(-1 == log_ret);
}

TEST_CASE_METHOD(MulogRealtime, "Normal logging", "[realtime]")
{
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    auto ret = mulog_set_log_level(MULOG_LOG_LVL_TRACE);
    REQUIRE(MULOG_RET_CODE_OK == ret);
    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    ret = mulog_add_output(test_output);
    REQUIRE(MULOG_RET_CODE_OK == ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api,
                 __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_))
        .RETURN(-1);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    FORBID_CALL(api, test_output(trompeloeil::_, trompeloeil::_));
    auto log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    REQUIRE(-1 == log_ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    REQUIRE_CALL(api,
                 __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_))
        .TIMES(1)
        .RETURN(-1);
    REQUIRE_CALL(api,
                 __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_))
        .TIMES(2)
        .RETURN(1);
    FORBID_CALL(api, test_output(trompeloeil::_, trompeloeil::_));
    log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    REQUIRE(-1 == log_ret);

    REQUIRE_CALL(api, mulog_config_mulog_lock()).RETURN(true);
    REQUIRE_CALL(api, mulog_config_mulog_unlock());
    REQUIRE_CALL(api,
                 __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_))
        .RETURN(-1);
    REQUIRE_CALL(api,
                 __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_))
        .RETURN(1);
    REQUIRE_CALL(api,
                 __wrap_vsnprintf_(trompeloeil::_, trompeloeil::_, trompeloeil::_, trompeloeil::_))
        .RETURN(1);
    FORBID_CALL(api, test_output(trompeloeil::_, trompeloeil::_));
    log_ret = MULOG_LOG_DBG("Hello %s", "Temp");
    REQUIRE(-1 == log_ret);
}
