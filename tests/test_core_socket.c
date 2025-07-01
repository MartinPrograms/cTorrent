#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "CoreSocket.h"

#define TCP_TEST_MSG  "HelloTCP"
#define UDP_TEST_MSG  "HelloUDP"
#define BUF_SIZE      128

START_TEST(test_create_and_destroy)
{
    CoreSocket *sock = CoreSocket_create(CORE_SOCKET_TYPE_TCP);
    ck_assert_ptr_nonnull(sock);
    CoreSocket_destroy(sock);
}
END_TEST

START_TEST(test_set_options)
{
    CoreSocket *sock = CoreSocket_create(CORE_SOCKET_TYPE_TCP);
    ck_assert_ptr_nonnull(sock);

    /* Non-blocking */
    ck_assert_int_eq(CoreSocket_set_nonblocking(sock, true), CORE_SOCKET_SUCCESS);
    ck_assert(sock->nonblocking);
    ck_assert_int_eq(CoreSocket_set_nonblocking(sock, false), CORE_SOCKET_SUCCESS);
    ck_assert(!sock->nonblocking);

    /* Reuse address */
    ck_assert_int_eq(CoreSocket_set_reuseaddr(sock, true), CORE_SOCKET_SUCCESS);
    ck_assert_int_eq(CoreSocket_set_reuseaddr(sock, false), CORE_SOCKET_SUCCESS);

    /* Timeouts */
    ck_assert_int_eq(CoreSocket_set_recv_timeout(sock, 200), CORE_SOCKET_SUCCESS);
    ck_assert_int_eq(CoreSocket_set_send_timeout(sock, 300), CORE_SOCKET_SUCCESS);

    CoreSocket_destroy(sock);
}
END_TEST

START_TEST(test_tcp_echo)
{
    /* Server setup */
    CoreSocket *server = CoreSocket_create(CORE_SOCKET_TYPE_TCP);
    ck_assert_ptr_nonnull(server);
    ck_assert_int_eq(CoreSocket_set_reuseaddr(server, true), CORE_SOCKET_SUCCESS);
    ck_assert_int_eq(CoreSocket_bind(server, "127.0.0.1", 0), CORE_SOCKET_SUCCESS);

    /* Determine assigned port */
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    ck_assert_int_eq(getsockname(server->fd, (struct sockaddr*)&addr, &addrlen), 0);
    uint16_t port = ntohs(addr.sin_port);

    ck_assert_int_eq(CoreSocket_listen(server, 1), CORE_SOCKET_SUCCESS);

    pid_t pid = fork();
    ck_assert_int_ge(pid, 0);

    if (pid == 0) {
        /* Child: client */
        CoreSocket *client = CoreSocket_create(CORE_SOCKET_TYPE_TCP);
        ck_assert_ptr_nonnull(client);
        ck_assert_int_eq(CoreSocket_connect(client, "127.0.0.1", port), CORE_SOCKET_SUCCESS);

        /* Send message */
        ssize_t sent = CoreSocket_send(client, TCP_TEST_MSG, strlen(TCP_TEST_MSG));
        ck_assert_int_eq(sent, (ssize_t)strlen(TCP_TEST_MSG));

        /* Receive echo */
        char buf[BUF_SIZE] = {0};
        ssize_t recvd = CoreSocket_recv(client, buf, BUF_SIZE);
        ck_assert_int_eq(recvd, (ssize_t)strlen(TCP_TEST_MSG));
        ck_assert_str_eq(buf, TCP_TEST_MSG);

        CoreSocket_destroy(client);
        exit(EXIT_SUCCESS);
    }

    /* Parent: server accepts and echoes */
    char peer_addr[INET_ADDRSTRLEN] = {0};
    uint16_t peer_port = 0;
    CoreSocket *conn = CoreSocket_accept(server, peer_addr, sizeof(peer_addr), &peer_port);
    ck_assert_ptr_nonnull(conn);

    char buf[BUF_SIZE] = {0};
    ssize_t recvd = CoreSocket_recv(conn, buf, BUF_SIZE);
    ck_assert_int_eq(recvd, (ssize_t)strlen(TCP_TEST_MSG));
    ck_assert_str_eq(buf, TCP_TEST_MSG);

    ssize_t sent = CoreSocket_send(conn, buf, recvd);
    ck_assert_int_eq(sent, recvd);

    CoreSocket_destroy(conn);
    CoreSocket_destroy(server);

    /* Wait for child */
    int status;
    waitpid(pid, &status, 0);
    ck_assert_int_eq(WEXITSTATUS(status), EXIT_SUCCESS);
}
END_TEST

