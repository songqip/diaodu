#include<iostream>
#include<fstream>
#include"inst.h"
#include"store.h"
using namespace std;
extern vector<IR*> instructions;
extern  vector<Depend_node* > dependents;
extern vector<int> degree;
extern	map<string, int> hmap;
extern	vector<int> textures;
vector<int> bars;
vector<int> stss;
vector<int> imads;
vector<int> lds_A;
vector<int> lds_B;
//vector<int> other_insts;
//记录一条指令依赖的读写寄存器index
vector<vector<int> > read(256);
vector<vector<int> > write(256);
//hmap["IR"] 对应指令的延迟周期数
void init(){
	hmap["FFMA"] = 5;
	hmap["LD"] = 40;
	hmap["FADD"] = 9;
	hmap["MOV"] = 9;
	hmap["S2R"] = 9;
	hmap["IMAD"] = 9;
	hmap["MOV32I"] = 9;
	hmap["FADD32I"] = 9;
	hmap["IMUL"] = 9;
	hmap["SHR"] = 9;
	hmap["SHF"] = 9;
	hmap["I2F"] = 9;
	hmap["F2F"] = 9;
	hmap["DADD"] = 9;
	hmap["FADD32I"] = 9;
	hmap["LDS"] = 40;
	hmap["LDL"] = 40;
	hmap["TEXDEPBAR"] = 9;
	hmap["BAR"] = 9;
	hmap["STS"] = 5;
	hmap["STL"] = 9;
	hmap["LDG"] = 200;
	hmap["ISCADD"] = 9;
	hmap["IADD"] = 5;
	
	hmap["LOP"] = 9;
}
//记录指令对应的目标寄存器个数，比如LDS.128对应目标寄存器是4个
int NumReg(IR* ir){
	string operator_ = ir->operator_;
	int res = 0;
	for(int i = 0; i < operator_.size(); i++){
		if(isdigit(operator_[i])){
			res = res * 10 + (operator_[i] - '0');
		}
	}
	return res/32;
}
//cur_index reference the cur IR index, src is the cur IR subsrc
void create_depend(Produce* src, int cur_index) {
	if(src->mark == 2) {
		return;
	}
	int flag = 1;
	IR * tmp = instructions[cur_index];
	if(split(tmp->operator_) == "STS" || split(tmp->operator_) == "ST"  ){
		flag = NumReg(tmp);
	}
	if(split(tmp->operator_) == "LDG"  ){
		flag = 2;
	}
	int k = 0;
	while(k < flag){
		int r_index = src->reg_index + k;
		//w afer r depend
		read[r_index].push_back(tmp->index);
		if(write[r_index].size() > 0) {
			int w_len = write[r_index].size();
			for(int i = 0; i < w_len; i++) {
				int inst_index = write[r_index][i];
				if(inst_index == tmp->index){
					continue;
				}
				dependents[inst_index]->depency.insert(tmp->index);
				dependents[tmp->index]->pre_depency.insert(inst_index);
				//weight----
			}
		}
		k++;
	}
}
string split(string operator_){
	string tmp;
	for(int i = 0; i < operator_.size(); i++){
		if(operator_[i] != '.'){
			tmp += operator_[i];
		}else{
			break;
		}
		
	}
	return tmp;
}
//在处理依赖时，需要考虑指令FFMA，访存指令TEX，LDG，LDS，BAR，STS等
void depency() {
	init();
	int len = instructions.size();
	cout << "depency len = " << len << endl;
	degree.resize(len);
	for(int i = 0; i < 256; i++){
		read[i].clear();
		write[i].clear();
	}
	for(int i = 0; i < len; i++) {
		Depend_node* new_node = new Depend_node;
		new_node->index = instructions[i]->index;
		new_node->weight = hmap[split(instructions[i]->operator_)];
		dependents.push_back(new_node);
	}
	cout << "dependents create:" <<endl;
	int current_tex = -1;
	int current_ldg = -1;
	int current_sts = -1;
	int current_bar = -1;
	int current_A = -1;
	int current_B = -1;
	textures.clear();
	bars.clear();
	imads.clear();
	stss.clear();
	//other_insts.clear();
	//---------------------------------------------------
/*	ofstream outfile1;
	outfile1.open("ffma_instruction.txt", ios::out);
	if(outfile1.fail()){
		cout << "Fail to write~ "<< endl;
	}*/
	for(int i = 0; i <len; i++) {
		IR * tmp = instructions[i];
			
		if(split(tmp->operator_) == "BAR"){
			bars.push_back(tmp->index);
			cout << "bars index = " << i << endl;
			current_bar = tmp->index;
			for(int k = 0; k < stss.size(); k++){
				int sts_index = stss[k];
				dependents[sts_index]->depency.insert(tmp->index);
				dependents[tmp->index]->pre_depency.insert(sts_index);
			}
			stss.clear();

			continue;
		}
		else if(split(tmp->operator_) == "TEXDEPBAR"){
			current_tex = tmp->index;
			cout << "tex index = " << tmp->index << endl;
			if(current_ldg != -1){
				dependents[current_ldg]->depency.insert(tmp->index);
				dependents[tmp->index]->pre_depency.insert(current_ldg);
			}
			
			continue;
		}
		else if(split(tmp->operator_) == "LDG"){
			cout << "ldg index = " << tmp->index << endl;
			current_ldg = tmp->index;
			if(current_bar != -1){
				dependents[current_bar]->depency.insert(tmp->index);
				dependents[tmp->index]->pre_depency.insert(current_bar);
			}
			for(int k = 0; k < imads.size(); k++){
				int imad_index = imads[k];
				dependents[imad_index]->depency.insert(tmp->index);
				dependents[tmp->index]->pre_depency.insert(imad_index);
			}
			imads.clear();

		}
		else if(split(tmp->operator_) == "STS"){
			stss.push_back(tmp->index);
			cout << "sts index = " << tmp->index << endl;
			current_sts = tmp->index;
			//pre_depency = current_tex
			dependents[current_tex]->depency.insert(tmp->index);
			dependents[tmp->index]->pre_depency.insert(current_tex);
		}
		else if(split(tmp->operator_) == "LDS"){
			if(current_bar != -1){
				dependents[current_bar]->depency.insert(tmp->index);
				dependents[tmp->index]->pre_depency.insert(current_bar);
			}
			int src_index = tmp->src1->reg_index;
			cout << "LDS src_index = " << src_index <<endl;
			if(current_A == -1){
				current_A = src_index;
				lds_A.push_back(tmp->index);
			}else if(src_index != current_A && current_B == -1){
				current_B = src_index;
				lds_B.push_back(tmp->index);
			}else if(src_index == current_A){
				lds_A.push_back(tmp->index);
			}else if(src_index == current_B){
				lds_B.push_back(tmp->index);
			}
		}
		else if(split(tmp->operator_) == "IMAD"){
			imads.push_back(tmp->index);
		}
/*		if(split(tmp->operator_) == "FFMA"){
			outfile1 << tmp->control_code << "      " << tmp->operator_ << "  " << tmp->dst->const_num << "  " << tmp->src1->const_num << "  " << tmp->src2->const_num << "  " << tmp->src3->const_num << endl;
		}else if(split(tmp->operator_) == "MOV"){
			outfile1 << tmp->control_code << "      " << tmp->operator_ << "  " << tmp->dst->const_num << "  " << tmp->src1->const_num  << endl;
		}else{
			other_insts.push_back(i);
		}
*/
		int flag = 1;
		if(split(tmp->operator_) == "LDG" || split(tmp->operator_) == "LDS"||split(tmp->operator_) == "LD"  ){
			flag = NumReg(tmp);
		}
		
	// deal with dst register
		int w_index = tmp->dst->reg_index;
		int k = 0;
		while(k < flag){
			w_index = tmp->dst->reg_index + k;
		//w afer w depend
			if(write[w_index].size() > 0) {
				int w_len = write[w_index].size();
				for(int j = 0; j < w_len; j++) {
					int inst_index = write[w_index][j];
					dependents[inst_index]->depency.insert(tmp->index);
					//store predecessors
					dependents[tmp->index]->pre_depency.insert(inst_index);
					
		//			degree[tmp->index]++;
				}
			}
		//w after w override
			write[w_index].clear();
			write[w_index].push_back(tmp->index);
		// r after w depend
			if(read[w_index].size() > 0) {
				int r_len = read[w_index].size();
				for(int j = 0; j < r_len; j++) {
					int inst_index = read[w_index][j];
					dependents[inst_index]->depency.insert(tmp->index);
					dependents[tmp->index]->pre_depency.insert(inst_index);
				}
			}
			read[w_index].clear();
			k++;
		}
		//deal with src register
		create_depend(tmp->src1, i);
		if(tmp->src2 != NULL) {
			create_depend(tmp->src2, i);
		}
		if(tmp->src3 != NULL) {
			create_depend(tmp->src3, i);
		}
	}
	outfile1.close();
	cout << "ldA.size() = " << lds_A.size() << " ldB.size() = " << lds_B.size() << endl;
	ofstream outfile;
	outfile.open("ld_depend.txt", ios::out);
	if(outfile.fail()){
		cout << "Fail to write~ "<< endl;
	}
	set<int>::iterator iter;	
	for(int i = 0; i < lds_A.size(); i++){
		outfile << lds_A[i] << " -> ";
		for(iter = dependents[lds_A[i]]->depency.begin(); iter != dependents[lds_A[i]]->depency.end(); iter++){
			outfile << *iter << "  ";
		}
		outfile << endl;
	}
	for(int i = 0; i < lds_B.size(); i++){
		outfile << lds_B[i] << " -> ";
		for(iter = dependents[lds_B[i]]->depency.begin(); iter != dependents[lds_B[i]]->depency.end(); iter++){
			outfile << *iter << "  ";
		}
		outfile << endl;
	}
		
	for(int i = 0; i < len; i++){
		IR* tmp = instructions[i];
		degree[tmp->index] = dependents[tmp->index]->pre_depency.size();
		outfile << tmp->index << " " << degree[tmp->index] << endl;
	}
	outfile << "---------" << endl;
	outfile.close();
	
}
/*int main(){
	Read_asm();
	cout << "end"<<endl;
	cout << "instructions[0]" << instructions[0]->index << endl;
	for(int i = 0; i < 2; i++){
		cout << instructions[i]->index << " "<< instructions[i]->control_code << " "<< instructions[i]->dst->const_num << " " << instructions[i]->dst->reg_index << endl;
	}
	depency();
	int len = instructions.size();
	for(int i = 0; i < len; i++){
		for(int j = 0; j < dependents[i]->depency.size(); j++){
		cout << "i=" << i << "depency=" << dependents[i]->depency[j] << "  ";
		}
		cout << endl;
	}
		
}*/
