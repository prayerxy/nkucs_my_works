#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h> // winsock2的头文件
#include <iostream>
#include <map>
#include<time.h>
#include <string>
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")


using namespace std;
int client_total_num = 0;//记录总共连接数
int client_id = 0;//用于标识client的id
map<SOCKET, string> client; // 存储socket和昵称对应关系,用于服务器转发消息时遍历
#define msg_size 1080  //封装的Mymsg的字节大小
#define content_size 1024//发送消息的最大大小
#define MaxClient 5 //最大客户端数量5
#define idMax 100
SOCKET serverSocket;//服务端套接字
SOCKADDR_IN serverAddr;//定义服务器地址
char server_name[10] = "系统提醒";//server的名字

//封装消息的结构体
struct Mymsg {
    int id; //标识符
    char name[31];//昵称限制在31字节内
    char online;//判断是否在线
    char Time[20];//发送消息的时间
    char content[1024];//发送内容
};//1080字节
int server_exitFlag = 0;//系统是否退出的标识符

//需要把封装的message转化为char去发送
char* msg2char(Mymsg message) {
    char temp[msg_size];
    memcpy(temp, &message, sizeof(Mymsg));//直接将字节数拷贝
    return temp;
}


//把消息内容封装成message
Mymsg send_char2msg(char* content, int _id, char* _name) {
    Mymsg temp;
    strcpy_s(temp.content, content);//消息内容复制
    char timeStr[20] = { 0 };
    time_t t = time(0);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&t));
    strcpy(temp.Time, timeStr);

    //判断content是否为exit
    if (strcmp(content, "exit") == 0) {
        temp.online = '0';//不在线
    }
    else {
        temp.online = '1';
    }
    memcpy(&temp, &_id, sizeof(int));
    strcpy(temp.name, _name);
    return temp;//返回封装好的message
}

//接收的char*转为message
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
    else if (t.id >= 1 && t.id <= idMax) { //虽然总数只有5个，但是id可能为100，这里设置了一个较为合理的值
        cout << t.Time << " (客户" << t.id << ") [" << t.name << "]:" << t.content << endl;
    }
    else {}
}
//服务端接受消息的线程函数 负责转发消息给所有客户
DWORD WINAPI ThreadReceive(LPVOID lpParameter) {
    int cur_id = client_id;//当前Id
    SOCKET receive_socket = (SOCKET)lpParameter;
    //1.储存昵称
    char cur_clientName[31] = { 0 };//客户的名称
    int ret = recv(receive_socket, cur_clientName, 31, 0);
    if (ret == SOCKET_ERROR || ret <= 0) {//初始化client时出错
        closesocket(receive_socket);
        return 0;
    }
    client[receive_socket] = string(cur_clientName);//储存昵称

    //初始化完成，现在开始用封装的Msg传递信息
    //2.系统发送欢迎消息
    string stemp1 = (string)"client" + to_string(cur_id) + (string)"的名字是" + client[receive_socket] + (string)"，ta来了，大家多多欢迎吧!";
    string stemp2 = (string)"欢迎你的到来，当前你是client" + to_string(cur_id);//针对当前用户欢迎词不一样

    char* welcome1 = (char*)stemp1.data();//欢迎语句
    Mymsg first_wel1 = send_char2msg(welcome1, 0, server_name);//封装成Mymsg
    char* welcome2 = (char*)stemp2.data();//欢迎语句
    Mymsg first_wel2 = send_char2msg(welcome2, 0, server_name);//封装成Mymsg
    for (auto i : client) {
        if (i.first == receive_socket)
            send(i.first, msg2char(first_wel2), sizeof(Mymsg), 0);
        else
            send(i.first, msg2char(first_wel1), sizeof(Mymsg), 0);
    }
    print_Mymsg(first_wel1);//系统端也打印
    ret = 0;

    //3.开始循环接受此client消息
    while (1) {
        char bufRecv[msg_size];
        ret = recv(receive_socket, bufRecv, msg_size, 0);
        if (ret != SOCKET_ERROR && ret > 0) { //接受0个也代表客户端退出
            Mymsg tmp = receive_char2msg(bufRecv);
            tmp.id = cur_id;//发送的客户端不知道id，默认0，这里要修改
            if (tmp.online == '0') {//标记位的判断
                //print_Mymsg(tmp); //客户端退出的exit不打印
                break;//客户端退出
            }
            else {
                print_Mymsg(tmp);
                for (auto i : client)
                    send(i.first, msg2char(tmp), msg_size, 0);//直接把收到的转发即可
            }
        }
        else {
            break;//同样标识为客户端退出
        }
    }
    if (server_exitFlag) {//系统发送退出消息
        closesocket(receive_socket);
        return 0;//直接退出此线程，善后工作在发送进程中
    }
    //客户退出，同步给其余所有人
    string str2 = (string)"离开了聊天室！";
    char* p = (char*)str2.data();
    map<SOCKET, string>::iterator iter = client.find(receive_socket);
    client.erase(iter);//退出聊天室，要把client记录的映射关系删掉
    client_total_num--;//注意id不会减少，只减少总数
    Mymsg exit_client = send_char2msg(p, cur_id, cur_clientName);//离开的成员自动发送消息(客户端代替转发)
    print_Mymsg(exit_client);
    for (auto i : client)
        send(i.first, msg2char(exit_client), msg_size, 0);
    closesocket(receive_socket);//把这个客户端的进程关掉即可
    return 0;//准备退出此线程

}

