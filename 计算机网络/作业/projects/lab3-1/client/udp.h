#include<iostream>
//��־λ
#define ACK  0B00000001
#define SYN  0B00000010
#define FIN  0B00000100
#define LAST 0B00001000
//һ�����ݰ��ش�12�Σ�����TIMEOUTʱ��
#define MISSCOUNT 12
#define TIMEOUT 500
//���ݶ����Ĵ�С
#define MSS 8016
struct Mymsg {
    unsigned short source_port;//Դ�˿�
    unsigned short dest_port;//Ŀ�Ķ˿�
    unsigned short seq_number;//�������ݰ������ 16λ
    unsigned short acknowledge_number;//ȷ�Ϻ��ֶ� 16λ
    char flag;//��־λ
    char ack_id;//rdt3.0 ack��ack0��ack1
    unsigned short length;//���ݰ����� 16λ
    unsigned short checksum;//У���16λ
    char data[MSS];//���ݴ�С,�MSS
};
//4110�ֽ� 
#define Msg_size sizeof(Mymsg)
//�����buf��sento/recvfrom�Ĳ���buf
//������
unsigned short check_sum(char* buf, int length) {
    // unsigned longռ4���ֽ�
    unsigned long sum = 0;//32λ 
    for (int i = 0; i < length; i += 2) {
        //��ȡ2��char תΪ16λ
        unsigned short temp;
        if (i == length - 1) {
            memset(&temp, 0, 2);
            //ע�������temp�ĸ�8λ����8λ��0
            memcpy((char*)&temp + 1, buf + i, 1);
        }
        else
            memcpy(&temp, buf + i, 2);
        sum += temp;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF);
}
//���յ�char*תΪmessage
Mymsg receive_char2msg(char* receive_buf) {
    Mymsg temp;
    memcpy(&temp, receive_buf, sizeof(Mymsg));
    return temp;
}
//��Ҫ�ѷ�װ��messageת��Ϊcharȥ����
char* msg2char(Mymsg message) {
    char temp[Msg_size];
    memcpy(temp, &message, sizeof(Mymsg));//ֱ�ӽ��ֽ�������
    return temp;
}

Mymsg send_generateMsg(unsigned short s_p, unsigned short d_p, unsigned short seq,
    unsigned short ack, char flag, char ack_id, unsigned short length = MSS, unsigned short ck_sum = 0) {
    Mymsg temp;
    temp.source_port = s_p;
    temp.dest_port = d_p;
    temp.seq_number = seq;
    temp.acknowledge_number = ack;
    temp.flag = flag;
    temp.ack_id = ack_id;
    temp.length = MSS;
    temp.checksum = ck_sum;
    char* p = msg2char(temp);
    //����checksum��ֵ ���ݶ�Ϊ�յ�ʱ�����ֱ������checksum�ֶ�
    temp.checksum = check_sum(p, Msg_size);
    return temp;
}

class Mymsg_explain {
public:
    Mymsg msg;
    Mymsg_explain(Mymsg s) :msg(s) {}
    void Mymsg_copy(Mymsg s) { memcpy(&msg, &s, Msg_size); }
    bool isACK0() {
        if (msg.ack_id == '0' && msg.flag & ACK) {
            return 1;
        }
        return 0;
    }
    bool isACK1() {
        if (msg.ack_id == '1' && msg.flag & ACK) {
            return 1;
        }
        return 0;
    }
    bool isACK() {
        if (msg.flag & ACK)
            return 1;
        return 0;
    }
    bool isSYN() {
        if (msg.flag & SYN)
            return 1;
        return 0;
    }
    bool isFIN() {
        if (msg.flag & FIN)
            return 1;
        return 0;
    }
    bool isSeq0() {
        if (msg.ack_id == '0')
            return 1;
        return 0;
    }
    bool isSeq1() {
        if (msg.ack_id == '1')
            return 1;
        return 0;
    }
    bool isLAST() {
        if (msg.flag & LAST)
            return 1;
        return 0;
    }
};
