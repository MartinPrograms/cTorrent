#include "CoreSocket.h"

CoreSocket * CoreSocket_create(CoreSocketType type) {
    CoreSocket *sock = (CoreSocket *)malloc(sizeof(CoreSocket));
    if (sock == NULL) {
        return NULL; // Memory allocation failed
    }

    sock->type = type;
    sock->fd = socket(AF_INET, (type == CORE_SOCKET_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (sock->fd < 0) {
        free(sock);
        return NULL; // Socket creation failed
    }
    sock->nonblocking = false;
    return sock;
}

void CoreSocket_close(CoreSocket *sock) {
    if (sock == NULL) {
        return; // Nothing to close
    }

    close(sock->fd);
    sock->fd = -1; // Invalidate the file descriptor
    sock->nonblocking = false; // Reset non-blocking state
}

void CoreSocket_destroy(CoreSocket *sock) {
    if (sock == NULL) {
        return; // Nothing to destroy
    }

    CoreSocket_close(sock);
    free(sock);
    sock = NULL; // Avoid dangling pointer
}

int CoreSocket_set_nonblocking(CoreSocket *sock, bool enable) {
    int flags = fcntl(sock->fd, F_GETFL, 0);
    if (flags < 0) {
        return CORE_SOCKET_ERROR; // Failed to get flags
    }
    if (enable) {
        flags |= O_NONBLOCK; // Set non-blocking mode
    } else {
        flags &= ~O_NONBLOCK; // Clear non-blocking mode
    }

    if (fcntl(sock->fd, F_SETFL, flags) < 0) {
        return CORE_SOCKET_ERROR; // Failed to set flags
    }
    sock->nonblocking = enable;

    return CORE_SOCKET_SUCCESS; // Successfully set non-blocking mode
}

int CoreSocket_set_reuseaddr(CoreSocket *sock, bool enable) {
    int optval = enable ? 1 : 0;

    if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        return CORE_SOCKET_ERROR; // Failed to set option
    }

    return CORE_SOCKET_SUCCESS; // Successfully set SO_REUSEADDR
}

int CoreSocket_set_recv_timeout(CoreSocket *sock, uint32_t milliseconds) {
    // Using the SO_RCVTIMEO option to set receive timeout
    struct timeval timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (int)(milliseconds % (uint32_t)1000) * 1000;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return CORE_SOCKET_ERROR; // Failed to set receive timeout
    }

    return CORE_SOCKET_SUCCESS; // Successfully set receive timeout
}

int CoreSocket_set_send_timeout(CoreSocket *sock, uint32_t milliseconds) {
    struct timeval timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (int)(milliseconds % (uint32_t)1000) * 1000;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return CORE_SOCKET_ERROR; // Failed to set send timeout
    }
    return CORE_SOCKET_SUCCESS; // Successfully set send timeout
}

