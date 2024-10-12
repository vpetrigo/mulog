/**
 * \file
 * \brief mulog generic interface for various logging modes
 * \author Vladimir Petrigo
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mulog.h"

#include <stdarg.h>
#include <stddef.h>

/**
 * \brief Adds a default output function to the logging interface with the global log level.
 *
 * \param output The output function to be added.
 * \return Status code indicating the result of the operation.
 */
enum mulog_ret_code interface_add_output_default(mulog_log_output_fn output);

/**
 * \brief Adds a specific output function to the logging interface with the specified log level.
 *
 * \param output The output function to be added.
 * \param log_level The log level associated with the output function.
 * \return Status code indicating the result of the operation.
 */
enum mulog_ret_code interface_add_output(mulog_log_output_fn output,
                                         enum mulog_log_level log_level);

/**
 * \brief Sets the log buffer for the logging interface.
 *
 * \param log_buffer Pointer to the buffer where log entries will be stored.
 * \param log_buffer_size Size of the log buffer in bytes.
 * \return Status code indicating the result of the operation.
 */
enum mulog_ret_code interface_set_log_buffer(char *log_buffer, size_t log_buffer_size);

/**
 * \brief Sets the global log level for the logging interface.
 *
 * \param log_level The log level to be set globally. It should be a valid value from the `mulog_log_level` enum.
 * \return Status code indicating the result of the operation. Returns `MULOG_RET_CODE_OK` on success or
 *         `MULOG_RET_CODE_INVALID_ARG` if the provided `log_level` is invalid.
 */
enum mulog_ret_code interface_set_global_log_level(enum mulog_log_level log_level);

/**
 * \brief Sets the log level for a specified output function.
 *
 * \param log_level The log level to be set for the specified output.
 * \param output The output function for which the log level is to be set.
 * \return Status code indicating the result of the operation.
 */
enum mulog_ret_code interface_set_log_level_per_output(enum mulog_log_level log_level,
                                                       mulog_log_output_fn output);

/**
 * \brief Unregisters a previously registered output function from the logging interface.
 *
 * \param output The output function to be removed.
 * \return Status code indicating the result of the unregistration operation.
 */
enum mulog_ret_code interface_unregister_output(mulog_log_output_fn output);

/**
 * \brief Unregisters all output functions from the logging interface.
 *
 * Removes all registered output functions, ensuring that no functions
 * will be called to handle log messages.
 */
void interface_unregister_all_outputs(void);
/**
 * \brief Resets the interface to its initial state.
 *
 * This function performs a complete reset of the interface, clearing any
 * configuration or state that may have been previously set.
 */
void interface_reset(void);

/**
 * \brief Outputs a log message at the specified log level with formatting.
 *
 * This function handles the output of log messages according to the specified
 * log level and format string. It processes the timestamp and log level information,
 * formats the message, and writes it to the ring buffer.
 *
 * \param level The log level at which the message should be output.
 * \param fmt The format string for the log message.
 * \param args The arguments for the format string.
 * \return The number of bytes written to the ring buffer, or zero on error.
 */
int interface_log_output(enum mulog_log_level level, const char *fmt, va_list args);

/**
 * \brief Logs deferred messages using the interface's logging mechanism.
 *
 * \return Status code indicating that the operation is unsupported.
 */
int interface_deferred_log(void);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_H */
