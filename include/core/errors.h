/**
 * Process and function-level status codes shared across the codebase.
 * `STATUS_*_EXIT_*` map onto the C standard library exit codes; the
 * `STATUS_*EXEC*`/`STATUS_FATAL_ERROR` pair is the int8_t-sized
 * success/failure convention used by functions that are not themselves
 * parsing requests (socket I/O, network setup).
 */
#ifndef CORE_ERRORS_H
#define CORE_ERRORS_H

#include <stdlib.h>

#define STATUS_FATAL_ERROR -1
#define STATUS_EXIT_FAILURE EXIT_FAILURE

#define STATUS_EXEC_SUCCESS 0
#define STATUS_EXIT_SUCCESS EXIT_SUCCESS

#endif
