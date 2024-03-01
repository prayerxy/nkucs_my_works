#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include "udp.h"
#include <fstream>
#include <iostream>
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")
using namespace  std;
SOCKET serverSocket;
SOCKADDR_IN serverAddr;
SOCKADDR_IN clientAddr;//������ʵ��·����
int client_addrlen = sizeof(clientAddr);
int name_flag = 0;//�ļ����Ƿ������
//Դ�˿���Ŀ�Ķ˿�
unsigned short source_port = 8000;
unsigned short dest_port = 4001;//·�����˿�
char buffer[100000000];
int file_len;//�ļ����ֽڳ���
//3������
void hand3Shakes() {
    int tv = TIMEOUT * 4;
    //���һ�λ��ֿ��ܶ����������������
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    char buffer[Msg_size];
    while (1) {
        int bytesRead = recvfrom(serverSocket, (char*)buffer, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
        if (bytesRead <= 0 && check_sum(buffer, Msg_size)) {
            cout << "[������]:��һ������������" << endl;
            continue;
        }
        else {
            Mymsg_explain cur = Mymsg_explain(receive_char2msg(buffer));
            if (check_sum(buffer, Msg_size) == 0 && cur.isSYN()) {
                cout << "[�����]:��һ�����֣����յ��ͻ���SYN����!" << endl;
                //��¼�ͻ��˶˿ں�
                dest_port = ntohs(clientAddr.sin_port);
                //׼������SYN+ACK
                Mymsg temp = send_generateMsg(source_port, dest_port, cur.msg.acknowledge_number, cur.msg.seq_number + 1, SYN | ACK, '0');
                sendto(serverSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                cout << "[�����]:�ڶ������֣�����SYN+ACK!" << endl;
                //��0buffer ���½���
                memset(buffer, 0, Msg_size);
                int ret = recvfrom(serverSocket, (char*)buffer, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
                Mymsg_explain cur1 = Mymsg_explain(receive_char2msg(buffer));
                if (ret > 0 && (check_sum(buffer, Msg_size) == 0 && cur1.isACK())) {
                    cout << "[�����]:���������֣��յ�ACK�ֶΣ��ɹ����ӣ�" << endl;
                    break;
                }
                else {
                    cout << "[�����]:����������ʧ��,ֻ��ȷ���ͻ��˷��ͷ������ɿ�\n";
                    break;

                }
            }
            else
                continue;
        }
        break;
    }
    cout << "[�����]:�����������̽���" << endl;
    cout << "[������]:Ŀ�Ķ˿�Ϊ:" << dest_port << endl;
}
//buffer�����յ�������
void recvMsg(char* buffer, int& len) {
    //���ó�ʱʱ��
    len = 0;
    int seq = -1;
    while (1) {
        while (1) {
            char curr_buf[Msg_size];
            int ret = recvfrom(serverSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
            Mymsg_explain r_temp0 = Mymsg_explain(receive_char2msg(curr_buf));
            //cout << r_temp0.msg.data;
            //��ȷ��׼��������һ��״̬
            if (ret > 0 && r_temp0.isSeq0() && check_sum(curr_buf, Msg_size) == 0) {
                //��ȡ����
                if (name_flag == 1 && seq == -1 && r_temp0.isLAST() || r_temp0.isACK() || r_temp0.isSYN())
                    continue;//������Ϣ ˵���ļ����ش���,�����ش��ˣ���Ҫ����
                seq = r_temp0.msg.seq_number;//��ʼ����Ϊ0
                printf("���յ�����seq:%d,����״̬:0,check_sum:0\n", seq);
                if (r_temp0.msg.length < MSS) {
                    memcpy(buffer + seq * MSS, r_temp0.msg.data, r_temp0.msg.length);
                    len += r_temp0.msg.length;
                }
                else {
                    memcpy(buffer + seq * MSS, r_temp0.msg.data, MSS);
                    len += MSS;
                }
                Mymsg s_temp0 = send_generateMsg(source_port, dest_port, 0, seq + 1, ACK, '0');//seq��������� ack=seq+1
                sendto(serverSocket, msg2char(s_temp0), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                cout << "����ack0,�ڴ�����:" << seq + 1 << endl;
                if (r_temp0.isLAST()) {
                    cout << "�������" << endl;
                    return;
                }
                break;
            }
            else {
                //����ack1 -1+1=0 �ڴ�����0
                Mymsg temp1 = send_generateMsg(source_port, dest_port, 0, seq + 1, ACK, '1');//seq��������� ack=seq+1
                sendto(serverSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                cout << "�ط�ack1,�ڴ�����:" << seq + 1 << endl;
            }
        }
        //�ȴ����ܷ���1
        while (1) {
            char curr_buf[Msg_size];
            int ret = recvfrom(serverSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
            Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(curr_buf));
            //��ȷ��׼��������һ��״̬
            if (ret > 0 && r_temp1.isSeq1() && check_sum(curr_buf, Msg_size) == 0) {
                //��ȡ����
                seq = r_temp1.msg.seq_number;
                printf("���յ�����seq:%d,����״̬:1,check_sum:0\n", seq);
                if (r_temp1.msg.length < MSS) {
                    memcpy(buffer + seq * MSS, r_temp1.msg.data, r_temp1.msg.length);
                    len += r_temp1.msg.length;

                }
                else {
                    memcpy(buffer + seq * MSS, r_temp1.msg.data, MSS);
                    len += MSS;
                }
                Mymsg s_temp1 = send_generateMsg(source_port, dest_port, 0, seq + 1, ACK, '1');//seq��������� ack=seq+1
                sendto(serverSocket, msg2char(s_temp1), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                cout << "����ack1,�ڴ�����:" << seq + 1 << endl;
                if (r_temp1.isLAST()) {
                    cout << "�������" << endl;
                    return;
                }
                break;
            }
            else {
                //����ack0
                Mymsg temp0 = send_generateMsg(source_port, dest_port, 0, seq + 1, ACK, '0');//seq��������� ack=seq+1
                sendto(serverSocket, msg2char(temp0), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                cout << "�ط�ack0���ڴ�����:" << seq + 1 << endl;
            }
        }
    }

}
void receive_file() {
    char name[30];
    int name_len;
    cout << "--------------��ʼ�����ļ���------------" << endl;
    recvMsg(name, name_len);
    cout << name << endl;
    string file_name(name);
    cout << "���յ��ļ���:" << file_name << endl;
    name_flag = 1;//�ļ������յ�
    //Ȼ������ļ�����
    cout << "------------��ʼ��������---------------" << endl;
    recvMsg(buffer, file_len);
    //д����

    ofstream fout(file_name.c_str(), ofstream::binary);
    // ʹ�û�����
    fout.write(buffer, file_len);
    cout << "�ļ�д���С:" << file_len << "bytes" << endl;
    fout.close();
    cout << "-----------�ѳɹ�д���ļ�------------------" << endl;
}

//4�λ���
void hand4Shakes() {
    int tv = TIMEOUT;
    //���һ�λ��ֿ��ܶ����������������
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    while (1) {
        char buf1[Msg_size];
        int bytesRead = recvfrom(serverSocket, (char*)buf1, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
        Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(buf1));
        if (bytesRead > 0 && r_temp1.isFIN() && r_temp1.isACK() && check_sum(buf1, Msg_size) == 0) {
            printf("[�����]:�յ���һ�λ��ֵ�FIN+ACK,seq:%d,ack:%d\n", r_temp1.msg.seq_number, r_temp1.msg.acknowledge_number);
            Mymsg temp1 = send_generateMsg(source_port, dest_port, 1, r_temp1.msg.seq_number + 1, ACK, '0');//seq���������
            sendto(serverSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("[�����]:���͵ڶ��λ��ֵ�ACK,У���:%d,seq:%d,ack:%d\n", temp1.checksum, temp1.seq_number, temp1.acknowledge_number);
            //������������͵Ĳ��ᶪ�������������ش���
            Mymsg temp2 = send_generateMsg(source_port, dest_port, 1, r_temp1.msg.seq_number + 1, FIN | ACK, '0');//seq���������
            sendto(serverSocket, msg2char(temp2), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("[�����]:���͵����λ��ֵ�FIN+ACK,У���:%d,seq:%d,ack:%d\n", temp2.checksum, temp2.seq_number, temp2.acknowledge_number);
            //���Ĵ�
            unsigned long long start_stamp = GetTickCount64();//��ʼ��ʱ
            while (1) {
                memset(buf1, 0, Msg_size);
                int bytesRead1 = recvfrom(serverSocket, (char*)buf1, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
                Mymsg_explain r_temp2 = Mymsg_explain(receive_char2msg(buf1));
                if (bytesRead > 0 && r_temp2.isACK() && check_sum(buf1, Msg_size) == 0) {
                    printf("[������]:�յ����Ĵλ��ֵ�ACK,seq:%d,ack:%d\n", r_temp2.msg.seq_number, r_temp2.msg.acknowledge_number);
                    cout << "-------------------�˳�---------------" << endl;
                    return;
                }
                else {
                    if (GetTickCount64() - start_stamp > TIMEOUT) {
                        //�ͻ����͵Ĳ��ɿ������ܲ�����
                        cout << "[������]:���Ĵλ��������δ�յ����Զ��˳�" << endl;
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

//��ȡ���ݴ���
int main() {


    // socket����
    WSADATA  wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "��ʼ��Socket DLLʧ��!" << endl;
        return 0;
    }
    else
        cout << "�ɹ���ʼ��Socket DLL,���绷�����سɹ�" << endl;

    // socket����
    //ʹ�����ݱ��׽���
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET)
    {
        cout << "���ݱ��׽��ִ���ʧ��" << endl;
        WSACleanup();
        return 0;
    }
    else {
        cout << "���ݱ��׽��ִ����ɹ�" << endl;
    }

    // ��ռ��<ip, port>
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int server_addrlen = sizeof(sockaddr_in);
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, server_addrlen) == SOCKET_ERROR) {
        cout << "�������󶨶˿ں�ipʧ��" << endl;
        WSACleanup();
    }
    else {
        cout << "�������󶨶˿ں�Ip�ɹ�" << endl;
    }
    //��������
    hand3Shakes();
    receive_file();
    hand4Shakes();

    closesocket(serverSocket);
    WSACleanup();
    return true;
}
