//
// Created by xiang on 2024/2/9.
//

#include <stdio.h>
#include <string.h>
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#define MAX_BUF_SIZE 1024

int pub_msg() {
    int sock;
    int rv;
    char buf[MAX_BUF_SIZE];

    // 创建发布者套接字
    sock = nn_socket(AF_SP, NN_PUB);
    if (sock < 0) {
        perror("nn_socket");
        return 1;
    }

    // 绑定到本地地址
    rv = nn_bind(sock, "tcp://127.0.0.1:5555");
    if (rv < 0) {
        perror("nn_bind");
        return 1;
    }

    printf("Publisher started.\n");

    while (1) {
        // 从标准输入读取消息
        printf("Enter message to publish: ");
        fgets(buf, MAX_BUF_SIZE, stdin);

        // 发布消息
        rv = nn_send(sock, buf, strlen(buf), 0);
        if (rv < 0) {
            perror("nn_send");
            break;
        }

        printf("Published %d bytes.\n", rv);
    }

    // 关闭套接字
    nn_close(sock);

    return 0;
}