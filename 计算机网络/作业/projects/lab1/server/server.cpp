#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h> // winsock2��ͷ�ļ�
#include <iostream>
#include <map>
#include<time.h>
#include <string>
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")


using namespace std;
int client_total_num = 0;//��¼�ܹ�������
int client_id = 0;//���ڱ�ʶclient��id
map<SOCKET, string> client; // �洢socket���ǳƶ�Ӧ��ϵ,���ڷ�����ת����Ϣʱ����
#define msg_size 1080  //��װ��Mymsg���ֽڴ�С
#define content_size 1024//������Ϣ������С
#define MaxClient 5 //���ͻ�������5
#define idMax 100
SOCKET serverSocket;//������׽���
SOCKADDR_IN serverAddr;//�����������ַ
char server_name[10] = "ϵͳ����";//server������

//��װ��Ϣ�Ľṹ��
struct Mymsg {
    int id; //��ʶ��
    char name[31];//�ǳ�������31�ֽ���
    char online;//�ж��Ƿ�����
    char Time[20];//������Ϣ��ʱ��
    char content[1024];//��������
};//1080�ֽ�
int server_exitFlag = 0;//ϵͳ�Ƿ��˳��ı�ʶ��

//��Ҫ�ѷ�װ��messageת��Ϊcharȥ����
char* msg2char(Mymsg message) {
    char temp[msg_size];
    memcpy(temp, &message, sizeof(Mymsg));//ֱ�ӽ��ֽ�������
    return temp;
}


//����Ϣ���ݷ�װ��message
Mymsg send_char2msg(char* content, int _id, char* _name) {
    Mymsg temp;
    strcpy_s(temp.content, content);//��Ϣ���ݸ���
    char timeStr[20] = { 0 };
    time_t t = time(0);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&t));
    strcpy(temp.Time, timeStr);

    //�ж�content�Ƿ�Ϊexit
    if (strcmp(content, "exit") == 0) {
        temp.online = '0';//������
    }
    else {
        temp.online = '1';
    }
    memcpy(&temp, &_id, sizeof(int));
    strcpy(temp.name, _name);
    return temp;//���ط�װ�õ�message
}

//���յ�char*תΪmessage
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
    else if (t.id >= 1 && t.id <= idMax) { //��Ȼ����ֻ��5��������id����Ϊ100������������һ����Ϊ�����ֵ
        cout << t.Time << " (�ͻ�" << t.id << ") [" << t.name << "]:" << t.content << endl;
    }
    else {}
}
//����˽�����Ϣ���̺߳��� ����ת����Ϣ�����пͻ�
DWORD WINAPI ThreadReceive(LPVOID lpParameter) {
    int cur_id = client_id;//��ǰId
    SOCKET receive_socket = (SOCKET)lpParameter;
    //1.�����ǳ�
    char cur_clientName[31] = { 0 };//�ͻ�������
    int ret = recv(receive_socket, cur_clientName, 31, 0);
    if (ret == SOCKET_ERROR || ret <= 0) {//��ʼ��clientʱ����
        closesocket(receive_socket);
        return 0;
    }
    client[receive_socket] = string(cur_clientName);//�����ǳ�

    //��ʼ����ɣ����ڿ�ʼ�÷�װ��Msg������Ϣ
    //2.ϵͳ���ͻ�ӭ��Ϣ
    string stemp1 = (string)"client" + to_string(cur_id) + (string)"��������" + client[receive_socket] + (string)"��ta���ˣ���Ҷ�໶ӭ��!";
    string stemp2 = (string)"��ӭ��ĵ�������ǰ����client" + to_string(cur_id);//��Ե�ǰ�û���ӭ�ʲ�һ��

    char* welcome1 = (char*)stemp1.data();//��ӭ���
    Mymsg first_wel1 = send_char2msg(welcome1, 0, server_name);//��װ��Mymsg
    char* welcome2 = (char*)stemp2.data();//��ӭ���
    Mymsg first_wel2 = send_char2msg(welcome2, 0, server_name);//��װ��Mymsg
    for (auto i : client) {
        if (i.first == receive_socket)
            send(i.first, msg2char(first_wel2), sizeof(Mymsg), 0);
        else
            send(i.first, msg2char(first_wel1), sizeof(Mymsg), 0);
    }
    print_Mymsg(first_wel1);//ϵͳ��Ҳ��ӡ
    ret = 0;

    //3.��ʼѭ�����ܴ�client��Ϣ
    while (1) {
        char bufRecv[msg_size];
        ret = recv(receive_socket, bufRecv, msg_size, 0);
        if (ret != SOCKET_ERROR && ret > 0) { //����0��Ҳ����ͻ����˳�
            Mymsg tmp = receive_char2msg(bufRecv);
            tmp.id = cur_id;//���͵Ŀͻ��˲�֪��id��Ĭ��0������Ҫ�޸�
            if (tmp.online == '0') {//���λ���ж�
                //print_Mymsg(tmp); //�ͻ����˳���exit����ӡ
                break;//�ͻ����˳�
            }
            else {
                print_Mymsg(tmp);
                for (auto i : client)
                    send(i.first, msg2char(tmp), msg_size, 0);//ֱ�Ӱ��յ���ת������
            }
        }
        else {
            break;//ͬ����ʶΪ�ͻ����˳�
        }
    }
    if (server_exitFlag) {//ϵͳ�����˳���Ϣ
        closesocket(receive_socket);
        return 0;//ֱ���˳����̣߳��ƺ����ڷ��ͽ�����
    }
    //�ͻ��˳���ͬ��������������
    string str2 = (string)"�뿪�������ң�";
    char* p = (char*)str2.data();
    map<SOCKET, string>::iterator iter = client.find(receive_socket);
    client.erase(iter);//�˳������ң�Ҫ��client��¼��ӳ���ϵɾ��
    client_total_num--;//ע��id������٣�ֻ��������
    Mymsg exit_client = send_char2msg(p, cur_id, cur_clientName);//�뿪�ĳ�Ա�Զ�������Ϣ(�ͻ��˴���ת��)
    print_Mymsg(exit_client);
    for (auto i : client)
        send(i.first, msg2char(exit_client), msg_size, 0);
    closesocket(receive_socket);//������ͻ��˵Ľ��̹ص�����
    return 0;//׼���˳����߳�

}

