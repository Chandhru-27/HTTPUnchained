#ifndef SERVER_H
#define SERVER_H

int net_create_serversock(void);
int net_bind_serversock(int serversock_fd);
int net_listen_on_serversock(int serversock_fd);
int net_create_clientsock(int serversock_fd);

#endif