// taken from https://www.linuxhowtos.org/C_C++/socket.htm
int CoreSocket_connect(CoreSocket *sock, const char *address, uint16_t port) {
    if (sock == NULL || address == NULL) {
        return CORE_SOCKET_ERROR; // Invalid parameters
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = htons(port); // Convert port to network byte order
    if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
        return CORE_SOCKET_ERROR; // Invalid address
    }

    if (connect(sock->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return CORE_SOCKET_ERROR; // Connection failed
    }

    return CORE_SOCKET_SUCCESS; // Successfully connected
}

int CoreSocket_bind(CoreSocket *sock, const char *bind_address, uint16_t port) {
    if (sock == NULL || bind_address == NULL) {
        return CORE_SOCKET_ERROR; // Invalid parameters
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = htons(port); // Convert port to network byte order
    if (inet_pton(AF_INET, bind_address, &addr.sin_addr) <= 0) {
        return CORE_SOCKET_ERROR; // Invalid address
    }

    if (bind(sock->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return CORE_SOCKET_ERROR; // Binding failed
    }

    return CORE_SOCKET_SUCCESS; // Successfully bound
}

int CoreSocket_listen(CoreSocket *sock, int backlog) {
    if (sock == NULL) {
        return CORE_SOCKET_ERROR; // Invalid socket
    }

    if (sock->type != CORE_SOCKET_TYPE_TCP) {
        return CORE_SOCKET_ERROR; // Listen only applies to TCP sockets
    }

    if (listen(sock->fd, backlog) < 0) {
        return CORE_SOCKET_ERROR; // Listening failed
    }

    return CORE_SOCKET_SUCCESS; // Successfully listening
}

CoreSocket * CoreSocket_accept(CoreSocket *server_sock, char *peer_addr, size_t addr_len, uint16_t *peer_port) {
    if (server_sock == NULL || peer_addr == NULL || addr_len == 0 || peer_port == NULL) {
        return NULL; // Invalid parameters
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // Clear the address structure
    socklen_t addr_len_var = sizeof(addr);

    int client_fd = accept(server_sock->fd, (struct sockaddr *)&addr, &addr_len_var);

    if (client_fd < 0) {
        return NULL; // Accept failed
    }
    if (addr_len < INET_ADDRSTRLEN) {
        close(client_fd); // Close the client socket if address buffer is too small
        return NULL; // Address buffer too small
    }

    if (inet_ntop(AF_INET, &addr.sin_addr, peer_addr, addr_len) == NULL) {
        close(client_fd); // Close the client socket on error
        return NULL; // Address conversion failed
    }

    *peer_port = ntohs(addr.sin_port); // Convert port to host byte order
    CoreSocket *client_sock = (CoreSocket *)malloc(sizeof(CoreSocket));
    if (client_sock == NULL) {
        close(client_fd); // Close the client socket on memory allocation failure
        return NULL; // Memory allocation failed
    }

    client_sock->fd = client_fd;
    client_sock->type = server_sock->type; // Same type as the server socket
    client_sock->nonblocking = server_sock->nonblocking; // Inherit non-blocking state
    return client_sock; // Return the new client socket
}

ssize_t CoreSocket_send(CoreSocket *sock, const void *buffer, size_t length) {
    if (sock == NULL || buffer == NULL || length == 0) {
        return CORE_SOCKET_ERROR; // Invalid parameters
    }
    ssize_t sent = send(sock->fd, buffer, length, 0);
    if (sent < 0) {
        return CORE_SOCKET_ERROR; // Send failed
    }
    return sent; // Return number of bytes sent
}

ssize_t CoreSocket_recv(CoreSocket *sock, void *buffer, size_t length) {
    if (sock == NULL || buffer == NULL || length == 0) {
        return CORE_SOCKET_ERROR; // Invalid parameters
    }
    ssize_t received = recv(sock->fd, buffer, length, 0);
    if (received < 0) {
        return CORE_SOCKET_ERROR; // Receive failed
    }
    return received; // Return number of bytes received
}

ssize_t CoreSocket_sendto(CoreSocket *sock, const void *buffer, size_t length, const char *dest_address,
    uint16_t dest_port) {
    if (sock == NULL || buffer == NULL || length == 0 || dest_address == NULL) {
        return CORE_SOCKET_ERROR; // Invalid parameters
    }

    if (sock->type != CORE_SOCKET_TYPE_UDP) {
        return CORE_SOCKET_ERROR; // sendto only applies to UDP sockets
    }

    if (length > CORE_SOCKET_UDP_LIMIT) {
        return CORE_SOCKET_ERROR; // UDP maximum size is 65507 bytes
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr)); // Clear the address structure
    dest_addr.sin_family = AF_INET; // IPv4
    dest_addr.sin_port = htons(dest_port); // Convert port to network byte order
    if (inet_pton(AF_INET, dest_address, &dest_addr.sin_addr) <= 0) {
        return CORE_SOCKET_ERROR; // Invalid address
    }
    ssize_t sent = sendto(sock->fd, buffer, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        return CORE_SOCKET_ERROR; // Send failed
    }
    return (int)sent; // Return number of bytes sent
}

ssize_t CoreSocket_recvfrom(CoreSocket *sock, void *buffer, size_t length, char *src_address, size_t addr_len,
        uint16_t *src_port) {
    if (!sock || !buffer || length == 0) {
        return CORE_SOCKET_ERROR; // Invalid parameters
    }

    if (sock->type != CORE_SOCKET_TYPE_UDP) {
        return CORE_SOCKET_ERROR; // recvfrom only applies to UDP sockets
    }

    struct sockaddr_storage src_storage;
    socklen_t src_storage_len = sizeof(src_storage);

    ssize_t received = recvfrom(
        sock->fd,
        buffer,
        length,
        0,
        (struct sockaddr *)&src_storage,
        &src_storage_len
    );
    if (received < 0) {
        return CORE_SOCKET_ERROR;
    }

    if (src_address) {
        if (src_storage.ss_family == AF_INET) {
            struct sockaddr_in *sa4 = (struct sockaddr_in *)&src_storage;
            if (addr_len < INET_ADDRSTRLEN ||
                inet_ntop(AF_INET, &sa4->sin_addr, src_address, addr_len) == NULL) {
                return CORE_SOCKET_ERROR;
            }
        }
    }

    if (src_port) {
        if (src_storage.ss_family == AF_INET) {
            struct sockaddr_in *sa4 = (struct sockaddr_in *)&src_storage;
            *src_port = ntohs(sa4->sin_port);
        }
    }

    return received;
}
