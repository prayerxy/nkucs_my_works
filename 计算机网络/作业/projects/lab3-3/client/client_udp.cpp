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
//����ش��������࣬����ʱ���и���
int resend_flag = 0;
//Դ�˿���Ŀ�Ķ˿�
unsigned short source_port = 8080;
unsigned short dest_port = 4001;
char buffer[100000000];//��������
char filename[30];
int file_len;//�ļ����ֽڳ���
int timeout = TIMEOUT;
int changetime_flag = 0;
//��Է�����м�ʱ�ı�־
int time_flag[8000] = { 0 };
//��Է�����м�ʱ��ʱ��
//Ϊ�˱�����ֶ��߳����⣬���Բ�ȡ�˴�ʩ
unsigned long long start_stamp[8000] = { 0 };
int finish_flag = 0;//��ɴ���ı�־

unsigned long  seq_len = 0;
unsigned short base = 0;
unsigned short nextseqnum = 0;//��seq_numһ�£����65535���������Զ�תΪ0


//���ʹ���  ��ȡSRЭ��
deque<Mymsg>window;


void hand3Shakes() {
    char buffer[Msg_size];
    int timeout = TIMEOUT * 4;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    while (1) {
        //SYN����
        Mymsg temp = send_generateMsg(source_port, dest_port, 0, 0, SYN, '0');
        sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        cout << "[�ͻ���]:���͵�һ������SYN����" << endl;
        int bytesRead = recvfrom(clientSocket, buffer, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain cur = Mymsg_explain(receive_char2msg(buffer));
        if (bytesRead > 0 && check_sum(buffer, Msg_size) == 0 && cur.isSYN() && cur.isACK()) {
            cout << "[�ͻ���]:�ڶ������ֽ���SYN+ACK" << endl;
            //����������,���ܶ�ʧ
            Mymsg temp1 = send_generateMsg(source_port, dest_port, cur.msg.acknowledge_number, cur.msg.seq_number + 1, ACK, '0');
            sendto(clientSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            cout << "[�ͻ���]:���͵���������ACK" << endl;
            break;
        }
        else
            continue;

    }
    cout << "[�ͻ���]:�����������ӽ���" << endl;
    cout << "[�ͻ���]:Ŀ�Ķ˿�Ϊ:" << dest_port << endl;
}


// ģ�ⷢ�����ݰ��ĺ���


// ģ����� ACK �ĺ���
void receiveACK(int groupN) {
    while (!finish_flag) {
        char curr_buf[Msg_size];
        int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain r_temp = Mymsg_explain(receive_char2msg(curr_buf));
        unsigned short r_temp_ack = r_temp.msg.acknowledge_number;
        if (ret > 0 && r_temp.isACK() && check_sum(curr_buf, Msg_size) == 0 && r_temp_ack >= base && r_temp_ack < base + WINDOW_SIZE - 1) {
            resend_flag = 0;
            printf("  ���յ�ȷ��ACK��ack:%d \n", r_temp.msg.acknowledge_number);
            //ֹͣ�÷���ļ�ʱ
            time_flag[r_temp_ack] = Endtime;
            //�������С���飬����ǰ��
            if (base == r_temp.msg.acknowledge_number) {
                //������ܻᵯ����� �����ϵĿ�ʼ  ���Ͷ��Ǽ���backβ��  ������front
                while (window.size() && time_flag[window.front().seq_number] == Endtime) {
                    window.pop_front();//���ϵ�ɾ�� ���µ�back����
                    base++;
                }

                printf("  ���ܴ�����С��ack�󣬷��ʹ���size:%d,base:%d,nextseqnum:%d\n", nextseqnum - base, base, nextseqnum);
            }

            if (base == groupN || (r_temp.msg.flag & LAST) && r_temp.msg.acknowledge_number == groupN - 1) {
                finish_flag = 1;
                printf("  �ļ��������\n");
                return;//�������
            }

        }
    }
    return;
}

void timeoutResend(int groupN, int len) {
    while (!finish_flag) {
        //�жϷ��ʹ��ڵ�Ԫ���Ƿ���ڳ�ʱ
        //������ʵ����Ҫ�����ƣ��������ack�ˣ���ôǰ��/���ò���ʱ ͬʱӰ�쳬ʱ�ط���û����
        for (int i = base; i < nextseqnum; i++) {
            if (time_flag[i] == STARTtime && GetTickCount64() - start_stamp[i] > timeout) {
                //����ʱ��
                start_stamp[i] = GetTickCount64();
                //�ط�
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
                printf("��ʱ���·���seq:%d��У���:%d\n", temp.seq_number, temp.checksum);
                resend_flag++;
                //��������ط���û���յ�ack������MISSCOUNT������Ϊ����
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


//����
void send_Msgs(char* buffer, int len) {
    //����
    int timeout = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    finish_flag = 0;
    int groupN = MSS * (len / MSS) < len ? len / MSS + 1 : len / MSS;
    seq_len = 0;
    base = nextseqnum = 0;
    //��ʼ����
    std::thread ackThread(receiveACK, groupN);
    std::thread resendThread(timeoutResend, groupN, len);
    while (!finish_flag) {
        //ֻ�ܷ���0-groupN-1�İ� ���seq_lenΪgroupN
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
            //����checksum��ֵ
            temp.checksum = check_sum(p, Msg_size);
            sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            //��������ͻ�����
            //�����·��飬������ʱ
            time_flag[nextseqnum] = STARTtime;//��ʼ��ʱ
            start_stamp[nextseqnum] = GetTickCount64();
            window.push_back(temp);

            printf("����seq:%d��У���:%d\n", temp.seq_number, temp.checksum);
            //ǰ���ƶ����Լ���λ���ƶ�
            nextseqnum++;//�������״̬����65535 �����ص�0
            seq_len++;
            printf("���ʹ���size:%d,base:%d,nextseqnum:%d\n", nextseqnum - base, base, nextseqnum);

        }


    }

    ackThread.join();
    resendThread.join();
    return;

}

//����ifstream��ȡ�ļ���Ϊchar����
//�ٵ���sendMsg��������
void send_file() {

    ifstream fin(filename, ifstream::in | ios::binary);
    fin.seekg(0, fin.end);  //���ļ���ָ�붨λ������ĩβ
    file_len = fin.tellg();
    fin.seekg(0, fin.beg);  //���ļ���ָ�����¶�λ�����Ŀ�ʼ
    fin.read(buffer, file_len);
    fin.close();
    //����rdt3.0���� ͣ�� ������һ���Żᴫ��һ��
    //�ȴ����ļ���
    cout << "----------------��ʼ�����ļ���-------------------" << endl;
    send_Msgs((char*)filename, 30);
    cout << "-------------��ʼ�����ļ�����--------------------" << endl;
    for (int i = 0; i < 8000; i++) {
        time_flag[i] = 0;
        start_stamp[i] = 0;
    }
    unsigned long long start_stamp1 = GetTickCount64();//��ʼ��ʱ
    send_Msgs(buffer, file_len);
    unsigned long long end_stamp1 = GetTickCount64();//��ʼ��ʱ
    cout << "-------------------" << endl;
    cout << "�����ļ���С:" << file_len << "bytes" << endl;
    cout << "�ļ�����ʱ��:" << end_stamp1 - start_stamp1 << "ms\n";
    double kbps = (file_len * 8.0) / (end_stamp1 - start_stamp1);
    cout << "������Ϊ:" << kbps << "kbps" << endl;
    cout << "--------------------" << endl;


}

void hand4Shakes() {
    int timeout = TIMEOUT;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    char curr_buf[Msg_size];
    recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);

    while (1) {

        memset(curr_buf, 0, Msg_size);
        Mymsg s_temp1 = send_generateMsg(source_port, dest_port, 1, 1, FIN | ACK, '0');//seq��������� ack=seq+1
        sendto(clientSocket, msg2char(s_temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        printf("[�ͻ���]:���͵�һ�λ���FIN+ACK,У���:%d,seq:%d,ack:%d\n", s_temp1.checksum, s_temp1.seq_number, s_temp1.acknowledge_number);

        int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(curr_buf));
        if (ret > 0 && check_sum(curr_buf, Msg_size) == 0 && r_temp1.isACK() && !r_temp1.isLAST()) {
            printf("[�ͻ���]:�յ��ڶ��λ���ACK,seq:%d,ack:%d\n", r_temp1.msg.seq_number, r_temp1.msg.acknowledge_number);
            memset(curr_buf, 0, Msg_size);
            int ret2 = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
            Mymsg_explain r_temp2 = Mymsg_explain(receive_char2msg(curr_buf));
            if (ret2 > 0 && check_sum(curr_buf, Msg_size) == 0 && r_temp2.isFIN() && r_temp2.isACK()) {
                printf("[�ͻ���]:�յ������λ���FIN+ACK,seq:%d,ack:%d\n", r_temp2.msg.seq_number, r_temp2.msg.acknowledge_number);
                Mymsg s_temp2 = send_generateMsg(source_port, dest_port, r_temp2.msg.acknowledge_number, r_temp2.msg.seq_number + 1, ACK, '0');//seq��������� ack=seq+1
                sendto(clientSocket, msg2char(s_temp2), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                printf("[�ͻ���]:���͵��Ĵδλ���ACK,У���:%d,seq:%d,ack:%d\n", s_temp2.checksum, s_temp2.seq_number, s_temp2.acknowledge_number);
                cout << "-----------------�˳�-------------" << endl;
                Sleep(2 * TIMEOUT);
                return;
            }
        }
        else
            continue;//��ͷ��ʼ
    }

}
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
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET)
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
    serverAddr.sin_port = htons(dest_port);//·����
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addrlen = sizeof(serverAddr);
    // ��ռ��<ip, port>
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(8080);
    clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int client_addrlen = sizeof(sockaddr_in);
    if (bind(clientSocket, (SOCKADDR*)&clientAddr, client_addrlen) == SOCKET_ERROR) {
        cout << "�ͻ��󶨶˿ں�ipʧ��" << endl;
        WSACleanup();
    }
    else {
        cout << "�ͻ��󶨶˿ں�Ip�ɹ�" << endl;
    }
    //��������
    hand3Shakes();
    cout << "������Ҫ���͵��ļ���:" << endl;
    cin >> filename;
    //�����ļ�
    send_file();
    hand4Shakes();
    closesocket(clientSocket);
    WSACleanup();
    return true;
}
