/**
 * Wire-level I/O for a single HTTP exchange: receiving a bounded raw
 * request and sending the (currently fixed) response. Neither function
 * parses HTTP semantics; they only move bytes over the socket.
 */
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Receive request bytes from a client socket into `buffer`
 *        until the header terminator (CRLFCRLF) is seen or `capacity`
 *        is exhausted.
 * @param clientsock_fd Connected client socket.
 * @param buffer Destination buffer, NUL-terminated on return.
 * @param capacity Size of buffer, including space for the terminating
 *        NUL.
 * @param request_length Output; number of bytes received, excluding
 *        the terminating NUL.
 * @return STATUS_EXEC_SUCCESS on success, STATUS_FATAL_ERROR on
 *         failure (disconnect, recv error, or buffer exhausted before
 *         the header terminator was seen).
 */
int8_t http_receive_request(int clientsock_fd, char* buffer, size_t capacity,
                            size_t* request_length);

/**
 * @brief Send a fixed "200 OK" plain-text response to a client.
 * @param clientsock_fd Connected client socket.
 * @return STATUS_EXEC_SUCCESS on success, STATUS_FATAL_ERROR on
 *         failure.
 */
int8_t http_send_response(int clientsock_fd);

#endif
