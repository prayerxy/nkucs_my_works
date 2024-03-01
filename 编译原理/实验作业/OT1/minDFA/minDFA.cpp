#include<iostream>
#include<map>
#include<set>
#include<vector>
#include<stack>
#include <unordered_map>
using namespace std;

string letters;//��ĸ��
string expression, infix, suffix;//��׺����׺�ַ���
string d_state;//״̬����
string min_dstate;//��С��dfa��״̬����
map<char, int>precedence;//���ȼ�
int start = 0;//nfa״̬������
char d_start = '@';//dfa״̬������
char null_action = '~';//������'~'���
char dead_state = '@';
int dead_flag = 0;//�Ƿ�����״̬��־
struct edge {
    char weight;//letter��������ĸ
    int next;  //��һ��nfa״̬
};
struct edge2 {
    char weight;//letter��������ĸ
    char next;//��һ��dfa״̬
};
map<int, int>left_right;
map<int, vector<edge>>nfa;//nfa ��㣬��������ı�
map<char, vector<edge2>>dfa;//dfa 
map<char, vector<edge2>>min_dfa;//��С����dfa
char D_S;
string D_F;//��̬����̬
string MIN_DS;//��С���ĳ�̬
string MIN_DF;//��С������̬
map<set<int>, char>class2state;//nfa�Ӽ���dfa��״̬��ӳ��
#define nfaIT  map<int, vector<edge>>::iterator
struct CompareMap {
    bool operator()(const std::map<char, edge2>& lhs, const std::map<char, edge2>& rhs) const {
        return lhs.begin()->first < rhs.begin()->first;
    }
};

