#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#define PORT        8080
#define MAX_EVENTS  64     // max events per epoll_wait call
#define MAX_CLIENTS 1024   // max simultaneous connections
#define BUF_SIZE    4096   // read buffer size per client

// Per-client state
// In a real server this would be more complex (partial writes, etc.)
struct Client {
    int fd;                  // -1 = slot is empty
    int msg_count;           // messages received from this client
    long bytes_received;     // total bytes
};

struct Client clients[MAX_CLIENTS];
int client_count = 0;

// Make fd non-blocking
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Register fd with epoll instance
void epoll_add(int epoll_fd, int fd, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;   // store fd in .data — we get it back in events
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl ADD");
    }
}

// Find slot for new client or return -1 if full
int add_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = fd;
            clients[i].msg_count = 0;
            clients[i].bytes_received = 0;
            client_count++;
            return i;  // return index
        }
    }
    return -1;  // server full
}

// Remove client from our tracking array
void remove_client(int fd, int epoll_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    // EPOLL_CTL_DEL: remove fd from epoll's interest list
    // NULL: no event needed for DEL operation
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) {
            printf("[-] fd=%d disconnected (%d msgs, %ld bytes)\n",
                   fd, clients[i].msg_count, clients[i].bytes_received);
            clients[i].fd = -1;
            client_count--;
            break;
        }
    }
    close(fd);
}

// Broadcast message to ALL connected clients except the sender
void broadcast(int sender_fd, const char* msg, ssize_t len) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && clients[i].fd != sender_fd) {
            // send() might not send all bytes in one call
            // In production: handle partial sends with a write buffer
            // For now: simplification
            ssize_t sent = send(clients[i].fd, msg, len, MSG_NOSIGNAL);
            // MSG_NOSIGNAL: don't send SIGPIPE if client disconnected
            // Without this, writing to a closed socket kills your server
            if (sent < 0 && errno != EAGAIN) {
                printf("[!] send failed to fd=%d\n", clients[i].fd);
            }
        }
    }
}

int main() {
    // Initialize all client slots as empty
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;
    
    // ── CREATE AND BIND SERVER SOCKET ───────────────────────
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(server_fd, 128) < 0) {
        perror("listen"); return 1;
    }
    set_nonblocking(server_fd);
    printf("[*] epoll chat server on port %d\n", PORT);
    printf("[*] Connect with: nc localhost %d\n\n", PORT);
    
    // ── CREATE EPOLL INSTANCE ────────────────────────────────
    int epoll_fd = epoll_create1(0);
    // epoll_create1(0) = create epoll, no special flags
    // Returns a file descriptor representing the epoll object
    // You can even epoll on this fd (epoll on epoll) — advanced usage
    if (epoll_fd < 0) { perror("epoll_create1"); return 1; }
    
    // Watch server_fd for new connections
    // EPOLLIN = notify when readable (new connection = read event on listen socket)
    epoll_add(epoll_fd, server_fd, EPOLLIN);
    
    // ── EVENT LOOP ───────────────────────────────────────────
    struct epoll_event events[MAX_EVENTS];
    
    while (1) {
        // epoll_wait: block until events are ready
        // Arg 1: our epoll instance fd
        // Arg 2: array to write ready events into
        // Arg 3: max events to return per call (our array size)
        // Arg 4: timeout in ms (-1 = block forever, 0 = return immediately)
        // Returns: number of ready events (0 on timeout, -1 on error)
        int nready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (nready < 0) {
            if (errno == EINTR) continue;  // interrupted by signal — retry
            perror("epoll_wait"); break;
        }
        
        // Process each ready event
        for (int i = 0; i < nready; i++) {
            int fd     = events[i].data.fd;     // which fd is ready
            uint32_t ev = events[i].events;    // what kind of event
            
            // ── NEW CONNECTION ───────────────────────────────
            if (fd == server_fd) {
                // accept() in a loop — there might be multiple clients
                // that connected between two epoll_wait() calls
                while (1) {
                    struct sockaddr_in caddr;
                    socklen_t clen = sizeof(caddr);
                    int cfd = accept(server_fd,
                                     (struct sockaddr*)&caddr, &clen);
                    
                    if (cfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;   // no more connections right now
                        perror("accept");
                        break;
                    }
                    
                    set_nonblocking(cfd);
                    
                    if (add_client(cfd) == -1) {
                        // Server is full
                        send(cfd, "Server full, try later\n", 23, 0);
                        close(cfd);
                        continue;
                    }
                    
                    // Register with epoll: EPOLLIN | EPOLLET
                    // EPOLLIN  = notify when data available
                    // EPOLLET  = edge-triggered mode (more efficient)
                    // EPOLLRDHUP = notify when peer closes connection
                    epoll_add(epoll_fd, cfd, EPOLLIN | EPOLLET | EPOLLRDHUP);
                    
                    char welcome[64];
                    snprintf(welcome, sizeof(welcome),
                             "[server] welcome! %d users online\n", client_count);
                    send(cfd, welcome, strlen(welcome), 0);
                    
                    printf("[+] fd=%d connected (%d total)\n", cfd, client_count);
                }
                continue;
            }
            
            // ── CLIENT DISCONNECTED ─────────────────────────
            // EPOLLRDHUP = remote peer closed connection
            // EPOLLERR   = error on fd
            // EPOLLHUP   = hangup (connection dropped)
            if (ev & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                remove_client(fd, epoll_fd);
                continue;
            }
            
            // ── DATA FROM CLIENT ─────────────────────────────
            if (ev & EPOLLIN) {
                char buf[BUF_SIZE];
                
                // CRITICAL: edge-triggered → must read ALL available data
                // Loop until recv() returns EAGAIN
                while (1) {
                    ssize_t n = recv(fd, buf, BUF_SIZE - 1, 0);
                    
                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;  // drained — no more data right now
                        }
                        // Real error
                        remove_client(fd, epoll_fd);
                        break;
                    }
                    
                    if (n == 0) {
                        // n==0 means client cleanly closed connection (TCP FIN)
                        remove_client(fd, epoll_fd);
                        break;
                    }
                    
                    buf[n] = '\0';
                    
                    // Update stats for this client
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fd) {
                            clients[j].msg_count++;
                            clients[j].bytes_received += n;
                            break;
                        }
                    }
                    
                    printf("[fd %d] %s", fd, buf);
                    
                    // Broadcast to all OTHER clients
                    char outbuf[BUF_SIZE + 32];
                    int outlen = snprintf(outbuf, sizeof(outbuf),
                                          "[client %d]: %s", fd, buf);
                    broadcast(fd, outbuf, outlen);
                }
            }
        }
    }
    
    close(epoll_fd);
    close(server_fd);
    return 0;
}