//ϵͳ���ѵ���Ϣ����
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
        //�жϷ������Ƿ���ֹ
        if (send_msg.online == '0') {
            server_exitFlag = 1;
            string str2 = "������������ڴ˿̹رգ����λ�ͻ�׼���˳�������ʱ5��";
            char* p = (char*)str2.data();
            Mymsg tmp = send_char2msg(p, 0, server_name);
            for (auto i : client)
                ret = send(i.first, msg2char(tmp), msg_size, 0);
            //ǿ���˳�
            print_Mymsg(tmp);
            Sleep(5000);//�����н��ܽ����ͷŵ�receive_socket
            closesocket(send_socket);//send_socket��server_socket
            WSACleanup();
            exit(100);
        }
    }
    return 0;
}


int main()
{
    cout << "###############���ڴ���������ϵͳ��################" << endl;
    // ����winsock����
    WSAData  wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "��ʼ��Socket DLLʧ��!" << endl;
        return 0;
    }
    else
        cout << "�ɹ���ʼ��Socket DLL,���绷�����سɹ�" << endl;

    // �����׽���
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);//ʹ����ʽ�׽��� ����TCP�İ�˳��
    if (serverSocket == INVALID_SOCKET) {
        cout << "��ʽ�׽��ִ���ʧ��" << endl;
        WSACleanup();
    }
    else
        cout << "��ʽ�׽��ִ����ɹ�" << endl;

    // �����������׽��ְ�ip��ַ�Ͷ˿ڣ�bind����
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int len = sizeof(sockaddr_in);
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, len) == SOCKET_ERROR) {
        cout << "�������󶨶˿ں�ipʧ��" << endl;
        WSACleanup();
    }
    else {
        cout << "�������󶨶˿ں�Ip�ɹ�" << endl;
    }
    // �����˿�
    if (listen(serverSocket, MaxClient) != 0) {
        cout << "���ü���״̬ʧ�ܣ�" << endl;
        WSACleanup();//����ʹ��Socket���ͷ�socket dll��Դ
    }
    else
        cout << "���ü���״̬�ɹ���" << endl;

    cout << "���������������У����Ե�......" << endl;
    cout << "###############�ȴ��ͻ��˽���#####################" << endl;
    // ����һ���̣߳����ڷ�����������Ϣ
    CloseHandle(CreateThread(NULL, 0, ThreadSend, (LPVOID)serverSocket, 0, NULL));

    // ѭ�����ܣ��ͻ��˷���������
    while (1) {

        sockaddr_in clientAddr;//�½���ǰclient�ĵ�ַ
        len = sizeof(sockaddr_in);
        SOCKET cur_clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &len); //�����µĿͻ��˵��׽���
        if (cur_clientSocket == INVALID_SOCKET) { // һ��ʧ�����Ǿͳ���
            cout << "��ͻ�������ʧ��" << endl;
            closesocket(cur_clientSocket);
            WSACleanup();
            return 0;
        }
        client_total_num++;//�ͻ�����++���ͻ������ӵ��׽��� 
        client_id++;//id++

        //ifinit��Ϊ�ɹ����ӵ��жϷ���
        // �����ǰ���ӵĿͻ��������Ѿ��ﵽ6������ر������Ӳ������ȴ���һ����������
        char ifinit[2] = { '1','0' };
        if (client_total_num > MaxClient) {
            cout << "�Ѵﵽ������������ܾ�������" << endl;
            ifinit[0] = '0';
            send(cur_clientSocket, ifinit, 2, 0);//���ݸ��ͻ��˵ı�ʶ����������ʼ�ķ���
            Sleep(1000);//�ȴ�1��
            closesocket(cur_clientSocket);
            client_total_num--;//��ԭ
            client_id--;//idҲҪ��ԭ
            continue;
        }
        else {
            ifinit[1] = client_id + '0' - 0;
            send(cur_clientSocket, ifinit, 2, 0);
        }
        cout << "client_total_num(��ǰ�ͻ�����):" << client_total_num << endl;

        HANDLE hthread2 = CreateThread(NULL, 0, ThreadReceive, (LPVOID)cur_clientSocket, 0, NULL);//�����߳����ڽ��ܸÿͻ�����Ϣ
        //���̺߳��������ٽ���client��ӳ���ϵ
        if (hthread2 == NULL)//�̴߳���ʧ��
        {
            perror("The Thread is failed!\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            CloseHandle(hthread2);
        }

    }
    // �ر���socket���ӣ��ͷ���Դ
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
