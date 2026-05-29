#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>       // fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>       // errno, EAGAIN, EWOULDBLOCK

void make_nonblocking(int fd) {
    // fcntl = "file control" — get/set flags on any fd
    // F_GETFL = "get file status flags" — returns current flags as int
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) { perror("fcntl get"); return; }
    
    // F_SETFL = "set file status flags"
    // flags | O_NONBLOCK = add the O_NONBLOCK bit to existing flags
    // Don't just set O_NONBLOCK alone — that clears all other flags!
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl set"); return;
    }
    // After this call, ALL operations on fd are non-blocking:
    // recv() with no data → returns -1, errno=EAGAIN (don't block)
    // send() with full buffer → returns -1, errno=EAGAIN (don't block)
    // accept() with no connections → returns -1, errno=EAGAIN
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);
    
    make_nonblocking(server_fd);
    
    // The PROBLEM with naive non-blocking — busy polling:
    int attempts = 0;
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection ready — but we don't block!
                // We spin and try again immediately
                attempts++;
                if (attempts % 1000000 == 0) {
                    printf("Attempted accept() %d times, no connections yet\n", attempts);
                    // This is burning CPU doing NOTHING useful
                    // This is called "busy waiting" or "spin-polling"
                    // CPU runs at 100% but accomplishes nothing
                }
                continue;
            }
            perror("accept");
            break;
        }
        
        printf("Client connected! fd=%d after %d polls\n", client_fd, attempts);
        close(client_fd);
        attempts = 0;
    }
    
    // We need something BETTER than spin-polling:
    // "Tell me when a fd is ready, then I'll call read/accept"
    // That's what select/poll/epoll provide
}
