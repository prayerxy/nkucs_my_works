#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include "udp.h"
#include <fstream>
#include <iostream>
#include <deque>
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")
using namespace  std;
SOCKET serverSocket;
SOCKADDR_IN serverAddr;
SOCKADDR_IN clientAddr;//这里其实是路由器
int client_addrlen = sizeof(clientAddr);
int name_flag = 0;//文件名是否发送完成
//源端口与目的端口
unsigned short source_port = 8000;
unsigned short dest_port = 4001;//路由器端口
char buffer[100000000];
int file_len;//文件的字节长度
unsigned short rcvbase = 0;
//接收窗口
struct SR_receive_ele {
    Mymsg m;
    bool receive = 0;
};
deque<SR_receive_ele>window(WINDOW_SIZE);
//3次握手
void hand3Shakes() {
    int tv = TIMEOUT * 4;
    //最后一次挥手可能丢包，这里避免阻塞
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    char buffer[Msg_size];
    while (1) {
        int bytesRead = recvfrom(serverSocket, (char*)buffer, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
        if (bytesRead <= 0 && check_sum(buffer, Msg_size)) {
            cout << "[服务器]:第一次握手重连中" << endl;
            continue;
        }
        else {
            Mymsg_explain cur = Mymsg_explain(receive_char2msg(buffer));
            if (check_sum(buffer, Msg_size) == 0 && cur.isSYN()) {
                cout << "[服务端]:第一次握手，接收到客户端SYN请求!" << endl;
                //记录客户端端口号
                dest_port = ntohs(clientAddr.sin_port);
                //准备发送SYN+ACK
                Mymsg temp = send_generateMsg(source_port, dest_port, cur.msg.acknowledge_number, cur.msg.seq_number + 1, SYN | ACK, '0');
                sendto(serverSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                cout << "[服务端]:第二次握手，发送SYN+ACK!" << endl;
                //清0buffer 重新接受
                memset(buffer, 0, Msg_size);
                int ret = recvfrom(serverSocket, (char*)buffer, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
                Mymsg_explain cur1 = Mymsg_explain(receive_char2msg(buffer));
                if (ret > 0 && (check_sum(buffer, Msg_size) == 0 && cur1.isACK())) {
                    cout << "[服务端]:第三次握手，收到ACK字段，成功连接！" << endl;
                    break;
                }
                else {
                    cout << "[服务端]:第三次握手失败,只能确保客户端发送服务器可靠\n";
                    break;

                }
            }
            else
                continue;
        }
        break;
    }
    cout << "[服务端]:三次握手流程结束" << endl;
    cout << "[服务器]:目的端口为:" << dest_port << endl;
}
//buffer储存收到的数据
void recv_Msgs(char* buffer, int& len) {
    //设置为阻塞模式
    int tv = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    len = 0;//字节数
    rcvbase = 0;
    int seq_len = 0;//分组数
    //分组序号 expectedsenum一个周期是65535
    //其中seq_len是记录所有分组的个数
    while (1) {
        char curr_buf[Msg_size];
        int ret = recvfrom(serverSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
        Mymsg msg_temp = receive_char2msg(curr_buf);
        Mymsg_explain r_temp = Mymsg_explain(msg_temp);
        if (ret > 0 && r_temp.hasnowseqnum(rcvbase) && check_sum(curr_buf, Msg_size) == 0) {
            if (name_flag == 1 && rcvbase == 0 && r_temp.isLAST() || r_temp.isACK() || r_temp.isSYN())
                continue;
            //printf("接受窗口size:1,接受seq:%d,check_sum=0\n", r_temp.msg.seq_number);
            //发送ACK 对接受到的分组确认
            Mymsg s_temp = send_generateMsg(source_port, dest_port, 0, r_temp.msg.seq_number, ACK, '0');//seq代表分组编号 ack=seq+1
            sendto(serverSocket, msg2char(s_temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("接收到分组，确认并发送ACK,ack:%d,校验和:%d\n", s_temp.acknowledge_number, s_temp.checksum);
            //首先缓存
            SR_receive_ele ele;
            ele.m = msg_temp;
            ele.receive = 1;
            //因为初始化及后续都保证了window的size，所以可以这样使用
            window[r_temp.msg.seq_number - rcvbase] = ele;
            //如果有序，交付多个
            if (rcvbase == r_temp.msg.seq_number) {
                while (window.size() && window.front().receive) {
                    //交付分组
                    if (window.front().m.length < MSS) {
                        memcpy(buffer + seq_len * MSS, window.front().m.data, window.front().m.length);
                        len += window.front().m.length;
                    }
                    else {
                        memcpy(buffer + seq_len * MSS, window.front().m.data, MSS);
                        len += MSS;
                    }
                    //判断是否为最后一个包
                    if (window.front().m.flag & LAST) {
                        if (name_flag) {
                            //文件传输完毕，要发一条消息告知
                            Sleep(20);
                            Mymsg s_temp = send_generateMsg(source_port, dest_port, 0, r_temp.msg.seq_number, ACK | LAST, '0');//seq代表分组编号 ack=seq+1
                            sendto(serverSocket, msg2char(s_temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                            printf("结束标志\n");
                        }
                        printf("接受完毕\n");
                        return;
                    }
                    rcvbase++;//窗口后沿前移 下一个期待接受seq
                    seq_len++;//与expectedseqnum区别是分组的总数而不是序号
                    window.pop_front();
                    //我们要保证window的尺寸，这里push空消息
                    SR_receive_ele nullmsg;
                    window.push_back(nullmsg);
                }
                //接收窗口移动的相关信息
                printf("进行交付后，接收窗口size:%d,recvbase:%d\n", WINDOW_SIZE, rcvbase);
            }


        }
        else if (ret > 0 && r_temp.hasprevseqnum(rcvbase) && check_sum(curr_buf, Msg_size) == 0) {
            //只用发送ACK
            Mymsg s_temp = send_generateMsg(source_port, dest_port, 0, r_temp.msg.seq_number, ACK, '0');//seq代表分组编号 ack=seq+1
            sendto(serverSocket, msg2char(s_temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("  重发ACK,ack:%d,校验和:%d\n", s_temp.acknowledge_number, s_temp.checksum);
        }
        else
            continue;
    }


}

void receive_file() {
    char name[30];
    int name_len;
    cout << "--------------开始接受文件名------------" << endl;
    recv_Msgs(name, name_len);
    cout << name << endl;
    string file_name(name);
    cout << "接收到文件名:" << file_name << endl;
    name_flag = 1;//文件名已收到
    //然后接收文件内容
    cout << "------------开始接受数据---------------" << endl;
    recv_Msgs(buffer, file_len);
    //写数据

    ofstream fout(file_name.c_str(), ofstream::binary);
    // 使用缓冲区
    fout.write(buffer, file_len);
    cout << "文件写入大小:" << file_len << "bytes" << endl;
    fout.close();
    cout << "-----------已成功写入文件------------------" << endl;
}

//4次挥手
void hand4Shakes() {
    int tv = TIMEOUT;
    //最后一次挥手可能丢包，这里避免阻塞
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    while (1) {
        char buf1[Msg_size];
        int bytesRead = recvfrom(serverSocket, (char*)buf1, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
        Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(buf1));
        if (bytesRead > 0 && r_temp1.isFIN() && r_temp1.isACK() && check_sum(buf1, Msg_size) == 0) {
            printf("[服务端]:收到第一次挥手的FIN+ACK,seq:%d,ack:%d\n", r_temp1.msg.seq_number, r_temp1.msg.acknowledge_number);
            Mymsg temp1 = send_generateMsg(source_port, dest_port, 1, r_temp1.msg.seq_number + 1, ACK, '0');//seq代表分组编号
            sendto(serverSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("[服务端]:发送第二次挥手的ACK,校验和:%d,seq:%d,ack:%d\n", temp1.checksum, temp1.seq_number, temp1.acknowledge_number);
            //这里服务器发送的不会丢包，简便起见不重传了
            Mymsg temp2 = send_generateMsg(source_port, dest_port, 1, r_temp1.msg.seq_number + 1, FIN | ACK, '0');//seq代表分组编号
            sendto(serverSocket, msg2char(temp2), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("[服务端]:发送第三次挥手的FIN+ACK,校验和:%d,seq:%d,ack:%d\n", temp2.checksum, temp2.seq_number, temp2.acknowledge_number);
            //第四次
            unsigned long long start_stamp = GetTickCount64();//开始计时
            while (1) {
                memset(buf1, 0, Msg_size);
                int bytesRead1 = recvfrom(serverSocket, (char*)buf1, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
                Mymsg_explain r_temp2 = Mymsg_explain(receive_char2msg(buf1));
                if (bytesRead > 0 && r_temp2.isACK() && check_sum(buf1, Msg_size) == 0) {
                    printf("[服务器]:收到第四次挥手的ACK,seq:%d,ack:%d\n", r_temp2.msg.seq_number, r_temp2.msg.acknowledge_number);
                    cout << "-------------------退出---------------" << endl;
                    return;
                }
                else {
                    if (GetTickCount64() - start_stamp > TIMEOUT) {
                        //客户发送的不可靠，可能不见了
                        cout << "[服务器]:第四次挥手因故障未收到，自动退出" << endl;
                        return;
                    }
                    else
                        continue;
                }
            }
        }
        else
            continue;
    }

}

//获取数据传输
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
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET)
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
    serverAddr.sin_port = htons(8000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int server_addrlen = sizeof(sockaddr_in);
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, server_addrlen) == SOCKET_ERROR) {
        cout << "服务器绑定端口和ip失败" << endl;
        WSACleanup();
    }
    else {
        cout << "服务器绑定端口和Ip成功" << endl;
    }
    //三次握手
    hand3Shakes();
    receive_file();
    hand4Shakes();

    closesocket(serverSocket);
    WSACleanup();
    return true;
}
