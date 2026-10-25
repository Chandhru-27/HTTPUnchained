/**
 * The per-connection request/response cycle: receive one request,
 * parse and validate it, and send the response. One call handles
 * exactly one HTTP exchange on an already-accepted client socket.
 */
#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

/**
 * @brief Run one full request/response cycle on a connected client
 *        socket. Does not close clientsock_fd; the caller owns its
 *        lifetime.
 * @param clientsock_fd Socket returned by net_create_clientsock().
 * @return STATUS_EXEC_SUCCESS if a response was sent, STATUS_FATAL_ERROR
 *         if reception, parsing, validation, or sending failed.
 */
int handle_connection(int clientsock_fd);

#endif
