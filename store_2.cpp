#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include"inst.h"
#include"store.h"
using namespace std;
vector<IR*> instructions;
vector<Depend_node* > dependents;
vector<int> degree;
map<string, int> hmap;
vector<int> textures;

int GetInt(string s){
	int len = s.size();
	int i = 1;
	int res = 0;
	while(i < len){
		if(isdigit(s[i])){
			res = res * 10 + s[i] - '0';
		}else{
			break;
		}
		i++;
	}
	return res;
}
//拆分load128-》load64
string rewrite_ope(string s){
	int i = 0;
	for(; i < s.size(); i++){
		if(isdigit(s[i])){
			break;
		}
	}
	string s1 = s.substr(0, i);
	s1 = s1 + "64";
	return s1;
}
//重写目标寄存器
Produce* rewrite_dst(Produce* pro1){
	Produce* pro2 = new Produce();
	pro2->mark = pro1->mark;
	pro2->reg_index = pro1->reg_index + 2;
	stringstream ss;
	ss << pro2->reg_index;
	pro2->const_num = "R" + ss.str() + ",";
	return pro2;
}
//重写load地址
Produce* rewrite_src(Produce* pro1){
	Produce* pro2 = new Produce();
	pro2->mark = pro1->mark;
	pro2->reg_index = pro1->reg_index;
	string s = pro1->const_num;
	int begin = -1;
	int end = -1;
	for(int i = 0; i < s.size(); i++){
		if(s[i] == 'x'){
			begin = i+1;
		}else if(s[i] == ']'){
			end = i;
		}
	}
	int offset_num = 0;
	if(begin != -1){
		s[end-1] = '8';
	}else{
		s = s.substr(0, end) + "+0x008" + s.substr(end);
	}
	pro2->const_num = s;
	return pro2;
}

Produce* create_produce(string s ){
	Produce * dst_tmp = new Produce;
	if(s[0] == 'R' && s[1] != 'Z') {
		dst_tmp->mark = 1;
		dst_tmp->reg_index = GetInt(s);
		dst_tmp->const_num = s;
	}else if(s[0] == '['&& s[1] == 'R'){
		dst_tmp->mark = 1;
		dst_tmp->reg_index = GetInt(s.substr(1));
		dst_tmp->const_num = s;
	}
	else{
		dst_tmp->mark = 2;
		dst_tmp->const_num = s;
	}
	return dst_tmp; 
}

void Store_IR(char *buffer, int &count){
	IR *new_inst = new IR;
	new_inst->index = count;
	stringstream ss;
	ss << buffer;
	string temp;
	vector<string> restore;
	while(getline(ss, temp, ' ')){
		if(temp != ""){
			restore.push_back(temp);
		}
	} 
	new_inst->control_code = restore[0];
	new_inst->operator_ = restore[1];
	if(new_inst->operator_ == "NOP;"){
		cout << "count = " << count << endl;
		delete new_inst;
		count--;
		return;
	}
	if(new_inst->operator_ == "TEXDEPBAR"){
		textures.push_back(count);
	}
	new_inst->dst = create_produce(restore[2]);
	int len = restore.size();
	if(len > 3){
		new_inst->src1 = create_produce(restore[3]);
	}
	if(len > 4){
		new_inst->src2 = create_produce(restore[4]);
	}
	if(len > 5){
		new_inst->src3 = create_produce(restore[5]);
	}
	if(new_inst->operator_ == "LDS.128"){
		new_inst->operator_ = rewrite_ope(new_inst->operator_);
		IR* new_inst2 = new IR();
		new_inst2->operator_ = new_inst->operator_;
		new_inst2->index = count+1;
		count++;
		new_inst2->control_code = new_inst->control_code;
		new_inst2->dst = rewrite_dst(new_inst->dst);
		new_inst2->src1 = rewrite_src(new_inst->src1);
//		cout << new_inst2->index << new_inst2->operator_ << " " << new_inst2->dst->const_num << " " << new_inst2->src1->const_num << endl;
		instructions.push_back(new_inst2);
	}
	
	instructions.push_back(new_inst);
}

void Read_asm() {
	ifstream inFile;
	char buffer[256];
	int count = 0;
	ofstream outfile;
	outfile.open("test_out_asm_128.txt", ios::app);
	if(outfile.fail()){
		cout << "Fail to write~ "<< endl;
	}
	inFile.open("main_128.asm", ios::in);
	if(inFile.fail()) {
		cout << "Fail to Read~ "<<endl;
	}else{
		while(!inFile.eof()){
			inFile.getline(buffer, 256, '\n');
			if(buffer[0] == '-' || buffer[0] == 'T'){
				Store_IR(buffer, count);
				count++;
			}else{
				outfile << buffer << endl ;
			}
		}
	}
	cout << "end Read_asm" << endl;
}

