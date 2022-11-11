/********************************************************************
 *   File Name: server.c
 *   Description: 用于实现多客户端通信
 *   Others:
 *     1. 服务器首先创建套接字并绑定网络信息
 *     2. 之后创建一个子线程用于接收客户端的网络请求
 *     3. 在子线程中接收客户端的网络请求，并为每一个客户端创建新的子线程，该子线程用于服务器接收客户端数据
 *     4. 服务器的主线程用于向所有客户端循环发送数据
 *********************************************************************/
#include "mysocket.h"

int main()
{
    ReadToSend = 0;
    conClientCount = 0;
    thrReceiveClientCount = 0;

    printf("Start Server...\n");

    /* 创建TCP连接的Socket套接字 */
    int socketListen = socket(AF_INET, SOCK_STREAM, 0);
    if (socketListen < 0)
    {
        printf("Fail to create Socket\n");
        exit(-1);
    }
    else
    {
        printf("Create Socket successful.\n");
    }

    /* 填充服务器端口地址信息，以便下面使用此地址和端口监听 */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    // 这里的地址使用所有本地网络设备的地址，表示服务器会接收任意地址的客户端信息
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    /* 将套接字绑定到服务器的网络地址上 */
    if (bind(socketListen, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) != 0)
    {
        perror("bind error");
        exit(-1);
    }
    printf("call bind() successful\n");

    /* 开始监听相应的端口 */
    if (listen(socketListen, 10) != 0)
    {
        perror("call listen()");
        exit(-1);
    }
    printf("call listen() successful\n");

    /* 创建一个线程用来接收客户端的连接请求 */
    pthread_t thrAccept;
    pthread_create(&thrAccept, NULL, fun_thrAcceptHandler, &socketListen);

    /* 主线程用来向所有客户端循环发送数据 */
    while (1)
    {
        if (ReadToSend)
        {
            ReadToSend=0;

            // 判断线程存活数量
            int i;
            for (i = 0; i < thrReceiveClientCount; i++)
            {
                if (checkThrIsKill(arrThrReceiveClient[i]))
                {
                    printf("A Thread has been killed\n");
                    thrReceiveClientCount--;
                }
            }
            printf("Number of connected client: %d\n", thrReceiveClientCount);
            if (conClientCount <= 0)
            {
                printf("No Clients!\n");
            }

            // 向除自身的客户端发送消息
            else
            {
                // printf("conClientCount = %d\n", conClientCount);
                // 把这部分逻辑写入 mysocket.c
                char copy_buf_send[BUFSIZ];
                memcpy(copy_buf_send, buffer, sizeof(buffer) / sizeof(buffer[0]) * sizeof(char));
                char **ans = _str_split(copy_buf_send, ' ');
                target_Client = atoi(ans[0]);
                if (target_Client == -1)
                {
                    char file_signal[BUFSIZ] = "-1 ";
                    strcat(file_signal, ans[2]);
                    // printf("file_signal:%s\n",file_signal);
                    send(send_Client, file_signal, strlen(file_signal), 0);
                    // printf("open file:%s\n",ans[1]);
                    FILE *fp = fopen(ans[1], "r");
                    bzero(buffer, BUFSIZ);
                    if (NULL == fp)
                    {
                        printf("File:%s Not Found\n", ans[1]);
                    }
                    else
                    {

                        int length = 0;
                        // 每读取一段数据，便将其发送给客户端，循环直到文件读完为止
                        while ((length = fread(buffer, sizeof(char), BUFSIZ, fp)) > 0)
                        {

                            buffer[length] = '\0';
                            printf("buffer:%s\n",buffer);
                            if (send(send_Client, buffer, length, 0) < 0)
                            {
                                printf("Send File:%s Failed./n", ans[1]);
                                break;
                            }
                            bzero(buffer, BUFSIZ);
                        }
                        // 关闭文件
                        fclose(fp);

                    }
                    // send(send_Client,"end...",7,0);

                }
                if (target_Client > 0)
                {
                    for (i = 0; i < conClientCount; i++)
                    {
                        if (arrConSocket[i].socketCon == send_Client)
                        {
                            ReadToSend = 0;
                            continue;
                        }
                        if (arrConSocket[i].socketCon == target_Client)
                        {
                            printf("socketCon = %d\nbuffer is: %s\n", arrConSocket[i].socketCon, buffer);
                            int sendMsg_len = send(arrConSocket[i].socketCon, ans[1], strlen(ans[1]), 0);
                            if (sendMsg_len > 0)
                            {
                                printf("Send Message to %s:%d successful\n", arrConSocket[i].ipaddr, arrConSocket[i].port);
                                ReadToSend = 0;
                            }
                            else
                            {
                                printf("Fail to send message to %s:%d\n", arrConSocket[i].ipaddr, arrConSocket[i].port);
                            }
                            break;
                        }
                    }
                }
            }
        }
        sleep(0.5);
    }

    /* 等待子进程退出 */
    printf("Waiting for child thread to exit ....\n");
    char *message;
    pthread_join(thrAccept, (void *)&message);
    printf("%s\n", message);

    return 0;
}