//系统提醒的消息发送
DWORD WINAPI ThreadSend(LPVOID lpParameter) {
    SOCKET send_socket = (SOCKET)lpParameter;
    int ret = 0;
    while (1) {
        char bufsend[content_size] = { 0 };
        cin.getline(bufsend, content_size);
        Mymsg send_msg = send_char2msg(bufsend, 0, server_name);
        print_Mymsg(send_msg);
        for (auto i : client)
            ret = send(i.first, msg2char(send_msg), msg_size, 0);
        //判断服务器是否终止
        if (send_msg.online == '0') {
            server_exitFlag = 1;
            string str2 = "聊天服务器将于此刻关闭，请各位客户准备退出，倒计时5秒";
            char* p = (char*)str2.data();
            Mymsg tmp = send_char2msg(p, 0, server_name);
            for (auto i : client)
                ret = send(i.first, msg2char(tmp), msg_size, 0);
            //强制退出
            print_Mymsg(tmp);
            Sleep(5000);//让所有接受进程释放掉receive_socket
            closesocket(send_socket);//send_socket是server_socket
            WSACleanup();
            exit(100);
        }
    }
    return 0;
}


int main()
{
    cout << "###############正在创建聊天室系统端################" << endl;
    // 加载winsock环境
    WSAData  wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "初始化Socket DLL失败!" << endl;
        return 0;
    }
    else
        cout << "成功初始化Socket DLL,网络环境加载成功" << endl;

    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);//使用流式套接字 基于TCP的按顺序
    if (serverSocket == INVALID_SOCKET) {
        cout << "流式套接字创建失败" << endl;
        WSACleanup();
    }
    else
        cout << "流式套接字创建成功" << endl;

    // 给服务器的套接字绑定ip地址和端口：bind函数
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int len = sizeof(sockaddr_in);
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, len) == SOCKET_ERROR) {
        cout << "服务器绑定端口和ip失败" << endl;
        WSACleanup();
    }
    else {
        cout << "服务器绑定端口和Ip成功" << endl;
    }
    // 监听端口
    if (listen(serverSocket, MaxClient) != 0) {
        cout << "设置监听状态失败！" << endl;
        WSACleanup();//结束使用Socket，释放socket dll资源
    }
    else
        cout << "设置监听状态成功！" << endl;

    cout << "服务器监听连接中，请稍等......" << endl;
    cout << "###############等待客户端进入#####################" << endl;
    // 创建一个线程，用于服务器发送消息
    CloseHandle(CreateThread(NULL, 0, ThreadSend, (LPVOID)serverSocket, 0, NULL));

    // 循环接受：客户端发来的连接
    while (1) {

        sockaddr_in clientAddr;//新建当前client的地址
        len = sizeof(sockaddr_in);
        SOCKET cur_clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &len); //创建新的客户端的套接字
        if (cur_clientSocket == INVALID_SOCKET) { // 一个失败我们就撤退
            cout << "与客户端连接失败" << endl;
            closesocket(cur_clientSocket);
            WSACleanup();
            return 0;
        }
        client_total_num++;//客户总数++，客户端连接到套接字 
        client_id++;//id++

        //ifinit作为成功连接的判断符号
        // 如果当前连接的客户端数量已经达到6个，则关闭新连接并继续等待下一个连接请求
        char ifinit[2] = { '1','0' };
        if (client_total_num > MaxClient) {
            cout << "已达到最大连接数，拒绝新连接" << endl;
            ifinit[0] = '0';
            send(cur_clientSocket, ifinit, 2, 0);//传递给客户端的标识符，用作初始的符号
            Sleep(1000);//等待1秒
            closesocket(cur_clientSocket);
            client_total_num--;//还原
            client_id--;//id也要还原
            continue;
        }
        else {
            ifinit[1] = client_id + '0' - 0;
            send(cur_clientSocket, ifinit, 2, 0);
        }
        cout << "client_total_num(当前客户人数):" << client_total_num << endl;

        HANDLE hthread2 = CreateThread(NULL, 0, ThreadReceive, (LPVOID)cur_clientSocket, 0, NULL);//创建线程用于接受该客户端消息
        //在线程函数里面再建立client的映射关系
        if (hthread2 == NULL)//线程创建失败
        {
            perror("The Thread is failed!\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            CloseHandle(hthread2);
        }

    }
    // 关闭主socket连接，释放资源
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
