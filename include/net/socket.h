/**
 * IPv4 TCP socket setup for the server: creating the listening socket,
 * binding it, listening on it, and accepting client connections. These
 * functions know nothing about HTTP; they are plain BSD-socket wrappers
 * that return STATUS_FATAL_ERROR (see core/errors.h) on failure.
 */
#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <stdint.h>

/**
 * @brief Create an IPv4 TCP socket with SO_REUSEADDR enabled.
 * @return The socket file descriptor, or STATUS_FATAL_ERROR on failure.
 */
int net_create_serversock(void);

/**
 * @brief Bind a server socket to the configured PORT on all interfaces.
 * @param serversock_fd Socket returned by net_create_serversock().
 * @return STATUS_EXEC_SUCCESS on success, STATUS_FATAL_ERROR on failure.
 */
int8_t net_bind_serversock(int serversock_fd);

/**
 * @brief Start listening on a bound server socket with the configured
 *        BACKLOG.
 * @param serversock_fd Socket returned by net_create_serversock().
 * @return STATUS_EXEC_SUCCESS on success, STATUS_FATAL_ERROR on failure.
 */
int8_t net_listen_on_serversock(int serversock_fd);

/**
 * @brief Block until a client connects, then accept the connection.
 * @param serversock_fd Listening socket returned by
 *        net_listen_on_serversock().
 * @return The connected client's socket file descriptor, or
 *         STATUS_FATAL_ERROR on failure.
 */
int net_create_clientsock(int serversock_fd);

#endif
