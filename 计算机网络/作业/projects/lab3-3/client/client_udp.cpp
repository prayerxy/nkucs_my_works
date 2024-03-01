#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include "udp.h"
#include <fstream>
#include <iostream>
#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")
using namespace  std;
SOCKET clientSocket;
SOCKADDR_IN serverAddr;
SOCKADDR_IN clientAddr;
int server_addrlen;
//如果重传次数过多，对延时进行更改
int resend_flag = 0;
//源端口与目的端口
unsigned short source_port = 8080;
unsigned short dest_port = 4001;
char buffer[100000000];//接收数据
char filename[30];
int file_len;//文件的字节长度
int timeout = TIMEOUT;
int changetime_flag = 0;
//针对分组进行计时的标志
int time_flag[8000] = { 0 };
//针对分组进行计时的时间
//为了避免出现多线程问题，所以采取此措施
unsigned long long start_stamp[8000] = { 0 };
int finish_flag = 0;//完成传输的标志

unsigned long  seq_len = 0;
unsigned short base = 0;
unsigned short nextseqnum = 0;//与seq_num一致，最大65535，超过会自动转为0


//发送窗口  采取SR协议
deque<Mymsg>window;


void hand3Shakes() {
    char buffer[Msg_size];
    int timeout = TIMEOUT * 4;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    while (1) {
        //SYN请求
        Mymsg temp = send_generateMsg(source_port, dest_port, 0, 0, SYN, '0');
        sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        cout << "[客户端]:发送第一次握手SYN请求" << endl;
        int bytesRead = recvfrom(clientSocket, buffer, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain cur = Mymsg_explain(receive_char2msg(buffer));
        if (bytesRead > 0 && check_sum(buffer, Msg_size) == 0 && cur.isSYN() && cur.isACK()) {
            cout << "[客户端]:第二次握手接受SYN+ACK" << endl;
            //第三次握手,可能丢失
            Mymsg temp1 = send_generateMsg(source_port, dest_port, cur.msg.acknowledge_number, cur.msg.seq_number + 1, ACK, '0');
            sendto(clientSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            cout << "[客户端]:发送第三次握手ACK" << endl;
            break;
        }
        else
            continue;

    }
    cout << "[客户端]:三次握手连接结束" << endl;
    cout << "[客户端]:目的端口为:" << dest_port << endl;
}


// 模拟发送数据包的函数


// 模拟接收 ACK 的函数
void receiveACK(int groupN) {
    while (!finish_flag) {
        char curr_buf[Msg_size];
        int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain r_temp = Mymsg_explain(receive_char2msg(curr_buf));
        unsigned short r_temp_ack = r_temp.msg.acknowledge_number;
        if (ret > 0 && r_temp.isACK() && check_sum(curr_buf, Msg_size) == 0 && r_temp_ack >= base && r_temp_ack < base + WINDOW_SIZE - 1) {
            resend_flag = 0;
            printf("  接收到确认ACK，ack:%d \n", r_temp.msg.acknowledge_number);
            //停止该分组的计时
            time_flag[r_temp_ack] = Endtime;
            //如果是最小分组，窗口前移
            if (base == r_temp.msg.acknowledge_number) {
                //这里可能会弹出多个 从最老的开始  发送都是加在back尾部  弹出在front
                while (window.size() && time_flag[window.front().seq_number] == Endtime) {
                    window.pop_front();//最老的删除 最新的back不变
                    base++;
                }

                printf("  接受窗口最小的ack后，发送窗口size:%d,base:%d,nextseqnum:%d\n", nextseqnum - base, base, nextseqnum);
            }

            if (base == groupN || (r_temp.msg.flag & LAST) && r_temp.msg.acknowledge_number == groupN - 1) {
                finish_flag = 1;
                printf("  文件发送完毕\n");
                return;//接受完毕
            }

        }
    }
    return;
}

void timeoutResend(int groupN, int len) {
    while (!finish_flag) {
        //判断发送窗口的元素是否存在超时
        //这里其实不需要锁机制，如果接受ack了，那么前移/设置不计时 同时影响超时重发，没问题
        for (int i = base; i < nextseqnum; i++) {
            if (time_flag[i] == STARTtime && GetTickCount64() - start_stamp[i] > timeout) {
                //重置时间
                start_stamp[i] = GetTickCount64();
                //重发
                Mymsg temp = send_generateMsg(source_port, dest_port, i, 0, 0, '0');
                if (i == groupN - 1) {
                    temp.length = len - (groupN - 1) * MSS;
                    temp.flag |= LAST;
                    memcpy(temp.data, buffer + i * MSS, len - (groupN - 1) * MSS);
                }
                else
                    memcpy(temp.data, buffer + i * MSS, MSS);
                temp.checksum = 0;
                char* p = msg2char(temp);
                temp.checksum = check_sum(p, Msg_size);

                sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                printf("超时重新发送seq:%d，校验和:%d\n", temp.seq_number, temp.checksum);
                resend_flag++;
                //如果连续重发，没有收到ack，超过MISSCOUNT，鉴定为出错
                if (nextseqnum >= groupN - 1 && resend_flag >= ERRORCOUNT) {
                    printf("server has exited\n");
                    int tv = 2;
                    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
                    finish_flag = 1;
                    break;
                }
                if (resend_flag > MISSCOUNT && nextseqnum < groupN - 1) {
                    timeout *= 1.5;
                    if (timeout > 1000)
                        timeout = 1000;
                    resend_flag = 0;
                }


            }
        }
    }
    return;
}


//传输
void send_Msgs(char* buffer, int len) {
    //分组
    int timeout = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    finish_flag = 0;
    int groupN = MSS * (len / MSS) < len ? len / MSS + 1 : len / MSS;
    seq_len = 0;
    base = nextseqnum = 0;
    //开始传送
    std::thread ackThread(receiveACK, groupN);
    std::thread resendThread(timeoutResend, groupN, len);
    while (!finish_flag) {
        //只能发送0-groupN-1的包 最后seq_len为groupN
        while (nextseqnum < base + WINDOW_SIZE && seq_len <= groupN - 1) {
            Mymsg temp = send_generateMsg(source_port, dest_port, nextseqnum, 0, 0, '0');

            if (seq_len == groupN - 1) {
                temp.length = len - (groupN - 1) * MSS;
                temp.flag |= LAST;
                memcpy(temp.data, buffer + seq_len * MSS, len - (groupN - 1) * MSS);
                int timeout = TIMEOUT;
                setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

            }
            else
                memcpy(temp.data, buffer + seq_len * MSS, MSS);
            temp.checksum = 0;
            char* p = msg2char(temp);
            //产生checksum的值
            temp.checksum = check_sum(p, Msg_size);
            sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            //储存进发送缓冲区
            //发送新分组，开启计时
            time_flag[nextseqnum] = STARTtime;//开始计时
            start_stamp[nextseqnum] = GetTickCount64();
            window.push_back(temp);

            printf("发送seq:%d，校验和:%d\n", temp.seq_number, temp.checksum);
            //前沿移动，以及总位数移动
            nextseqnum++;//这里最大状态数是65535 超过回到0
            seq_len++;
            printf("发送窗口size:%d,base:%d,nextseqnum:%d\n", nextseqnum - base, base, nextseqnum);

        }


    }

    ackThread.join();
    resendThread.join();
    return;

}

//先用ifstream读取文件化为char数组
//再调用sendMsg传送数据
void send_file() {

    ifstream fin(filename, ifstream::in | ios::binary);
    fin.seekg(0, fin.end);  //将文件流指针定位到流的末尾
    file_len = fin.tellg();
    fin.seekg(0, fin.beg);  //将文件流指针重新定位到流的开始
    fin.read(buffer, file_len);
    fin.close();
    //采用rdt3.0传送 停等 即传完一个才会传下一个
    //先传输文件名
    cout << "----------------开始传输文件名-------------------" << endl;
    send_Msgs((char*)filename, 30);
    cout << "-------------开始传输文件内容--------------------" << endl;
    for (int i = 0; i < 8000; i++) {
        time_flag[i] = 0;
        start_stamp[i] = 0;
    }
    unsigned long long start_stamp1 = GetTickCount64();//开始计时
    send_Msgs(buffer, file_len);
    unsigned long long end_stamp1 = GetTickCount64();//开始计时
    cout << "-------------------" << endl;
    cout << "传输文件大小:" << file_len << "bytes" << endl;
    cout << "文件传输时间:" << end_stamp1 - start_stamp1 << "ms\n";
    double kbps = (file_len * 8.0) / (end_stamp1 - start_stamp1);
    cout << "吞吐率为:" << kbps << "kbps" << endl;
    cout << "--------------------" << endl;


}

void hand4Shakes() {
    int timeout = TIMEOUT;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    char curr_buf[Msg_size];
    recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);

    while (1) {

        memset(curr_buf, 0, Msg_size);
        Mymsg s_temp1 = send_generateMsg(source_port, dest_port, 1, 1, FIN | ACK, '0');//seq代表分组编号 ack=seq+1
        sendto(clientSocket, msg2char(s_temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        printf("[客户端]:发送第一次挥手FIN+ACK,校验和:%d,seq:%d,ack:%d\n", s_temp1.checksum, s_temp1.seq_number, s_temp1.acknowledge_number);

        int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(curr_buf));
        if (ret > 0 && check_sum(curr_buf, Msg_size) == 0 && r_temp1.isACK() && !r_temp1.isLAST()) {
            printf("[客户端]:收到第二次挥手ACK,seq:%d,ack:%d\n", r_temp1.msg.seq_number, r_temp1.msg.acknowledge_number);
            memset(curr_buf, 0, Msg_size);
            int ret2 = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
            Mymsg_explain r_temp2 = Mymsg_explain(receive_char2msg(curr_buf));
            if (ret2 > 0 && check_sum(curr_buf, Msg_size) == 0 && r_temp2.isFIN() && r_temp2.isACK()) {
                printf("[客户端]:收到第三次挥手FIN+ACK,seq:%d,ack:%d\n", r_temp2.msg.seq_number, r_temp2.msg.acknowledge_number);
                Mymsg s_temp2 = send_generateMsg(source_port, dest_port, r_temp2.msg.acknowledge_number, r_temp2.msg.seq_number + 1, ACK, '0');//seq代表分组编号 ack=seq+1
                sendto(clientSocket, msg2char(s_temp2), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                printf("[客户端]:发送第四次次挥手ACK,校验和:%d,seq:%d,ack:%d\n", s_temp2.checksum, s_temp2.seq_number, s_temp2.acknowledge_number);
                cout << "-----------------退出-------------" << endl;
                Sleep(2 * TIMEOUT);
                return;
            }
        }
        else
            continue;//从头开始
    }

}
int main() {
    // socket环境
    WSADATA  wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "初始化Socket DLL失败!" << endl;
        return 0;
    }
    else
        cout << "成功初始化Socket DLL,网络环境加载成功" << endl;

    // socket对象
    //使用数据报套接字
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET)
    {
        cout << "数据报套接字创建失败" << endl;
        WSACleanup();
        return 0;
    }
    else {
        cout << "数据报套接字创建成功" << endl;
    }
    // 绑定占用<ip, port>
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(dest_port);//路由器
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addrlen = sizeof(serverAddr);
    // 绑定占用<ip, port>
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(8080);
    clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int client_addrlen = sizeof(sockaddr_in);
    if (bind(clientSocket, (SOCKADDR*)&clientAddr, client_addrlen) == SOCKET_ERROR) {
        cout << "客户绑定端口和ip失败" << endl;
        WSACleanup();
    }
    else {
        cout << "客户绑定端口和Ip成功" << endl;
    }
    //三次握手
    hand3Shakes();
    cout << "请输入要传送的文件名:" << endl;
    cin >> filename;
    //传输文件
    send_file();
    hand4Shakes();
    closesocket(clientSocket);
    WSACleanup();
    return true;
}
