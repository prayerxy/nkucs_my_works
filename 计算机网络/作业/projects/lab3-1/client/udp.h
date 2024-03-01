#include<iostream>
//标志位
#define ACK  0B00000001
#define SYN  0B00000010
#define FIN  0B00000100
#define LAST 0B00001000
//一个数据包重传12次，增大TIMEOUT时间
#define MISSCOUNT 12
#define TIMEOUT 500
//数据段最大的大小
#define MSS 8016
struct Mymsg {
    unsigned short source_port;//源端口
    unsigned short dest_port;//目的端口
    unsigned short seq_number;//发送数据包的序号 16位
    unsigned short acknowledge_number;//确认号字段 16位
    char flag;//标志位
    char ack_id;//rdt3.0 ack有ack0与ack1
    unsigned short length;//数据包长度 16位
    unsigned short checksum;//校验和16位
    char data[MSS];//数据大小,最长MSS
};
//4110字节 
#define Msg_size sizeof(Mymsg)
//这里的buf是sento/recvfrom的参数buf
//差错检验
unsigned short check_sum(char* buf, int length) {
    // unsigned long占4个字节
    unsigned long sum = 0;//32位 
    for (int i = 0; i < length; i += 2) {
        //获取2个char 转为16位
        unsigned short temp;
        if (i == length - 1) {
            memset(&temp, 0, 2);
            //注意是填充temp的高8位，低8位是0
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
//接收的char*转为message
Mymsg receive_char2msg(char* receive_buf) {
    Mymsg temp;
    memcpy(&temp, receive_buf, sizeof(Mymsg));
    return temp;
}
//需要把封装的message转化为char去发送
char* msg2char(Mymsg message) {
    char temp[Msg_size];
    memcpy(temp, &message, sizeof(Mymsg));//直接将字节数拷贝
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
    //产生checksum的值 数据段为空的时候可以直接生成checksum字段
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
