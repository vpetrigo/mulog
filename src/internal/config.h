/**
 * \file
 * \brief mulog configuration
 * \author Vladimir Petrigo
 */

#ifndef CONFIG_H
#define CONFIG_H

/**
 * \brief Flag that specify whether to add timestamp to the log line
 */
#define MULOG_ENABLE_TIMESTAMP (MULOG_INTERNAL_ENABLE_COLOR_OUTPUT)

/**
 * \brief Flag that specify whether to enable color log lines
 */
#define MULOG_ENABLE_COLOR (MULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT)

/**
 * \brief Maximum number of output handlers that can be registered
 */
#define MULOG_OUTPUT_HANDLERS (MULOG_INTERNAL_OUTPUT_HANDLERS)

/**
 * \brief Log line termination
 */
#define MULOG_LOG_LINE_TERMINATION "\n"

#endif /* CONFIG_H */
