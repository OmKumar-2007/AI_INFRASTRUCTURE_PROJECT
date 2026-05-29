#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>  // fd_set, FD_ZERO, FD_SET, FD_ISSET, select()
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 100

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("select() server on port %d\n", PORT);
    
    // Track all client fds in an array
    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;
    int max_fd = server_fd;  // select() needs to know the highest fd
    
    while (1) {
        // fd_set is a BITMASK — one bit per fd
        // Maximum fd is FD_SETSIZE = 1024 on most systems
        // This is the #1 limitation of select()
        fd_set read_fds;
        FD_ZERO(&read_fds);           // clear all bits
        FD_SET(server_fd, &read_fds);  // watch server fd (new connections)
        
        // Add all active clients to the watch set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1) {
                FD_SET(clients[i], &read_fds);
                if (clients[i] > max_fd) max_fd = clients[i];
            }
        }
        // ↑ We REBUILD this set on EVERY iteration — select() modifies it!
        // This is O(n) per call just to set up the call itself
        
        // BLOCK until at least one fd is ready
        // Arguments:
        // 1. max_fd + 1: check fds 0 through max_fd
        // 2. &read_fds: which fds to watch for READ readiness
        // 3. NULL: don't watch for WRITE readiness
        // 4. NULL: don't watch for EXCEPTION readiness
        // 5. NULL: block forever (pass struct timeval* for timeout)
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0) { perror("select"); break; }
        
        // Check if NEW connection is ready
        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, NULL, NULL);
            // Store in our array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == -1) { clients[i] = client_fd; break; }
            }
            printf("New client: fd=%d\n", client_fd);
        }
        
        // Check ALL clients for data — O(n) scan
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1) continue;
            if (FD_ISSET(clients[i], &read_fds)) {
                // THIS client has data ready
                char buf[1024];
                ssize_t n = recv(clients[i], buf, sizeof(buf)-1, 0);
                if (n <= 0) {
                    close(clients[i]);
                    clients[i] = -1;
                } else {
                    buf[n] = '\0';
                    printf("[fd %d]: %s", clients[i], buf);
                    send(clients[i], buf, n, 0);
                }
            }
        }
        // ↑ We scan ALL 100 client slots every time
        // Even if only 1 client sent data
        // At 1024 clients: check 1024 slots to find 1 ready fd = O(n)
    }
}
