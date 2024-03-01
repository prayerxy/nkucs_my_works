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
SOCKADDR_IN clientAddr;//������ʵ��·����
int client_addrlen = sizeof(clientAddr);
int name_flag = 0;//�ļ����Ƿ������
//Դ�˿���Ŀ�Ķ˿�
unsigned short source_port = 8000;
unsigned short dest_port = 4001;//·�����˿�
char buffer[100000000];
int file_len;//�ļ����ֽڳ���
unsigned short rcvbase = 0;
//���մ���
struct SR_receive_ele {
    Mymsg m;
    bool receive = 0;
};
deque<SR_receive_ele>window(WINDOW_SIZE);
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
void recv_Msgs(char* buffer, int& len) {
    //����Ϊ����ģʽ
    int tv = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    len = 0;//�ֽ���
    rcvbase = 0;
    int seq_len = 0;//������
    //������� expectedsenumһ��������65535
    //����seq_len�Ǽ�¼���з���ĸ���
    while (1) {
        char curr_buf[Msg_size];
        int ret = recvfrom(serverSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&clientAddr, &client_addrlen);
        Mymsg msg_temp = receive_char2msg(curr_buf);
        Mymsg_explain r_temp = Mymsg_explain(msg_temp);
        if (ret > 0 && r_temp.hasnowseqnum(rcvbase) && check_sum(curr_buf, Msg_size) == 0) {
            if (name_flag == 1 && rcvbase == 0 && r_temp.isLAST() || r_temp.isACK() || r_temp.isSYN())
                continue;
            //printf("���ܴ���size:1,����seq:%d,check_sum=0\n", r_temp.msg.seq_number);
            //����ACK �Խ��ܵ��ķ���ȷ��
            Mymsg s_temp = send_generateMsg(source_port, dest_port, 0, r_temp.msg.seq_number, ACK, '0');//seq��������� ack=seq+1
            sendto(serverSocket, msg2char(s_temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("���յ����飬ȷ�ϲ�����ACK,ack:%d,У���:%d\n", s_temp.acknowledge_number, s_temp.checksum);
            //���Ȼ���
            SR_receive_ele ele;
            ele.m = msg_temp;
            ele.receive = 1;
            //��Ϊ��ʼ������������֤��window��size�����Կ�������ʹ��
            window[r_temp.msg.seq_number - rcvbase] = ele;
            //������򣬽������
            if (rcvbase == r_temp.msg.seq_number) {
                while (window.size() && window.front().receive) {
                    //��������
                    if (window.front().m.length < MSS) {
                        memcpy(buffer + seq_len * MSS, window.front().m.data, window.front().m.length);
                        len += window.front().m.length;
                    }
                    else {
                        memcpy(buffer + seq_len * MSS, window.front().m.data, MSS);
                        len += MSS;
                    }
                    //�ж��Ƿ�Ϊ���һ����
                    if (window.front().m.flag & LAST) {
                        if (name_flag) {
                            //�ļ�������ϣ�Ҫ��һ����Ϣ��֪
                            Sleep(20);
                            Mymsg s_temp = send_generateMsg(source_port, dest_port, 0, r_temp.msg.seq_number, ACK | LAST, '0');//seq��������� ack=seq+1
                            sendto(serverSocket, msg2char(s_temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
                            printf("������־\n");
                        }
                        printf("�������\n");
                        return;
                    }
                    rcvbase++;//���ں���ǰ�� ��һ���ڴ�����seq
                    seq_len++;//��expectedseqnum�����Ƿ�����������������
                    window.pop_front();
                    //����Ҫ��֤window�ĳߴ磬����push����Ϣ
                    SR_receive_ele nullmsg;
                    window.push_back(nullmsg);
                }
                //���մ����ƶ��������Ϣ
                printf("���н����󣬽��մ���size:%d,recvbase:%d\n", WINDOW_SIZE, rcvbase);
            }


        }
        else if (ret > 0 && r_temp.hasprevseqnum(rcvbase) && check_sum(curr_buf, Msg_size) == 0) {
            //ֻ�÷���ACK
            Mymsg s_temp = send_generateMsg(source_port, dest_port, 0, r_temp.msg.seq_number, ACK, '0');//seq��������� ack=seq+1
            sendto(serverSocket, msg2char(s_temp), Msg_size, 0, (struct sockaddr*)&clientAddr, client_addrlen);
            printf("  �ط�ACK,ack:%d,У���:%d\n", s_temp.acknowledge_number, s_temp.checksum);
        }
        else
            continue;
    }


}

void receive_file() {
    char name[30];
    int name_len;
    cout << "--------------��ʼ�����ļ���------------" << endl;
    recv_Msgs(name, name_len);
    cout << name << endl;
    string file_name(name);
    cout << "���յ��ļ���:" << file_name << endl;
    name_flag = 1;//�ļ������յ�
    //Ȼ������ļ�����
    cout << "------------��ʼ��������---------------" << endl;
    recv_Msgs(buffer, file_len);
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
