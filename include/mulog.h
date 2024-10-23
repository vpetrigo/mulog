/**
 * \file
 * \brief mulog Public interface declaration
 * \author Vladimir Petrigo
 */

#ifndef MULOG_H
#define MULOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * \addtogroup mulog mulog Public API
 * @{
 */

#define MULOG_PRINTF_ATTR                                                                          \
    __attribute__((format(printf, 2, 3))) /**< Printf-like function attribute */

/**
 * \brief Log levels
 */
enum mulog_log_level {
    MULOG_LOG_LVL_TRACE,   /**< Trace log level */
    MULOG_LOG_LVL_DEBUG,   /**< Debug log level */
    MULOG_LOG_LVL_INFO,    /**< Info log level */
    MULOG_LOG_LVL_WARNING, /**< Warning log level */
    MULOG_LOG_LVL_ERROR,   /**< Error log level */
    // private
    MULOG_LOG_LVL_COUNT,
};

enum mulog_ret_code {
    MULOG_RET_CODE_OK = 0,
    MULOG_RET_CODE_NO_MEM = -1,
    MULOG_RET_CODE_NOT_FOUND = -2,
    MULOG_RET_CODE_INVALID_ARG = -3,
    MULOG_RET_CODE_UNSUPPORTED = -4,
    MULOG_RET_CODE_LOCK_FAILED = -5,
};

/**
 * \brief Function definition to be used by mulog for performing logging to a preferred interface/environment
 * \details mulog provides a log line string to this function, and it is up to a caller to send it properly to an
 * interface/environment.
 *
 * Currently, the return value is not checked, because there might be multiple output functions registered. It is not
 * clear how to handle error condition in one output function while the second one may finish correctly.
 */
typedef void (*mulog_log_output_fn)(const char *, size_t);

/**
 * \brief Set log buffer to be used for formatting log lines
 * \param[in] buf Logging buffer storage
 * \param[in] buf_size Size of the buffer storage
 */
enum mulog_ret_code mulog_set_log_buffer(char *buf, size_t buf_size);

/**
 * \brief Set logger global log level
 * \param[in] level Log level below which log calls are ignored
 */
enum mulog_ret_code mulog_set_log_level(enum mulog_log_level level);

/**
 * \brief Set log level per output function
 * \details mulog_set_log_level() allows to set up the global log level which may be overwritten by this
 * call for a specified output function. Might be useful if you would like to overwrite and make output more/less
 * verbose only for a particular interface/environment
 * \param[in] output Output function to adjust log level for
 * \param[in] level Log level below which log calls are ignored
 */
enum mulog_ret_code mulog_set_channel_log_level(mulog_log_output_fn output,
                                                enum mulog_log_level level);

/**
 * \brief Add output function that will be used for logging
 * \param[in] output Logging output function
 */
enum mulog_ret_code mulog_add_output(mulog_log_output_fn output);

/**
 * \brief Add output function that will be used for logging
 * \param[in] output Logging output function
 * \param[in] level Log level to set for the channel
 */
enum mulog_ret_code mulog_add_output_with_log_level(mulog_log_output_fn output,
                                                    enum mulog_log_level level);

/**
 * \brief Remove the given output function from the logger
 * \param[in] output Output function
 */
enum mulog_ret_code mulog_unregister_output(mulog_log_output_fn output);

/**
 * \brief Remove all registered outputs
 */
void mulog_unregister_all_outputs(void);

/**
 * \brief Reset mulog module
 */
void mulog_reset(void);

/**
 * \brief Processes deferred log entries.
 *
 * This function handles the deferred logging mechanism by
 * executing any pending log operations that were deferred.
 *
 * \warning This function must be called by a single log consumer, as it does not include
 * any locking mechanisms.
 * It interacts with the underlying circular buffer to read as much data as possible and sends that
 * data to the registered outputs.
 *
 * \return Integer status code. Typically, returns the value from
 *         the underlying deferred log interface routine.
 */
int mulog_deferred_process(void);

/**
 * \brief Logs messages at the specified log level
 *
 * This function logs messages with a specified format string and additional
 * arguments similar to printf. The log level determines the severity of the
 * log message.
 *
 * \param level The log level specified by the enum mulog_log_level
 * \param fmt The format string for the log message, similar to printf
 * \param ... Additional arguments for the format string
 * \return The result of the logging operation, where 0 indicates success
 */
int mulog_log(enum mulog_log_level level, const char *fmt, ...) MULOG_PRINTF_ATTR;

/**
 * \brief Logs a message with trace level.
 *
 * This macro logs a trace level message using the specified format string.
 *
 * \param fmt The format string (printf-style).
 * \param ... Additional arguments for the format string.
 *
 * Example usage:
 * \code{.c}
 * MULOG_LOG_TRACE("This is a trace message, value: %d", 42);
 * \endcode
 */
#define MULOG_LOG_TRACE(fmt, ...) mulog_log(MULOG_LOG_LVL_TRACE, fmt, ##__VA_ARGS__)

/**
 * \brief Logs a message with debug level.
 *
 * This macro logs a debug level message using the specified format string.
 *
 * \param fmt The format string (printf-style).
 * \param ... Additional arguments for the format string.
 *
 * Example usage:
 * \code{.c}
 * MULOG_LOG_DBG("This is a debug message, value: %d", 42);
 * \endcode
 */
#define MULOG_LOG_DBG(fmt, ...) mulog_log(MULOG_LOG_LVL_DEBUG, fmt, ##__VA_ARGS__)

/**
 * \brief Logs a message with info level.
 *
 * This macro logs an info level message using the specified format string.
 *
 * \param fmt The format string (printf-style).
 * \param ... Additional arguments for the format string.
 *
 * Example usage:
 * \code{.c}
 * MULOG_LOG_INFO("This is an info message, value: %d", 42);
 * \endcode
 */
#define MULOG_LOG_INFO(fmt, ...) mulog_log(MULOG_LOG_LVL_INFO, fmt, ##__VA_ARGS__)

/**
 * \brief Logs a message with warning level.
 *
 * This macro logs a warning level message using the specified format string.
 *
 * \param fmt The format string (printf-style).
 * \param ... Additional arguments for the format string.
 *
 * Example usage:
 * \code{.c}
 * MULOG_LOG_WARN("This is a warning message, value: %d", 42);
 * \endcode
 */
#define MULOG_LOG_WARN(fmt, ...) mulog_log(MULOG_LOG_LVL_WARNING, fmt, ##__VA_ARGS__)

/**
 * \brief Logs a message with error level.
 *
 * This macro logs an error level message using the specified format string.
 *
 * \param fmt The format string (printf-style).
 * \param ... Additional arguments for the format string.
 *
 * Example usage:
 * \code{.c}
 * MULOG_LOG_ERR("This is an error message, value: %d", 42);
 * \endcode
 */
#define MULOG_LOG_ERR(fmt, ...) mulog_log(MULOG_LOG_LVL_ERROR, fmt, ##__VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MULOG_H */
