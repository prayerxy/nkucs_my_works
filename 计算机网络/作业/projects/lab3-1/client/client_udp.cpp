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
//����ش��������࣬����ʱ���и���
int resend_flag = 0;
//Դ�˿���Ŀ�Ķ˿�
unsigned short source_port = 8080;
unsigned short dest_port = 4001;
char buffer[100000000];//��������
char filename[30];
int file_len;//�ļ����ֽڳ���
int timeout = TIMEOUT;
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
//buffer�Ǵ洢Ҫ������ֽڵ�����
//lenΪ�ܹ����ֽ���
void sendMsg(char* buffer, int len) {
    //����
    //ʹ��ͣ�Ȼ��ƣ�����һ������ȴ����ն���Ӧ

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    if (len > MSS) {
        int groupN = MSS * len / MSS < len ? len / MSS + 1 : len / MSS;
        //��ʼ����
        int seq = 0;
        //���ڵ����䣬��������ֻ��ֵseq�ֶΣ��ͻ���ֻ��ֵack�ֶΣ���Ӧ������
        while (1) {
            //�������0
            if (seq >= groupN)
                break;
            Mymsg temp0 = send_generateMsg(source_port, dest_port, seq, 0, 0, '0');//seq���������
            //���ݸ�ֵ
            if (seq == groupN - 1) {
                temp0.length = len - (groupN - 1) * MSS;
                temp0.flag |= LAST;
                memcpy(temp0.data, buffer + seq * MSS, len - (groupN - 1) * MSS);
            }
            else
                memcpy(temp0.data, buffer + seq * MSS, MSS);
            temp0.checksum = 0;
            char* p = msg2char(temp0);
            //����checksum��ֵ
            temp0.checksum = check_sum(p, Msg_size);
            sendto(clientSocket, msg2char(temp0), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            printf("���ͷ���seq:%d,����״̬0��У���:%d\n", seq, temp0.checksum);
            unsigned long long start_stamp = GetTickCount64();//��ʼ��ʱ
            //����ȴ�ACK0״̬
            while (1) {
                char curr_buf[Msg_size];
                int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
                Mymsg_explain r_temp0 = Mymsg_explain(receive_char2msg(curr_buf));
                //��ȷ��׼��������һ��״̬
                if (ret > 0 && r_temp0.isACK0() && check_sum(curr_buf, Msg_size) == 0) { printf("���ܵ�ȷ��ACK0,ack:%d\n", r_temp0.msg.acknowledge_number); break; }
                else {}//ʲôҲ�����ȴ���ʱ
                //��ʱ
                if (GetTickCount64() - start_stamp > timeout) {
                    //�ش�����
                    if (resend_flag > MISSCOUNT) {
                        timeout *= 1.5;
                        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                        resend_flag = 0;
                        cout << "timeout�޸�Ϊ" << timeout << "ms\n";
                    }
                    sendto(clientSocket, msg2char(temp0), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                    //���¼�ʱ
                    start_stamp = GetTickCount64();
                    cout << "��ʱ�ش�����seq:" << temp0.seq_number << endl;
                    resend_flag++;
                }

            }
            resend_flag = 0;
            //�������1
            seq++;//��һ������
            if (seq >= groupN)
                break;
            Mymsg temp1 = send_generateMsg(source_port, dest_port, seq, 0, 0, '1');//seq���������
            //���ݸ�ֵ
            if (seq == groupN - 1) {
                temp1.length = len - (groupN - 1) * MSS;
                temp1.flag |= LAST;
                memcpy(temp1.data, buffer + seq * MSS, len - (groupN - 1) * MSS);
            }
            else
                memcpy(temp1.data, buffer + seq * MSS, MSS);
            temp1.checksum = 0;
            p = msg2char(temp1);
            //����checksum��ֵ
            temp1.checksum = check_sum(p, Msg_size);
            sendto(clientSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
            printf("���ͷ���seq:%d,У���:%d\n", seq, temp1.checksum);
            //��ʼ��ʱ
            start_stamp = GetTickCount64();
            //����ȴ�ACK1״̬
            while (1) {
                char curr_buf1[Msg_size];
                int ret = recvfrom(clientSocket, curr_buf1, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
                Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(curr_buf1));

                //��ȷ��׼��������һ��״̬
                if (ret > 0 && r_temp1.isACK1() && check_sum(curr_buf1, Msg_size) == 0) { printf("���ܵ�ȷ��ACK1,ack:%d\n", r_temp1.msg.acknowledge_number); break; }
                else {}//ʲôҲ�����ȴ���ʱ
                if (GetTickCount64() - start_stamp > timeout) {
                    if (resend_flag > MISSCOUNT) {
                        timeout *= 1.5;
                        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                        resend_flag = 0;
                        cout << "timeout�޸�Ϊ" << timeout << "ms\n";
                    }
                    //�ش�����
                    sendto(clientSocket, msg2char(temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                    //���¼�ʱ
                    start_stamp = GetTickCount64();
                    cout << "��ʱ�ش�����seq:" << temp1.seq_number << endl;
                    resend_flag++;
                }

            }
            seq++;//��һ��
            resend_flag = 0;
            //���ĸ�״̬��ϣ���ͷ��ʼ
            continue;
        }
        cout << "�ɹ�����������" << endl;
    }
    //��һ������
    else {

        Mymsg temp = send_generateMsg(source_port, dest_port, 0, 0, LAST, '0');//seq���������
        memcpy(temp.data, buffer, len);
        temp.length = len;
        temp.checksum = 0;
        char* p = msg2char(temp);
        //����checksum��ֵ
        temp.checksum = check_sum(p, Msg_size);
        sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        printf("���ͷ���seq:%d,У���:%d\n", 0, temp.checksum);
        unsigned long long start_stamp = GetTickCount64();//��ʼ��ʱ
        while (1) {
            char curr_buf[Msg_size];
            int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
            Mymsg_explain r_temp = Mymsg_explain(receive_char2msg(curr_buf));
            //ʲôҲ�����ȴ���ʱ
            if (ret <= 0 || ret > 0 && r_temp.isACK1()) {}
            //��ȷ��׼��������һ��״̬
            if (ret > 0 && r_temp.isACK0() && check_sum(curr_buf, Msg_size) == 0) { break; }
            if (GetTickCount64() - start_stamp > timeout) {
                if (resend_flag > MISSCOUNT) {
                    timeout *= 1.5;
                    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
                    resend_flag = 0;
                    cout << "timeout�޸�Ϊ" << timeout << "ms\n";
                }
                //�ش�����
                sendto(clientSocket, msg2char(temp), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
                //���¼�ʱ
                start_stamp = GetTickCount64();
                cout << "��ʱ�ش�����seq:" << temp.seq_number << endl;
                resend_flag++;
            }
        }
        resend_flag = 0;
        cout << "�ɹ�����������" << endl;
    }
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
    sendMsg((char*)filename, 30);
    cout << "-------------��ʼ�����ļ�����--------------------" << endl;
    unsigned long long start_stamp1 = GetTickCount64();//��ʼ��ʱ
    sendMsg(buffer, file_len);
    unsigned long long end_stamp1 = GetTickCount64();//��ʼ��ʱ
    cout << "-------------------" << endl;
    cout << "�����ļ���С:" << file_len << "bytes" << endl;
    cout << "�ļ�����ʱ��:" << end_stamp1 - start_stamp1 << "ms\n";
    double kbps = (file_len * 8.0) / (end_stamp1 - start_stamp1);
    cout << "������Ϊ:" << kbps << "kbps" << endl;
    cout << "--------------------" << endl;

}

void hand4Shakes() {
    while (1) {
        Mymsg s_temp1 = send_generateMsg(source_port, dest_port, 1, 1, FIN | ACK, '0');//seq��������� ack=seq+1
        sendto(clientSocket, msg2char(s_temp1), Msg_size, 0, (struct sockaddr*)&serverAddr, server_addrlen);
        printf("[�ͻ���]:���͵�һ�λ���FIN+ACK,У���:%d,seq:%d,ack:%d\n", s_temp1.checksum, s_temp1.seq_number, s_temp1.acknowledge_number);
        char curr_buf[Msg_size];
        int ret = recvfrom(clientSocket, curr_buf, Msg_size, 0, (struct sockaddr*)&serverAddr, &server_addrlen);
        Mymsg_explain r_temp1 = Mymsg_explain(receive_char2msg(curr_buf));
        if (ret > 0 && check_sum(curr_buf, Msg_size) == 0 && r_temp1.isACK()) {
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
    cout << "������Ҫ���͵��ļ���:" << endl;
    cin >> filename;
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
    //�����ļ�
    send_file();
    hand4Shakes();
    closesocket(clientSocket);
    WSACleanup();
    return true;
}
