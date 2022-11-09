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
                char **ans = _str_split(buffer, ' ');
                target_Client = atoi(ans[0]);

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
        sleep(0.5);
    }

    /* 等待子进程退出 */
    printf("Waiting for child thread to exit ....\n");
    char *message;
    pthread_join(thrAccept, (void *)&message);
    printf("%s\n", message);

    return 0;
}

/********************************************************************
 *   Function Name: void *fun_thrAcceptHandler(void *socketListen)
 *   Description: 监听客户端的连接请求，获取待连接客户端的网络信息，并为该客户端创建子线程.
 *   Called By: server.c[main]
 *   Input: socketListen -> 表示用于监听的被动套接字
 *********************************************************************/
void *fun_thrAcceptHandler(void *socketListen)
{
    while (1)
    {
        int sockaddr_in_size = sizeof(struct sockaddr_in);
        struct sockaddr_in client_addr;
        int _socketListen = *((int *)socketListen);

        /* 接收相应的客户端的连接请求 */
        int socketCon = accept(_socketListen, (struct sockaddr *)(&client_addr), (socklen_t *)(&sockaddr_in_size));
        if (socketCon < 0)
        {
            printf("call accept()");
        }
        else
        {
            printf("Connected %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        }
        printf("Client socket: %d\n", socketCon);

        /* 获取新客户端的网络信息 */
        _MySocketInfo socketInfo;
        socketInfo.socketCon = socketCon;
        socketInfo.ipaddr = inet_ntoa(client_addr.sin_addr);
        socketInfo.port = client_addr.sin_port;

        /* 将新客户端的网络信息保存在 arrConSocket 数组中 */
        arrConSocket[conClientCount++] = socketInfo;
        // conClientCount++;
        printf("Number of users: %d\n", conClientCount);

        /* 为新连接的客户端开辟线程 fun_thrReceiveHandler，该线程用来循环接收客户端的数据 */
        pthread_t thrReceive = 0;
        pthread_create(&thrReceive, NULL, fun_thrReceiveHandler, &socketInfo);
        arrThrReceiveClient[thrReceiveClientCount++] = thrReceive;
        // thrReceiveClientCount ++;
        printf("A thread has been created for the user.\n");

        /* 让进程休息0.1秒 */
        usleep(100000);
    }

    char *s = "Safe exit from the receive process ...";
    pthread_exit(s);
}

/********************************************************************
 *   Function Name: void *fun_thrReceiveHandler(void *socketInfo)
 *   Description: 向服务器发送初始消息，从服务器循环接收信息.
 *   Called By: server.c[main]
 *   Input: socketInfo -> 表示客户端的网络信息
 *********************************************************************/
void *fun_thrReceiveHandler(void *socketInfo)
{
    int buffer_length;
    int con;
    int i;
    _MySocketInfo _socketInfo = *((_MySocketInfo *)socketInfo);

    /* 向服务器发送握手消息 */
    char str[80] = HANDSHARK_MSG;
    char socketCon[25];
    sprintf(socketCon, "%d", _socketInfo.socketCon);
    strcat(str, socketCon);
    send(_socketInfo.socketCon, str, sizeof(str), 0);
    // _send_msg2allclient(_socketInfo.socketCon);

    /* 从服务器循环接收消息 */
    while (1)
    {

        // 将接收缓冲区buffer清空
        bzero(&buffer, sizeof(buffer));

        // 接收服务器信息
        printf("Receiving messages from client %d ...\n", _socketInfo.socketCon);
        buffer_length = recv(_socketInfo.socketCon, buffer, BUFSIZ, 0);
        if (buffer_length == 0)
        {
            // 判断为客户端退出
            printf("%s:%d Closed!\n", _socketInfo.ipaddr, _socketInfo.port);
            // 找到该客户端在数组中的位置
            for (con = 0; con < conClientCount; con++)
            {
                if (arrConSocket[con].socketCon == _socketInfo.socketCon)
                {
                    break;
                }
            }
            // 将该客户端的信息删除，重置客户端数组
            for (i = con; i < conClientCount - 1; i++)
            {
                arrConSocket[i] = arrConSocket[i + 1];
            }
            conClientCount--;
            break;
        }
        else if (buffer_length < 0)
        {
            printf("Fail to call read()\n");
            break;
        }
        buffer[buffer_length] = '\0';
        send_Client = _socketInfo.socketCon;
        printf("%s:%d said: %s,sockertCon: %d\n", _socketInfo.ipaddr, _socketInfo.port, buffer, send_Client);
        ReadToSend = 1; // 发送标志置位，允许主线程发送数据
        usleep(100000);
    }
    printf("%s:%d Exit\n", _socketInfo.ipaddr, _socketInfo.port);
    return NULL;
}

/********************************************************************
 *   Function Name: checkThrIsKill(pthread_t thr)
 *   Description: 检测当前线程是否存活.
 *   Called By: server.c[main]
 *   Input: thr -> 线程数组中的线程
 *   Date: 2020/05/24
 *********************************************************************/
int checkThrIsKill(pthread_t thr)
{
    int res = 1;
    int res_kill = pthread_kill(thr, 0);
    if (res_kill == 0)
    {
        res = 0;
    }
    return res;
}

char **_str_split(char *a_str, const char a_delim)
{
    char **result = 0;
    size_t count = 0;
    char *tmp = a_str;
    char *last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char *) * count);

    if (result)
    {
        size_t idx = 0;
        char *token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

// void _send_msg2allclient(int socketCon)
// {
//     char *wecomle="your friend online: ";
//     char con[5];
//     sprintf(con,"%d",socketCon);
//     strcat(wecomle,con);
//     for (int i = 0; i < conClientCount; i++)
//     {
//         int sendMsg_len = send(arrConSocket[i].socketCon,wecomle , strlen(wecomle), 0);
//         if (sendMsg_len > 0)
//         {
//             printf("Send Message to %s:%d successful\n", arrConSocket[i].ipaddr, arrConSocket[i].port);
//             ReadToSend = 0;
//         }
//         else
//         {
//             printf("Fail to send message to %s:%d\n", arrConSocket[i].ipaddr, arrConSocket[i].port);
//         }
//     }
// }
