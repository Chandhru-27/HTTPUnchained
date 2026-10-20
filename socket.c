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

    close(sock_fd);

    return 0;
}