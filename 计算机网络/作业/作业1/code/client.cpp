#define  _CRT_SECURE_NO_WARNINGS
#include<bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h> // winsock2的头文件
#include <iostream>
#include <map>
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")
using namespace std;
#define msg_size 1080
#define idMax 100
int cur_clientID;

struct Mymsg {
    int id; //标识符
    char name[31];//昵称限制在31字节内
    char online;//判断是否在线
    char Time[20];//发送消息的时间
    char content[1024];//发送内容
};//1080字节

SOCKET clientSocket;//客户端套接字
sockaddr_in serverAddr; // 服务器地址

//需要把封装的message转化为char去发送
char* msg2char(Mymsg message) {
    char temp[msg_size];
    memcpy(temp, &message, sizeof(Mymsg));//直接将字节数拷贝
    return temp;
}


//把消息内容封装成message
Mymsg send_char2msg(char* _content, int _id, char* _name) {
    Mymsg temp;
    strcpy_s(temp.content, _content);//消息内容复制
    char tmp[20] = { NULL };
    time_t t = time(0);
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    strcpy(temp.Time, tmp);

    //判断content是否为exit
    if (strcmp(_content, "exit") == 0) {
        temp.online = '0';//不在线
    }
    else {
        temp.online = '1';
    }
    memcpy(&temp, &_id, sizeof(int));
    strcpy(temp.name, _name);
    return temp;//返回封装好的message
}

//接收的char*转为message 这里直接拷贝即可
Mymsg receive_char2msg(char* receive_buf) {
    Mymsg temp;
    memcpy(&temp, receive_buf, sizeof(Mymsg));
    return temp;
}
//格式输出
void print_Mymsg(Mymsg t) {
    if (t.id == 0) {
        cout << t.Time << " " << "[" << t.name << "]:" << t.content << endl;
    }
    else if (t.id >= 1 && t.id <= idMax) {
        if (cur_clientID == t.id) {
            cout << t.Time << " (我自己)" << "[" << t.name << "]:" << t.content << endl;
        }
        else
            cout << t.Time << " (客户" << t.id << ") [" << t.name << "]:" << t.content << endl;
    }
    else {}

}

//这里创建一个子进程来收客户端消息即可，主进程用来发消息
DWORD WINAPI ThreadReceive() {
    int ret = 0;
    while (1) {
        char bufrecv[msg_size] = { 0 }; //用来接受数据
        ret = recv(clientSocket, bufrecv, msg_size, 0);
        if (ret == SOCKET_ERROR || ret <= 0) {
            cout << "连接错误！2s后退出" << endl;
            Sleep(2000);
            closesocket(clientSocket);
            WSACleanup();
            exit(100); //强制所有线程全部退出
        }
        Mymsg temp = receive_char2msg(bufrecv);

        if (temp.id == 0) {
            if (temp.online == '0') {//系统端要退出
                char exitMessage[msg_size] = { 0 };
                recv(clientSocket, exitMessage, msg_size, 0);
                print_Mymsg(receive_char2msg(exitMessage));//打印exitMessage后退出
                Sleep(2000);
                closesocket(clientSocket);//释放资源
                WSACleanup();
                exit(100);//由于系统端终止，强制客户端退出
                //break;
            }
        }
        //说明服务器没有终止，打印msg
        print_Mymsg(temp);
    }
    return 0;
}

int main()
{
    // 加载winsock环境
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "初始化Socket DLL失败!" << endl;
        return 0;
    }
    else
        cout << "成功初始化Socket DLL,网络环境加载成功" << endl;

    // 创建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {

        cout << "流式套接字失败" << endl;
        WSACleanup();
    }
    else
        cout << "流式套接字成功" << endl;

    // 给套接字绑定ip地址和端口：bind函数
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int len = sizeof(sockaddr_in);
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, len) == SOCKET_ERROR) {
        cout << "客户端连接失败" << endl;
        WSACleanup();
        return 0;
    }
    else
        cout << "客户端连接成功" << endl;
    cout << "###############正在创建聊天室客户端端################" << endl;
    cout << "please send a message or use \"exit\" to exit / 请发送消息或使用exit退出" << endl;
    cout << "正在初始化........请稍等" << endl;

    char ifinit[2];//初始2bit，判断是否成功
    recv(clientSocket, ifinit, 2, 0);
    if (ifinit[0] == '0') {
        cout << "###########聊天室客户端端创建失败，倒计时3秒退出############" << endl;
        Sleep(3000);//倒计时三秒
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    else {
        cur_clientID = ifinit[1] + 0 - '0';
        cout << "###############聊天室客户端端创建成功################" << endl;
    }
    // 发送和接受数据即可
    string name;
    cout << "聊天前请输入你的昵称：";
    getline(cin, name); // 读入一整行，可以有空格
    char* client_name = (char*)name.data();
    int ret = send(clientSocket, name.data(), 31, 0);//和server匹配，31字节
    if (ret == SOCKET_ERROR || ret <= 0) {
        cout << "连接错误！3s后退出";
        // 关闭连接，释放资源
        Sleep(3000);
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    // 建立连接后，创建线程用于接受数据，主线程用来发送数据
    // 初始化全部完成，这里开始使用封装的msg传递信息
    ret = 0;
    CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadReceive, NULL, 0, NULL));//创建子线程
    //在线程函数里面再建立client的映射关系
    while (1) {

        char bufrecv[1024] = { 0 };
        cin.getline(bufrecv, 1024);
        Mymsg temp = send_char2msg(bufrecv, 0, client_name);//这里的0不是id，只是默认传参，在服务端传的时候会知道id

        ret = send(clientSocket, msg2char(temp), msg_size, 0);
        //print_Mymsg(temp);  接受服务器发送的再打印，这个时候才能获得id
        if (ret == SOCKET_ERROR || ret <= 0) {
            cout << "发送消息失败!程序在3秒后退出" << endl;
            Sleep(3000);
            break;
        }
        else {
            if (temp.online == '0') {
                cout << "自动提醒：客户端" << client_name << "在3秒后退出" << endl;
                Sleep(3000);
                break;
            }
        }

    }
    // 关闭连接，释放资源
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
