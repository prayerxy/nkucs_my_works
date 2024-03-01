#include<iostream>
#include<map>
#include<set>
#include<vector>
#include<stack>
#include <unordered_map>
using namespace std;

string letters;//字母表
string expression, infix, suffix;//中缀、后缀字符串
string d_state;//状态集合
string min_dstate;//最小化dfa的状态集合
map<char, int>precedence;//优先级
int start = 0;//nfa状态的名字
char d_start = '@';//dfa状态的名字
char null_action = '~';//空字用'~'替代
char dead_state = '@';
int dead_flag = 0;//是否有死状态标志
struct edge {
    char weight;//letter，给定字母
    int next;  //下一个nfa状态
};
struct edge2 {
    char weight;//letter，给定字母
    char next;//下一个dfa状态
};
map<int, int>left_right;
map<int, vector<edge>>nfa;//nfa 起点，起点相连的边
map<char, vector<edge2>>dfa;//dfa 
map<char, vector<edge2>>min_dfa;//最小化的dfa
char D_S;
string D_F;//初态与终态
string MIN_DS;//最小化的初态
string MIN_DF;//最小化的终态
map<set<int>, char>class2state;//nfa子集与dfa中状态的映射
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
//增加连接符
void expr2infix() {
    for (int i = 0; i < expression.length(); i++) {
        char cur = expression[i];
        char next = expression[i + 1];
        //默认连接符省略，这里增加'.'
        if ((cur != '|' && cur != '.' && cur != '(') && (next != ')' && next != '|' && next != '*' && next != '.' && next != '\0')) {
            infix = infix + cur + '.';
        }
        else {
            infix += cur;
        }
    }
}
//中缀转为后缀
//若栈非空，判断栈顶操作符，若该操作符，该操作符入栈；
//当前操作符优先级低于栈顶操作符，那么要让栈顶操作符出栈。
//直到栈空或栈顶操作符优先级低于该操作符，该操作符再入栈。
void infix2suffix() {
    stack<char>cstack;
    for (int i = 0; i < infix.size(); i++) {
        char cur = infix[i];//当前字符
        if (cur == '(')
            cstack.push(cur);
        else if (cur == ')') {//需要把括号里面全部解决
            while (cstack.top() != '(') {
                char temp = cstack.top();
                suffix += temp;
                cstack.pop();
            }
            cstack.pop();//删掉左括号
        }
        else if (cur == '|' || cur == '*' || cur == '.') {
            while (!cstack.empty()) {
                char temp = cstack.top();
                //cur优先级低，需要把过去的temp打印
                if (precedence[temp] >= precedence[cur]) {
                    suffix += temp;
                    cstack.pop();
                }
                else {//当前字符优先级高
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
            string temp2 = (string)"—" + it->second[i].weight + "—>";
            cout << tmp1 << it->first << temp2 << it->second[i].next << "]" << endl;
        }
    }
}
int suffix2nfa() {
    stack<int>cstack;//储存状态
    for (int i = 0; i < suffix.size(); i++) {
        char cur = suffix[i];
        if (cur == '*') {
            int l_temp = cstack.top();
            int r_temp = left_right[l_temp];
            left_right.erase(l_temp);
            cstack.pop();
            start++;//作为开始状态
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
            cstack.push(start - 1);//把最左边状态入栈

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
            //不需要再pop
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
            start++;//初态
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
            start++;//初态从1开始
            edge e;
            e.weight = cur;
            e.next = start + 1;
            nfa[start].push_back(e);
            cstack.push(start);
            left_right[start] = e.next;
            start++;
        }
    }
    return cstack.top();//初态
}
//计算集合subClass的闭包'~'空字
//用set可以去重
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
                if (temp.find(nfa[cur][i].next) == temp.end()) {//说明temp集合里面没有
                    cstack.push(nfa[cur][i].next);
                    temp.insert(nfa[cur][i].next);
                }
            }
        }
    }
    return temp;
}
//a是输入符号
set<int> transition(set<int>subClass, char a) {
    set<int>temp;
    //用nfa的状态转换
    for (auto i = subClass.begin(); i != subClass.end(); i++) {
        for (auto j = 0; j < nfa[*i].size(); j++) {
            if (nfa[*i][j].weight == a) {
                if (temp.find(nfa[*i][j].next) == temp.end()) {//说明temp集合里面没有
                    temp.insert(nfa[*i][j].next);
                }
            }
        }
    }
    return temp;
}
//nfa->dfa
//参数的S代表初态，F代表终态 是nfa的
void nfa2dfa(int S, int F) {
    set<int>temp;
    temp.insert(S);
    set<int>set_S = _closure(temp);//作为初态
    set<set<int>>dstates2; //起一个查重的作用
    vector<set<int>>dstates;//dfa状态集 int是标记位
    int flag = 0;//代表标记状态
    dstates.push_back(set_S);
    dstates2.insert(set_S);
    d_start++;//设置开始状态
    D_S = d_start;
    d_state = d_state + (char)d_start;
    class2state[set_S] = d_start;
    while (flag < dstates.size()) {
        flag++;//标记的作用
        set<int>cur(dstates[flag - 1]);//当前状态集合
        for (int i = 0; i < letters.size(); i++) {//对于每个符号，letters是字母表
            set<int>U = _closure(transition(cur, letters[i]));
            if (dstates2.find(U) == dstates2.end() && !U.empty()) {//不能为空，且必须之前没有
                dstates.push_back(U);
                dstates2.insert(U);
                d_start++;//新状态
                d_state = d_state + (char)d_start;
                class2state[U] = d_start;
            }
            if (U.empty())
                continue;
            edge2 e;
            e.weight = letters[i];
            e.next = class2state[U];
            dfa[class2state[cur]].push_back(e);//完成dfa的转换
        }
    }
    //最后完成终态设置
    for (int i = 0; i < dstates.size(); i++) {
        if (dstates[i].find(F) != dstates[i].end()) {
            D_F.push_back(class2state[dstates[i]]);//设置终态
        }
    }
}
void printDFA() {
    for (auto it = dfa.begin(); it != dfa.end(); it++) {
        for (int i = 0; i < it->second.size(); i++) {
            string tmp1 = "[";
            string temp2 = (string)"—" + it->second[i].weight + "—>";
            cout << tmp1 << it->first << temp2 << it->second[i].next << "]" << endl;
        }
    }
}
//最小化dfa
void addDeadState() {
    for (int i = 0; i < d_state.size(); i++) {
        //增加死状态
        if (dfa[d_state[i]].size() < letters.size()) {
            for (int j = 0; j < letters.size(); j++) {
                int cur_flag = 0;//是否存在该符号对应边
                for (int k = 0; k < dfa[d_state[i]].size(); k++) {
                    if (dfa[d_state[i]][k].weight == letters[j]) {
                        cur_flag = 1;
                        break;
                    }
                }
                //说明没有该符号对应输入，增加死状态对应转换
                if (cur_flag == 0) {
                    dead_flag = 1;
                    edge2 e;
                    e.next = dead_state;//死状态
                    e.weight = letters[j];
                    dfa[d_state[i]].push_back(e);
                }
                else {}//对于当前符号不用加边
            }
        }
        else {}//说明边的数量等于符号个数，没有死状态

    }
    if (dead_flag) {
        for (int i = 0; i < letters.size(); i++) {
            edge2 e;
            e.next = dead_state;//死状态
            e.weight = letters[i];
            dfa[dead_state].push_back(e);
        }

    }
}
void MIN_dfa() {
    set<char>s1;
    set<char>s2;
    map<char, set<char>>count;//记录了dfa每个状态与集合的对应关系，不断更新
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
    unordered_map<char, map<char, set<char>>>transi_count;//第一个char对应状态 第二个对应动作与组别的映射
    //s1是不可接受,s2可接受
    set<set<char>>Mnew;
    int size_flag;
    while (M != Mnew) {
        //count的映射会不断被覆盖
        if (Mnew.size() != 0)
            M = Mnew;
        Mnew.clear();//新的重置
        for (auto i = M.begin(); i != M.end(); i++) {
            //对每个组执行如下操作
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
                char flag = '0';//判断是否可以找到一组的状态标志
                for (auto e = transi_count.begin(); e != w; e++) {
                    //遍历所有输入
                    for (int k = 0; k < letters.size(); k++) {
                        if (w->second[letters[k]] != e->second[letters[k]]) {
                            break;
                        }
                        if (k == letters.size() - 1)
                            flag = e->first;
                    }
                    //如果当前找到了一个，就退出
                    if (flag != '0')
                        break;
                }
                if (flag != '0') {
                    count[flag].insert(w->first);
                    count[w->first] = count[flag];//count 映射一个状态，与一个集合
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
                    Mnew.insert(iter->second);//新的组
                    /*for (auto sk = iter->second.begin(); sk != iter->second.end(); sk++)
                        cout << *sk << " ";
                    cout << endl;*/
                }

            }
        }
    }
    //现在M是所有组的集合，需要清除死状态'@'
    map<char, char>handle;//第一个为dfa中所有状态，第二个为最小化后的每组代表元素
    for (auto i = M.begin(); i != M.end(); i++) {
        //对于第i组
        for (auto j = i->begin(); j != i->end(); j++) {
            if (*j != '@') {
                handle[*j] = *(i->begin());//选用*(i->begin())作为代表
            }
        }
        if (*(i->begin()) != '@')
            min_dstate += *(i->begin());
    }

    //处理边 min_dfa

    set<string>NoCopy;//处理重复的问题
    for (auto i = handle.begin(); i != handle.end(); i++) {
        if (i->first == D_S)
            MIN_DS += i->second;
        if (D_F.find(i->first) != D_F.npos && MIN_DF.find(i->second) == MIN_DF.npos)
            MIN_DF += i->second;

        for (auto j = dfa[i->first].begin(); j != dfa[i->first].end(); j++) {
            if (j->next == '@')
                continue;//死状态去除
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
            string temp2 = (string)"—" + it->second[i].weight + "—>";
            cout << tmp1 << it->first << temp2 << it->second[i].next << "]" << endl;
        }
    }
}
int main() {
    //初始化dfa
    D_S = 'A';
    D_F = "E";
    letters = "ab";//字母表
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
    cout << "对DFA增加死状态后" << endl;
    addDeadState();
    printDFA();
    cout << "最小化DFA" << endl;
    MIN_dfa();
    cout << "最小化DFA的初态:" << MIN_DS << endl;
    cout << "最小化DFA的终态:" << MIN_DF << endl;
    cout << "MIN_DFA状态集合:" << min_dstate << endl;
    printMIN_DFA();
    system("pause");
}
