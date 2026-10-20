#ifndef HTTP_H
#define HTTP_H

int http_receive_request(int clientsock_fd);
int http_send_response(int clientsock_fd);

#endif