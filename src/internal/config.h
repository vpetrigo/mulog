/**
 * \file
 * \brief mulog configuration
 * \author Vladimir Petrigo
 */

#ifndef CONFIG_H
#define CONFIG_H

#define MULOG_ENABLE_TIMESTAMP (MULOG_INTERNAL_ENABLE_COLOR_OUTPUT) /**< Add timestamp to the log line */
#define MULOG_ENABLE_COLOR     (MULOG_INTERNAL_ENABLE_TIMESTAMP_OUTPUT) /**< Enable color log lines */
#define MULOG_OUTPUT_HANDLERS  (MULOG_INTERNAL_OUTPUT_HANDLERS) /**< Maximum number of output handlers that can be registered */

#endif /* CONFIG_H */
