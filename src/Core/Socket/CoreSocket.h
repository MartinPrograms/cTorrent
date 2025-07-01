#ifndef CORESOCKET_H
#define CORESOCKET_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>    // For socket types
#include <sys/socket.h>   // For socket API
#include <netinet/in.h>   // For sockaddr_in
#include <arpa/inet.h>    // For inet_pton
#include <unistd.h>       // For close
#include <fcntl.h>
#include <string.h>

// Return codes
#define CORE_SOCKET_SUCCESS 0
#define CORE_SOCKET_ERROR  -1

#define CORE_SOCKET_UDP_LIMIT 65507 // Maximum UDP packet size (RFC 768)

typedef enum {
    CORE_SOCKET_TYPE_TCP,
    CORE_SOCKET_TYPE_UDP
} CoreSocketType;

typedef struct {
    int fd;        // Underlying file descriptor
    CoreSocketType type;     // TCP or UDP
    bool nonblocking;
} CoreSocket;

CoreSocket* CoreSocket_create(CoreSocketType type);
void CoreSocket_close(CoreSocket* sock);
void CoreSocket_destroy(CoreSocket* sock);

int CoreSocket_set_nonblocking(CoreSocket* sock, bool enable);
int CoreSocket_set_reuseaddr(CoreSocket* sock, bool enable);
int CoreSocket_set_recv_timeout(CoreSocket* sock, uint32_t milliseconds);
int CoreSocket_set_send_timeout(CoreSocket* sock, uint32_t milliseconds);

int CoreSocket_connect(CoreSocket* sock, const char* address, uint16_t port);

int CoreSocket_bind(CoreSocket* sock, const char* bind_address, uint16_t port);
int CoreSocket_listen(CoreSocket* sock, int backlog);
CoreSocket* CoreSocket_accept(CoreSocket* server_sock, char* peer_addr, size_t addr_len, uint16_t* peer_port);

ssize_t CoreSocket_send(CoreSocket* sock, const void* buffer, size_t length);
ssize_t CoreSocket_recv(CoreSocket* sock, void* buffer, size_t length);

ssize_t CoreSocket_sendto(CoreSocket* sock, const void* buffer, size_t length,
                          const char* dest_address, uint16_t dest_port);
ssize_t CoreSocket_recvfrom(CoreSocket* sock, void* buffer, size_t length,
                            char* src_address, size_t addr_len, uint16_t* src_port);

#endif // CORESOCKET_H
