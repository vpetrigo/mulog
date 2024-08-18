/**
 * \file
 * \brief
 * \author Vladimir Petrigo
 */

#ifndef MULOG_H
#define MULOG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup mulog mulog Public API
 * @{
 */
#define MULOG_PRINTF_ATTR __attribute__((format(printf, 2, 3)))

enum mulog_log_level {
    MULOG_LOG_LVL_TRACE,
    MULOG_LOG_LVL_DEBUG,
    MULOG_LOG_LVL_INFO,
    MULOG_LOG_LVL_WARNING,
    MULOG_LOG_LVL_ERROR,
};

typedef int (*mulog_log_output_fn)(const char *);

void mulog_set_log_level(enum mulog_log_level level);
void mulog_add_output(mulog_log_output_fn output);
void mulog_remove_output(mulog_log_output_fn output);
int mulog_log(enum mulog_log_level level, const char *fmt, ...) MULOG_PRINTF_ATTR;

#define MULOG_LOG_TRACE(fmt, ...) mulog_log(MULOG_LOG_LVL_TRACE, fmt, ##__VA_ARGS__);
#define MULOG_LOG_DBG(fmt, ...)   mulog_log(MULOG_LOG_LVL_DEBUG, fmt, ##__VA_ARGS__);
#define MULOG_LOG_INFO(fmt, ...)  mulog_log(MULOG_LOG_LVL_INFO, fmt, ##__VA_ARGS__);
#define MULOG_LOG_WARN(fmt, ...)  mulog_log(MULOG_LOG_LVL_WARNING, fmt, ##__VA_ARGS__);
#define MULOG_LOG_ERR(fmt, ...)   mulog_log(MULOG_LOG_LVL_ERROR, fmt, ##__VA_ARGS__);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MULOG_H */
