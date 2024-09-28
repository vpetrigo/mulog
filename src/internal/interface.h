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

enum mulog_ret_code interface_add_output_default(mulog_log_output_fn output);
enum mulog_ret_code interface_add_output(mulog_log_output_fn output,
                                         enum mulog_log_level log_level);
enum mulog_ret_code interface_set_log_buffer(char *log_buffer, size_t log_buffer_size);
enum mulog_ret_code interface_set_global_log_level(enum mulog_log_level log_level);
enum mulog_ret_code interface_set_log_level_per_output(enum mulog_log_level log_level,
                                                       mulog_log_output_fn output);
enum mulog_ret_code interface_unregister_output(mulog_log_output_fn output);
void interface_unregister_all_outputs(void);
void interface_reset(void);

int interface_log_output(enum mulog_log_level level, const char *fmt, va_list args);
int interface_deferred_log(void);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_H */
