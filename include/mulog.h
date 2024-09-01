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
};

/**
 * \brief Function definition to be used by mulog for performing logging to a preferred interface/environment
 * \details mulog provides a log line string to this function, and it is up to a caller to send it properly to an
 * interface/environment.
 *
 * Currently, the return value is not checked, because there might be multiple output functions registered. It is not
 * clear how to handle error condition in one output function while the second one may finish correctly.
 */
typedef void (*mulog_log_output_fn)(const char *);

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
enum mulog_ret_code mulog_remove_output(mulog_log_output_fn output);

/**
 * \brief Remove all registered outputs
 */
void mulog_remove_all_outputs(void);

/**
 * \brief 
 * \param level
 * \param fmt 
 * \param ... 
 * \return 
 */
int mulog_log(enum mulog_log_level level, const char *fmt, ...) MULOG_PRINTF_ATTR;

/**
 * \brief 
 * \param fmt 
 */
#define MULOG_LOG_TRACE(fmt, ...) mulog_log(MULOG_LOG_LVL_TRACE, fmt, ##__VA_ARGS__);

/**
 * \brief 
 * \param fmt 
 */
#define MULOG_LOG_DBG(fmt, ...) mulog_log(MULOG_LOG_LVL_DEBUG, fmt, ##__VA_ARGS__);

/**
 * \brief 
 * \param fmt 
 */
#define MULOG_LOG_INFO(fmt, ...) mulog_log(MULOG_LOG_LVL_INFO, fmt, ##__VA_ARGS__);

/**
 * \brief 
 * \param fmt 
 */
#define MULOG_LOG_WARN(fmt, ...) mulog_log(MULOG_LOG_LVL_WARNING, fmt, ##__VA_ARGS__);

/**
 * \brief 
 * \param fmt 
 */
#define MULOG_LOG_ERR(fmt, ...) mulog_log(MULOG_LOG_LVL_ERROR, fmt, ##__VA_ARGS__);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MULOG_H */
