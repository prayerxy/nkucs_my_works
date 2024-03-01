#define  _CRT_SECURE_NO_WARNINGS
#include<bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h> // winsock2��ͷ�ļ�
#include <iostream>
#include <map>
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")
using namespace std;
#define msg_size 1080
#define idMax 100
int cur_clientID;

struct Mymsg {
    int id; //��ʶ��
    char name[31];//�ǳ�������31�ֽ���
    char online;//�ж��Ƿ�����
    char Time[20];//������Ϣ��ʱ��
    char content[1024];//��������
};//1080�ֽ�

SOCKET clientSocket;//�ͻ����׽���
sockaddr_in serverAddr; // ��������ַ

//��Ҫ�ѷ�װ��messageת��Ϊcharȥ����
char* msg2char(Mymsg message) {
    char temp[msg_size];
    memcpy(temp, &message, sizeof(Mymsg));//ֱ�ӽ��ֽ�������
    return temp;
}


//����Ϣ���ݷ�װ��message
Mymsg send_char2msg(char* _content, int _id, char* _name) {
    Mymsg temp;
    strcpy_s(temp.content, _content);//��Ϣ���ݸ���
    char tmp[20] = { NULL };
    time_t t = time(0);
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    strcpy(temp.Time, tmp);

    //�ж�content�Ƿ�Ϊexit
    if (strcmp(_content, "exit") == 0) {
        temp.online = '0';//������
    }
    else {
        temp.online = '1';
    }
    memcpy(&temp, &_id, sizeof(int));
    strcpy(temp.name, _name);
    return temp;//���ط�װ�õ�message
}

//���յ�char*תΪmessage ����ֱ�ӿ�������
Mymsg receive_char2msg(char* receive_buf) {
    Mymsg temp;
    memcpy(&temp, receive_buf, sizeof(Mymsg));
    return temp;
}
//��ʽ���
void print_Mymsg(Mymsg t) {
    if (t.id == 0) {
        cout << t.Time << " " << "[" << t.name << "]:" << t.content << endl;
    }
    else if (t.id >= 1 && t.id <= idMax) {
        if (cur_clientID == t.id) {
            cout << t.Time << " (���Լ�)" << "[" << t.name << "]:" << t.content << endl;
        }
        else
            cout << t.Time << " (�ͻ�" << t.id << ") [" << t.name << "]:" << t.content << endl;
    }
    else {}

}

//���ﴴ��һ���ӽ������տͻ�����Ϣ���ɣ���������������Ϣ
DWORD WINAPI ThreadReceive() {
    int ret = 0;
    while (1) {
        char bufrecv[msg_size] = { 0 }; //������������
        ret = recv(clientSocket, bufrecv, msg_size, 0);
        if (ret == SOCKET_ERROR || ret <= 0) {
            cout << "���Ӵ���2s���˳�" << endl;
            Sleep(2000);
            closesocket(clientSocket);
            WSACleanup();
            exit(100); //ǿ�������߳�ȫ���˳�
        }
        Mymsg temp = receive_char2msg(bufrecv);

        if (temp.id == 0) {
            if (temp.online == '0') {//ϵͳ��Ҫ�˳�
                char exitMessage[msg_size] = { 0 };
                recv(clientSocket, exitMessage, msg_size, 0);
                print_Mymsg(receive_char2msg(exitMessage));//��ӡexitMessage���˳�
                Sleep(2000);
                closesocket(clientSocket);//�ͷ���Դ
                WSACleanup();
                exit(100);//����ϵͳ����ֹ��ǿ�ƿͻ����˳�
                //break;
            }
        }
        //˵��������û����ֹ����ӡmsg
        print_Mymsg(temp);
    }
    return 0;
}

int main()
{
    // ����winsock����
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "��ʼ��Socket DLLʧ��!" << endl;
        return 0;
    }
    else
        cout << "�ɹ���ʼ��Socket DLL,���绷�����سɹ�" << endl;

    // �����׽���
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {

        cout << "��ʽ�׽���ʧ��" << endl;
        WSACleanup();
    }
    else
        cout << "��ʽ�׽��ֳɹ�" << endl;

    // ���׽��ְ�ip��ַ�Ͷ˿ڣ�bind����
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int len = sizeof(sockaddr_in);
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, len) == SOCKET_ERROR) {
        cout << "�ͻ�������ʧ��" << endl;
        WSACleanup();
        return 0;
    }
    else
        cout << "�ͻ������ӳɹ�" << endl;
    cout << "###############���ڴ��������ҿͻ��˶�################" << endl;
    cout << "please send a message or use \"exit\" to exit / �뷢����Ϣ��ʹ��exit�˳�" << endl;
    cout << "���ڳ�ʼ��........���Ե�" << endl;

    char ifinit[2];//��ʼ2bit���ж��Ƿ�ɹ�
    recv(clientSocket, ifinit, 2, 0);
    if (ifinit[0] == '0') {
        cout << "###########�����ҿͻ��˶˴���ʧ�ܣ�����ʱ3���˳�############" << endl;
        Sleep(3000);//����ʱ����
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    else {
        cur_clientID = ifinit[1] + 0 - '0';
        cout << "###############�����ҿͻ��˶˴����ɹ�################" << endl;
    }
    // ���ͺͽ������ݼ���
    string name;
    cout << "����ǰ����������ǳƣ�";
    getline(cin, name); // ����һ���У������пո�
    char* client_name = (char*)name.data();
    int ret = send(clientSocket, name.data(), 31, 0);//��serverƥ�䣬31�ֽ�
    if (ret == SOCKET_ERROR || ret <= 0) {
        cout << "���Ӵ���3s���˳�";
        // �ر����ӣ��ͷ���Դ
        Sleep(3000);
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    // �������Ӻ󣬴����߳����ڽ������ݣ����߳�������������
    // ��ʼ��ȫ����ɣ����￪ʼʹ�÷�װ��msg������Ϣ
    ret = 0;
    CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadReceive, NULL, 0, NULL));//�������߳�
    //���̺߳��������ٽ���client��ӳ���ϵ
    while (1) {

        char bufrecv[1024] = { 0 };
        cin.getline(bufrecv, 1024);
        Mymsg temp = send_char2msg(bufrecv, 0, client_name);//�����0����id��ֻ��Ĭ�ϴ��Σ��ڷ���˴���ʱ���֪��id

        ret = send(clientSocket, msg2char(temp), msg_size, 0);
        //print_Mymsg(temp);  ���ܷ��������͵��ٴ�ӡ�����ʱ����ܻ��id
        if (ret == SOCKET_ERROR || ret <= 0) {
            cout << "������Ϣʧ��!������3����˳�" << endl;
            Sleep(3000);
            break;
        }
        else {
            if (temp.online == '0') {
                cout << "�Զ����ѣ��ͻ���" << client_name << "��3����˳�" << endl;
                Sleep(3000);
                break;
            }
        }

    }
    // �ر����ӣ��ͷ���Դ
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
