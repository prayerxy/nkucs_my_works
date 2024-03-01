#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include "udp.h"
#include <fstream>
#include <iostream>
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
//buffer是存储要传输的字节的数组
//len为总共的字节数
void sendMsg(char* buffer, int len) {
    //分组
    //使用停等机制，发送一个分组等待接收端响应

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    if (len > MSS) {
        int groupN = MSS * len / MSS < len ? len / MSS + 1 : len / MSS;
        //开始传送
        int seq = 0;
        //由于单向传输，服务器端只赋值seq字段，客户端只赋值ack字段，对应分组编号
        while (1) {
            //进入调用0
            if (seq >= groupN)
                break;
            Mymsg temp0 = send_generateMsg(source_port, dest_port, seq, 0, 0, '0');//seq代表分组编号
            //内容赋值
            if (seq == groupN - 1) {
                temp0.length = len - (groupN - 1) * MSS;
                temp0.flag |= LAST;
                memcpy(temp0.data, buffer + seq * MSS, len - (groupN - 1) * MSS);
            }
            else
                memcpy(temp0.data, buffer + seq * MSS, MSS);
            temp0.checksum = 0;
            char* p = msg2char(temp0);
            //产生checksum的值
            temp0.checksum = check_sum(p, Msg_size);
            sendto(clientSocket, msg2char(temp0), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            printf("发送分组seq:%d,分组状态0，校验和:%d\n", seq, temp0.checksum);
            unsigned long long start_stamp = GetTickCount64();//开始计时
            //进入等待ACK0状态
            while (1) {
                char curr_buf[Msg_size];
                int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
                Mymsg_explain r_temp0 = Mymsg_explain(receive_char2msg(curr_buf));
                //正确，准备进入下一个状态
                if (ret > 0 && r_temp0.isACK0() && check_sum(curr_buf, Msg_size) == 0) { printf("接受到确认ACK0,ack:%d\n", r_temp0.msg.acknowledge_number); break; }
                else {}//什么也不做等待超时
                //超时
                if (GetTickCount64() - start_stamp > timeout) {
                    //重传分组
                    if (resend_flag > MISSCOUNT) {
                        timeout *= 1.5;
                        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                        resend_flag = 0;
                        cout << "timeout修改为" << timeout << "ms\n";
                    }
                    sendto(clientSocket, msg2char(temp0), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                    //重新计时
                    start_stamp = GetTickCount64();
                    cout << "超时重传分组seq:" << temp0.seq_number << endl;
                    resend_flag++;
                }

            }
            resend_flag = 0;
            //进入调用1
            seq++;//下一个分组
            if (seq >= groupN)
                break;
            Mymsg temp1 = send_generateMsg(source_port, dest_port, seq, 0, 0, '1');//seq代表分组编号
            //内容赋值
            if (seq == groupN - 1) {
                temp1.length = len - (groupN - 1) * MSS;
                temp1.flag |= LAST;
                memcpy(temp1.data, buffer + seq * MSS, len - (groupN - 1) * MSS);
            }
            else
                memcpy(temp1.data, buffer + seq * MSS, MSS);
            temp1.checksum = 0;
            p = msg2char(temp1);
            //产生checksum的值
            temp1.checksum = check_sum(p, Msg_size);
            sendto(clientSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            printf("发送分组seq:%d,校验和:%d\n", seq, temp1.checksum);
            //开始计时
            start_stamp = GetTickCount64();
            //进入等待ACK1状态
            while (1) {
                char curr_buf1[Msg_size];
                int ret = recvfrom(clientSocket, curr_buf1, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
                Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(curr_buf1));

                //正确，准备进入下一个状态
                if (ret > 0 && r_temp1.isACK1() && check_sum(curr_buf1, Msg_size) == 0) { printf("接受到确认ACK1,ack:%d\n", r_temp1.msg.acknowledge_number); break; }
                else {}//什么也不做等待超时
                if (GetTickCount64() - start_stamp > timeout) {
                    if (resend_flag > MISSCOUNT) {
                        timeout *= 1.5;
                        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                        resend_flag = 0;
                        cout << "timeout修改为" << timeout << "ms\n";
                    }
                    //重传分组
                    sendto(clientSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                    //重新计时
                    start_stamp = GetTickCount64();
                    cout << "超时重传分组seq:" << temp1.seq_number << endl;
                    resend_flag++;
                }

            }
            seq++;//下一个
            resend_flag = 0;
            //第四个状态完毕，从头开始
            continue;
        }
        cout << "成功传输完数据" << endl;
    }
    //传一个即可
    else {

        Mymsg temp = send_generateMsg(source_port, dest_port, 0, 0, LAST, '0');//seq代表分组编号
        memcpy(temp.data, buffer, len);
        temp.length = len;
        temp.checksum = 0;
        char* p = msg2char(temp);
        //产生checksum的值
        temp.checksum = check_sum(p, Msg_size);
        sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        printf("发送分组seq:%d,校验和:%d\n", 0, temp.checksum);
        unsigned long long start_stamp = GetTickCount64();//开始计时
        while (1) {
            char curr_buf[Msg_size];
            int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
            Mymsg_explain r_temp = Mymsg_explain(receive_char2msg(curr_buf));
            //什么也不做等待超时
            if (ret <= 0 || ret > 0 && r_temp.isACK1()) {}
            //正确，准备进入下一个状态
            if (ret > 0 && r_temp.isACK0() && check_sum(curr_buf, Msg_size) == 0) { break; }
            if (GetTickCount64() - start_stamp > timeout) {
                if (resend_flag > MISSCOUNT) {
                    timeout *= 1.5;
                    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                    resend_flag = 0;
                    cout << "timeout修改为" << timeout << "ms\n";
                }
                //重传分组
                sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                //重新计时
                start_stamp = GetTickCount64();
                cout << "超时重传分组seq:" << temp.seq_number << endl;
                resend_flag++;
            }
        }
        resend_flag = 0;
        cout << "成功传输完数据" << endl;
    }
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
    sendMsg((char*)filename, 30);
    cout << "-------------开始传输文件内容--------------------" << endl;
    unsigned long long start_stamp1 = GetTickCount64();//开始计时
    sendMsg(buffer, file_len);
    unsigned long long end_stamp1 = GetTickCount64();//开始计时
    cout << "-------------------" << endl;
    cout << "传输文件大小:" << file_len << "bytes" << endl;
    cout << "文件传输时间:" << end_stamp1 - start_stamp1 << "ms\n";
    double kbps = (file_len * 8.0) / (end_stamp1 - start_stamp1);
    cout << "吞吐率为:" << kbps << "kbps" << endl;
    cout << "--------------------" << endl;

}

void hand4Shakes() {
    while (1) {
        Mymsg s_temp1 = send_generateMsg(source_port, dest_port, 1, 1, FIN | ACK, '0');//seq代表分组编号 ack=seq+1
        sendto(clientSocket, msg2char(s_temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        printf("[客户端]:发送第一次挥手FIN+ACK,校验和:%d,seq:%d,ack:%d\n", s_temp1.checksum, s_temp1.seq_number, s_temp1.acknowledge_number);
        char curr_buf[Msg_size];
        int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(curr_buf));
        if (ret > 0 && check_sum(curr_buf, Msg_size) == 0 && r_temp1.isACK()) {
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
    cout << "请输入要传送的文件名:" << endl;
    cin >> filename;
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
    //传输文件
    send_file();
    hand4Shakes();
    closesocket(clientSocket);
    WSACleanup();
    return true;
}
