#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    setbuf(stdout, NULL);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_sock);
        exit(2);
    }

    if(listen(listen_sock, 5) < 0) {
        perror("listen");
        close(listen_sock);
        exit(3);
    }
    printf("Server is listening on port 8080...\n");

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int service_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);

        if(service_sock < 0) {
            perror("accept");
            continue;
        }
        printf("new client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        close(service_sock);
    }

    close(listen_sock);

    return 0;
}