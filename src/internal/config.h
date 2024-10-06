/**
 * \file
 * \brief mulog configuration
 * \author Vladimir Petrigo
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * \brief Flag that specify whether to add timestamp to the log line
 */
#define MULOG_ENABLE_TIMESTAMP (MULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT)

#define MULOG_ENABLE_LOCKING (0)

/**
 * \brief Flag that specify whether to enable color log lines
 */
#define MULOG_ENABLE_COLOR (MULOG_INTERNAL_ENABLE_COLOR_OUTPUT)

/**
 * \brief Maximum number of output handlers that can be registered
 */
#define MULOG_OUTPUT_HANDLERS (MULOG_INTERNAL_OUTPUT_HANDLERS)

/**
 * \brief Log line termination
 */
#define MULOG_LOG_LINE_TERMINATION "\n"

/**
 * \brief External function that is used for getting time value
 * \return Current system time value in milliseconds
 */
extern unsigned long mulog_config_mulog_timestamp_get(void);

/**
 * \brief External function that is used for locking logger in a multi-thread environment
 * \return Status of the lock operation
 * \retval true Lock has been successfully acquired
 * \retval false Lock has not been acquired
 */
extern bool mulog_config_mulog_lock(void);

/**
 * \brief External function that is used for unlocking logger in a multi-thread environment
 */
extern void mulog_config_mulog_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
