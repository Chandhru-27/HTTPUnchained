#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8000
#define BACKLOG 10


int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0) {
        printf("Socket creation failed!"); 
        return 1;
    }
    
    printf("Socket succesfully created with descriptior: %d\n", sock_fd);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    printf("The address blueprint is filled out and ready to use!\n");

    printf("The address: IP = %d (AF_INET)\n", address.sin_family);
    printf("The address: Listening on = %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    
    if (bind(sock_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Bind failed! The port might already be in use.\n");
        return 1;
    }

    printf("Successfully bound to port 8080!\n");

    if (listen(sock_fd, BACKLOG) < 0) {
        printf("Listen failed!\n");
        return 1;
    }

    printf("Server is now listening on port 8000... Waiting room queue set to %d.\n",BACKLOG);

    socklen_t addrlen = sizeof(address);
    int nsock_fd = accept(sock_fd, (struct sockaddr *)&address, &addrlen);

    if (nsock_fd < 0) {
        printf("Accept failed!\n");
        return 1;
    }
    
    printf("Connection accepted! A client has successfully connected to us.\n");

    char buffer[1024];
    size_t total_received = 0;

    while (true) {
        if (total_received >= 1023) {
            printf("Payload too heavy: Buffer overloaded!\n");
            return 1;
        }

        ssize_t bytes_read = recv(nsock_fd, buffer + total_received, sizeof(buffer) - total_received - 1, 0);

        if (bytes_read < 0) {
            printf("Failed to recieve from client!\n");
            return 1;    
        } else if (bytes_read == 0) {
            printf("Orderly shutdown: Client disconnected!\n");
            return 1;
        } 

        total_received += bytes_read;

        buffer[total_received] = '\0';

        char *end = strstr(buffer, "\r\n\r\n");

        if (end != NULL) {
            printf("End of HTTP headers found!\n");
            break;
        }
    } 
    
    printf("Received from client: %s\n", buffer);

    char *reply = "Message received loud and clear!\n";

    ssize_t bytes_sent = send(nsock_fd, reply, strlen(reply), 0);

    if (bytes_sent < 0) {
        printf("Failed to send data to client!\n");
        return 1;
    }

    printf("Reply sent successfully.\n");
    close(nsock_fd);
    close(sock_fd);

    return 0;
}