#include<iostream>
#include<stack>
#include<string>
#include <algorithm> 
#include<vector>
#include<set>
#include<map>
using namespace std;

map<char, set<char>>scount;

int main() {
    set<int>a={1,2};
    set<int>b;
    b=a;
    cout<<&a<<endl;
    cout<<&b<<endl;
}