void setPrecedence() {
    precedence['*'] = 3;
    precedence['.'] = 2;
    precedence['|'] = 1;
}
//�������ӷ�
void expr2infix() {
    for (int i = 0; i < expression.length(); i++) {
        char cur = expression[i];
        char next = expression[i + 1];
        //Ĭ�����ӷ�ʡ�ԣ���������'.'
        if ((cur != '|' && cur != '.' && cur != '(') && (next != ')' && next != '|' && next != '*' && next != '.' && next != '\0')) {
            infix = infix + cur + '.';
        }
        else {
            infix += cur;
        }
    }
}
//��׺תΪ��׺
//��ջ�ǿգ��ж�ջ�������������ò��������ò�������ջ��
//��ǰ���������ȼ�����ջ������������ôҪ��ջ����������ջ��
//ֱ��ջ�ջ�ջ�����������ȼ����ڸò��������ò���������ջ��
void infix2suffix() {
    stack<char>cstack;
    for (int i = 0; i < infix.size(); i++) {
        char cur = infix[i];//��ǰ�ַ�
        if (cur == '(')
            cstack.push(cur);
        else if (cur == ')') {//��Ҫ����������ȫ�����
            while (cstack.top() != '(') {
                char temp = cstack.top();
                suffix += temp;
                cstack.pop();
            }
            cstack.pop();//ɾ��������
        }
        else if (cur == '|' || cur == '*' || cur == '.') {
            while (!cstack.empty()) {
                char temp = cstack.top();
                //cur���ȼ��ͣ���Ҫ�ѹ�ȥ��temp��ӡ
                if (precedence[temp] >= precedence[cur]) {
                    suffix += temp;
                    cstack.pop();
                }
                else {//��ǰ�ַ����ȼ���
                    cstack.push(cur);
                    break;
                }
            }
            if (cstack.empty())
                cstack.push(cur);
        }
        else {
            suffix += cur;
        }
    }
    while (!cstack.empty()) {
        char temp = cstack.top();
        suffix += temp;
        cstack.pop();
    }
}
void printNFA() {
    string tmp;
    for (nfaIT it = nfa.begin(); it != nfa.end(); it++) {
        for (int i = 0; i < it->second.size(); i++) {

            string tmp1 = "[";
            string temp2 = (string)"��" + it->second[i].weight + "��>";
            cout << tmp1 << it->first << temp2 << it->second[i].next << "]" << endl;
        }
    }
}
int suffix2nfa() {
    stack<int>cstack;//����״̬
    for (int i = 0; i < suffix.size(); i++) {
        char cur = suffix[i];
        if (cur == '*') {
            int l_temp = cstack.top();
            int r_temp = left_right[l_temp];
            left_right.erase(l_temp);
            cstack.pop();
            start++;//��Ϊ��ʼ״̬
            edge e;
            e.weight = null_action;
            e.next = l_temp;
            nfa[start].push_back(e);
            e.next = l_temp;
            nfa[r_temp].push_back(e);
            e.next = start + 1;
            nfa[start].push_back(e);
            nfa[r_temp].push_back(e);
            start++;
            left_right[(start - 1)] = start;
            cstack.push(start - 1);//�������״̬��ջ

        }
        else if (cur == '.') {
            int l_temp2 = cstack.top();
            int r_temp2 = left_right[l_temp2];
            cstack.pop();
            int l_temp1 = cstack.top();
            int r_temp1 = left_right[l_temp1];
            left_right.erase(l_temp1);
            left_right.erase(l_temp2);
            edge e;
            e.weight = null_action;
            e.next = l_temp2;
            nfa[r_temp1].push_back(e);
            left_right[l_temp1] = r_temp2;
            //����Ҫ��pop
        }
        else if (cur == '|') {
            int l_temp2 = cstack.top();
            int r_temp2 = left_right[l_temp2];
            cstack.pop();
            int l_temp1 = cstack.top();
            int r_temp1 = left_right[l_temp1];
            cstack.pop();
            left_right.erase(l_temp1);
            left_right.erase(l_temp2);
            start++;//��̬
            edge e;
            e.weight = null_action;
            e.next = l_temp1;
            nfa[start].push_back(e);
            e.next = l_temp2;
            nfa[start].push_back(e);
            start++;
            e.next = start;
            nfa[r_temp1].push_back(e);
            nfa[r_temp2].push_back(e);
            left_right[(start - 1)] = start;
            cstack.push((start - 1));

        }
        else {
            start++;//��̬��1��ʼ
            edge e;
            e.weight = cur;
            e.next = start + 1;
            nfa[start].push_back(e);
            cstack.push(start);
            left_right[start] = e.next;
            start++;
        }
    }
    return cstack.top();//��̬
}
//���㼯��subClass�ıհ�'~'����
//��set����ȥ��
set<int> _closure(set<int>subClass) {
    stack<int>cstack;
    set<int>temp(subClass);
    for (auto i = subClass.begin(); i != subClass.end(); i++)
        cstack.push(*i);
    while (!cstack.empty()) {
        int cur = cstack.top();
        cstack.pop();
        for (auto i = 0; i < nfa[cur].size(); i++) {
            if (nfa[cur][i].weight == null_action) {
                if (temp.find(nfa[cur][i].next) == temp.end()) {//˵��temp��������û��
                    cstack.push(nfa[cur][i].next);
                    temp.insert(nfa[cur][i].next);
                }
            }
        }
    }
    return temp;
}
//a���������
set<int> transition(set<int>subClass, char a) {
    set<int>temp;
    //��nfa��״̬ת��
    for (auto i = subClass.begin(); i != subClass.end(); i++) {
        for (auto j = 0; j < nfa[*i].size(); j++) {
            if (nfa[*i][j].weight == a) {
                if (temp.find(nfa[*i][j].next) == temp.end()) {//˵��temp��������û��
                    temp.insert(nfa[*i][j].next);
                }
            }
        }
    }
    return temp;
}
//nfa->dfa
//������S�����̬��F������̬ ��nfa��
void nfa2dfa(int S, int F) {
    set<int>temp;
    temp.insert(S);
    set<int>set_S = _closure(temp);//��Ϊ��̬
    set<set<int>>dstates2; //��һ�����ص�����
    vector<set<int>>dstates;//dfa״̬�� int�Ǳ��λ
    int flag = 0;//������״̬
    dstates.push_back(set_S);
    dstates2.insert(set_S);
    d_start++;//���ÿ�ʼ״̬
    D_S = d_start;
    d_state = d_state + (char)d_start;
    class2state[set_S] = d_start;
    while (flag < dstates.size()) {
        flag++;//��ǵ�����
        set<int>cur(dstates[flag - 1]);//��ǰ״̬����
        for (int i = 0; i < letters.size(); i++) {//����ÿ�����ţ�letters����ĸ��
            set<int>U = _closure(transition(cur, letters[i]));
            if (dstates2.find(U) == dstates2.end() && !U.empty()) {//����Ϊ�գ��ұ���֮ǰû��
                dstates.push_back(U);
                dstates2.insert(U);
                d_start++;//��״̬
                d_state = d_state + (char)d_start;
                class2state[U] = d_start;
            }
            if (U.empty())
                continue;
            edge2 e;
            e.weight = letters[i];
            e.next = class2state[U];
            dfa[class2state[cur]].push_back(e);//���dfa��ת��
        }
    }
    //��������̬����
    for (int i = 0; i < dstates.size(); i++) {
        if (dstates[i].find(F) != dstates[i].end()) {
            D_F.push_back(class2state[dstates[i]]);//������̬
        }
    }
}
void printDFA() {
    for (auto it = dfa.begin(); it != dfa.end(); it++) {
        for (int i = 0; i < it->second.size(); i++) {
            string tmp1 = "[";
            string temp2 = (string)"��" + it->second[i].weight + "��>";
            cout << tmp1 << it->first << temp2 << it->second[i].next << "]" << endl;
        }
    }
}
//��С��dfa
void addDeadState() {
    for (int i = 0; i < d_state.size(); i++) {
        //������״̬
        if (dfa[d_state[i]].size() < letters.size()) {
            for (int j = 0; j < letters.size(); j++) {
                int cur_flag = 0;//�Ƿ���ڸ÷��Ŷ�Ӧ��
                for (int k = 0; k < dfa[d_state[i]].size(); k++) {
                    if (dfa[d_state[i]][k].weight == letters[j]) {
                        cur_flag = 1;
                        break;
                    }
                }
                //˵��û�и÷��Ŷ�Ӧ���룬������״̬��Ӧת��
                if (cur_flag == 0) {
                    dead_flag = 1;
                    edge2 e;
                    e.next = dead_state;//��״̬
                    e.weight = letters[j];
                    dfa[d_state[i]].push_back(e);
                }
                else {}//���ڵ�ǰ���Ų��üӱ�
            }
        }
        else {}//˵���ߵ��������ڷ��Ÿ�����û����״̬

    }
    if (dead_flag) {
        for (int i = 0; i < letters.size(); i++) {
            edge2 e;
            e.next = dead_state;//��״̬
            e.weight = letters[i];
            dfa[dead_state].push_back(e);
        }

    }
}
void MIN_dfa() {
    set<char>s1;
    set<char>s2;
    map<char, set<char>>count;//��¼��dfaÿ��״̬�뼯�ϵĶ�Ӧ��ϵ�����ϸ���
    //map<char, set<char>>cur_count;
    if (dead_flag)
        d_state += '@';
    for (int i = 0; i < D_F.size(); i++)
        s2.insert(D_F[i]);
    for (int i = 0; i < d_state.size(); i++) {
        if (s2.find(d_state[i]) == s2.end()) {
            s1.insert(d_state[i]);
        }
    }
    for (auto i = s1.begin(); i != s1.end(); i++) {
        count[*i] = s1;
    }
    for (auto i = s2.begin(); i != s2.end(); i++) {
        count[*i] = s2;
    }
    set<set<char>>M;
    M.insert(s1); M.insert(s2);
    unordered_map<char, map<char, set<char>>>transi_count;//��һ��char��Ӧ״̬ �ڶ�����Ӧ����������ӳ��
    //s1�ǲ��ɽ���,s2�ɽ���
    set<set<char>>Mnew;
    int size_flag;
    while (M != Mnew) {
        //count��ӳ��᲻�ϱ�����
        if (Mnew.size() != 0)
            M = Mnew;
        Mnew.clear();//�µ�����
        for (auto i = M.begin(); i != M.end(); i++) {
            //��ÿ����ִ�����²���
            transi_count.clear();
            for (int k = 0; k < letters.size(); k++) {
                for (auto s = (*i).begin(); s != (*i).end(); s++) {
                    transi_count[*s][dfa[*s][k].weight] = count[dfa[*s][k].next];

                }
            }
            /*for (auto xy = transi_count.begin(); xy != transi_count.end(); xy++) {
                cout << xy->first << " ";
                for (auto fk = xy->second.begin(); fk != xy->second.end(); fk++)
                    cout << fk->first << " ";
                cout << endl;
            }*/
            for (auto w = transi_count.begin(); w != transi_count.end(); w++) {
                char flag = '0';//�ж��Ƿ�����ҵ�һ���״̬��־
                for (auto e = transi_count.begin(); e != w; e++) {
                    //������������
                    for (int k = 0; k < letters.size(); k++) {
                        if (w->second[letters[k]] != e->second[letters[k]]) {
                            break;
                        }
                        if (k == letters.size() - 1)
                            flag = e->first;
                    }
                    //�����ǰ�ҵ���һ�������˳�
                    if (flag != '0')
                        break;
                }
                if (flag != '0') {
                    count[flag].insert(w->first);
                    count[w->first] = count[flag];//count ӳ��һ��״̬����һ������
                    for (auto member = count[flag].begin(); member != count[flag].end(); member++)
                        count[*member] = count[flag];
                }
                else {
                    set<char>temp;
                    temp.insert(w->first);
                    count[w->first] = temp;

                }
            }
            if (&(*i) != &(*M.rbegin()))continue;
            for (auto iter = count.begin(); iter != count.end(); iter++) {
                /*for (auto ks = iter->second.begin(); ks != iter->second.end(); ks++)
                    cout << *ks;*/
                    //cout << endl;
                if (Mnew.find(iter->second) == Mnew.end()) {
                    Mnew.insert(iter->second);//�µ���
                    /*for (auto sk = iter->second.begin(); sk != iter->second.end(); sk++)
                        cout << *sk << " ";
                    cout << endl;*/
                }

            }
        }
    }
    //����M��������ļ��ϣ���Ҫ�����״̬'@'
    map<char, char>handle;//��һ��Ϊdfa������״̬���ڶ���Ϊ��С�����ÿ�����Ԫ��
    for (auto i = M.begin(); i != M.end(); i++) {
        //���ڵ�i��
        for (auto j = i->begin(); j != i->end(); j++) {
            if (*j != '@') {
                handle[*j] = *(i->begin());//ѡ��*(i->begin())��Ϊ����
            }
        }
        if (*(i->begin()) != '@')
            min_dstate += *(i->begin());
    }

    //����� min_dfa

    set<string>NoCopy;//�����ظ�������
    for (auto i = handle.begin(); i != handle.end(); i++) {
        if (i->first == D_S)
            MIN_DS += i->second;
        if (D_F.find(i->first) != D_F.npos && MIN_DF.find(i->second) == MIN_DF.npos)
            MIN_DF += i->second;

        for (auto j = dfa[i->first].begin(); j != dfa[i->first].end(); j++) {
            if (j->next == '@')
                continue;//��״̬ȥ��
            edge2 e;
            e.next = handle[j->next];
            e.weight = j->weight;
            string ttemp;
            ttemp.push_back(i->second);
            ttemp.push_back(e.weight);
            ttemp.push_back(e.next);
            if (NoCopy.find(ttemp) == NoCopy.end()) {
                min_dfa[handle[i->second]].push_back(e);
                NoCopy.insert(ttemp);

            }
        }
    }
}
void printMIN_DFA() {
    for (auto it = min_dfa.begin(); it != min_dfa.end(); it++) {
        for (int i = 0; i < it->second.size(); i++) {
            string tmp1 = "[";
            string temp2 = (string)"��" + it->second[i].weight + "��>";
            cout << tmp1 << it->first << temp2 << it->second[i].next << "]" << endl;
        }
    }
}
int main() {
    //��ʼ��dfa
    D_S = 'A';
    D_F = "E";
    letters = "ab";//��ĸ��
    d_state = "ABCDE";
    edge2 e;
    e.next = 'C';
    e.weight = 'b';
    dfa['A'].push_back(e);
    dfa['E'].push_back(e);
    dfa['C'].push_back(e);
    e.weight = 'a';
    e.next = 'B';
    dfa['A'].push_back(e);
    dfa['D'].push_back(e);
    dfa['C'].push_back(e);
    dfa['B'].push_back(e);
    e.weight = 'b';
    e.next = 'D';
    dfa['B'].push_back(e);
    e.next = 'E';
    dfa['D'].push_back(e);
    e.next = 'B';
    e.weight = 'a';
    dfa['E'].push_back(e);
    cout << "��DFA������״̬��" << endl;
    addDeadState();
    printDFA();
    cout << "��С��DFA" << endl;
    MIN_dfa();
    cout << "��С��DFA�ĳ�̬:" << MIN_DS << endl;
    cout << "��С��DFA����̬:" << MIN_DF << endl;
    cout << "MIN_DFA״̬����:" << min_dstate << endl;
    printMIN_DFA();
    system("pause");
}
