/**
 * \file
 * \brief Supplementary utils for the mulog implementation
 * \author Vladimir Petrigo
 */

#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MULOG_ENABLE_COLOR) && MULOG_ENABLE_COLOR == 1
#include "color.h"
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define UNUSED(x) ((void)(x))

#define MULOG_TRACE   "[TRC]"
#define MULOG_DEBUG   "[DBG]"
#define MULOG_INFO    "[INF]"
#define MULOG_WARNING "[WRN]"
#define MULOG_ERROR   "[ERR]"

#if defined(MULOG_ENABLE_COLOR) && MULOG_ENABLE_COLOR == 1
#define MULOG_COLOR_TRACE   MULOG_COLOR_MAG MULOG_TRACE MULOG_COLOR_RESET
#define MULOG_COLOR_DEBUG   MULOG_COLOR_BLU MULOG_DEBUG MULOG_COLOR_RESET
#define MULOG_COLOR_INFO    MULOG_COLOR_GRN MULOG_INFO MULOG_COLOR_RESET
#define MULOG_COLOR_WARNING MULOG_COLOR_YEL MULOG_WARNING MULOG_COLOR_RESET
#define MULOG_COLOR_ERROR   MULOG_COLOR_RED MULOG_ERROR MULOG_COLOR_RESET

#define MULOG_TRACE_LVL   MULOG_COLOR_TRACE
#define MULOG_DEBUG_LVL   MULOG_COLOR_DEBUG
#define MULOG_INFO_LVL    MULOG_COLOR_INFO
#define MULOG_WARNING_LVL MULOG_COLOR_WARNING
#define MULOG_ERROR_LVL   MULOG_COLOR_ERROR
#else
#define MULOG_TRACE_LVL   MULOG_TRACE
#define MULOG_DEBUG_LVL   MULOG_DEBUG
#define MULOG_INFO_LVL    MULOG_INFO
#define MULOG_WARNING_LVL MULOG_WARNING
#define MULOG_ERROR_LVL   MULOG_ERROR
#endif

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
