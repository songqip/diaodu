#include<iostream>
#include<fstream>
#include<sstream>
#include"inst.h"
#include"store.h"
using namespace std;

vector<IR*> instructions;
vector<Depend_node* > dependents;
vector<int> degree;
map<string, int> hmap;
vector<int> textures;
//get register index
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
//restore items of every IR
void Store_IR(char *buffer, int &count){
	IR *new_inst = new IR;
	new_inst->index = count;
	stringstream ss;
	ss << buffer;
	//cout << "buffer = " << buffer << endl;
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
	//Produce * dst_tmp = new Produce;
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
	
	instructions.push_back(new_inst);
	
	
	
}
//read the .asm file and split&&store IR information
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