START_TEST(test_udp_echo)
{
    /* Server setup */
    CoreSocket *server = CoreSocket_create(CORE_SOCKET_TYPE_UDP);
    ck_assert_ptr_nonnull(server);
    ck_assert_int_eq(CoreSocket_bind(server, "127.0.0.1", 0), CORE_SOCKET_SUCCESS);

    /* Determine assigned port */
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    ck_assert_int_eq(getsockname(server->fd, (struct sockaddr*)&addr, &addrlen), 0);
    uint16_t port = ntohs(addr.sin_port);

    /* Client sends datagram */
    CoreSocket *client = CoreSocket_create(CORE_SOCKET_TYPE_UDP);
    ck_assert_ptr_nonnull(client);
    ssize_t sent = CoreSocket_sendto(client, UDP_TEST_MSG, strlen(UDP_TEST_MSG), "127.0.0.1", port);
    ck_assert_int_eq(sent, (ssize_t)strlen(UDP_TEST_MSG));

    /* Server receives datagram */
    char buf[BUF_SIZE] = {0};
    char src_addr[INET_ADDRSTRLEN] = {0};
    uint16_t src_port = 0;
    ssize_t recvd = CoreSocket_recvfrom(server, buf, BUF_SIZE, src_addr, sizeof(src_addr), &src_port);
    ck_assert_int_eq(recvd, (ssize_t)strlen(UDP_TEST_MSG));
    ck_assert_str_eq(buf, UDP_TEST_MSG);

    /* Echo back */
    sent = CoreSocket_sendto(server, buf, recvd, src_addr, src_port);
    ck_assert_int_eq(sent, recvd);

    /* Client receives echo */
    memset(buf, 0, sizeof(buf));
    ssize_t echo = CoreSocket_recvfrom(client, buf, BUF_SIZE, NULL, 0, NULL);
    ck_assert_int_eq(echo, recvd);
    ck_assert_str_eq(buf, UDP_TEST_MSG);

    CoreSocket_destroy(client);
    CoreSocket_destroy(server);
}
END_TEST

START_TEST(test_invalid_operations)
{
    CoreSocket *sock = CoreSocket_create(CORE_SOCKET_TYPE_TCP);
    ck_assert_ptr_nonnull(sock);
    /* Invalid address */
    ck_assert_int_eq(CoreSocket_connect(sock, "256.256.256.256", 1234), CORE_SOCKET_ERROR);
    /* Accept on non-listening */
    ck_assert_ptr_null(CoreSocket_accept(sock, NULL, 0, NULL));
    CoreSocket_destroy(sock);
}
END_TEST

Suite *core_socket_suite(void) {
    Suite *s = suite_create("CoreSocket");
    TCase *tc = tcase_create("CoreSocketTests");

    tcase_add_test(tc, test_create_and_destroy);
    tcase_add_test(tc, test_set_options);
    tcase_add_test(tc, test_tcp_echo);
    tcase_add_test(tc, test_udp_echo);
    tcase_add_test(tc, test_invalid_operations);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int failed;
    Suite *s = core_socket_suite();
    SRunner *runner = srunner_create(s);
    srunner_run_all(runner, CK_NORMAL);
    failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
