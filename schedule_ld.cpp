#include<iostream>
#include<sstream>
#include<fstream>
#include<list>
#include<queue>
#include"inst.h"
#include"store.h"
#define MAX 1000000
using namespace std;
extern vector<IR*> instructions;
extern  vector<Depend_node* > dependents;
extern vector<int> degree;
extern	map<string, int> hmap;
vector<int> priority;
extern	vector<int> textures;
extern vector<int> bars;
list<IR*>lists;
/*struct Cmp{
	bool operator()(int &a, int &b){
		if(priority[a] == priority[b]){
			return a > b;
		}
		return (priority[a] > priority[b]);
	}
};*/
bool isLSInst(IR *ir){
	if(split(ir->operator_) == "LD" || split(ir->operator_) == "ST"|| split(ir->operator_) == "LDG" || split(ir->operator_) == "STS"|| split(ir->operator_) == "LDS"  ){
		return true;
	}else{
		return false;
	}
}
bool isCaluInst(IR * ir){
	if(isLSInst(ir) || split(ir->operator_) == "TEXDEPBAR" ||split(ir->operator_) == "BAR"  ){
		return false;
	}else{
		return true;
	}
}
	
void control_update(string &s, string s1){
	stringstream ss;
	ss << s;
	string tmp;
	vector<string> controls;
	while(getline(ss, tmp, ':')){
		controls.push_back(tmp);
	}
	if(s1 != "00"){
		controls[2] = "D";
	}else{
		controls[2] = "-";
	}
	controls[4] = s1;
	s = controls[0] + ":" + controls[1] + ":" + controls[2] + ":" + controls[3]+ ":"  + controls[4];
}
void update_degree(int index, vector<int>& current){
	set<int>::iterator iter;	
	for(iter = dependents[index]->depency.begin(); iter != dependents[index]->depency.end(); iter++){
		int d_index = *iter;
		degree[d_index]--;
		int d_weight = dependents[index]->weight;
		current[d_index] = max(current[d_index], current[index] + d_weight);
		instructions[d_index]->current_cycle = current[d_index];
	}
}
void scheduling(){
	int len = instructions.size();
	cout << "instructions len = " << len << endl;	
		
	vector<int>current(len);
	queue<int> LD_queue;

	int inst = 0;
	int count = 1;
	int flag0 = 0;
	int flag1 = 0;
	int cycle = 0;
    //计算指令重排为1221的模式
	for(int i = 0; i < len; i++){
		if(!isCaluInst(instructions[i])){
			LD_queue.push(i);
			continue;
		}
		if(count == 1){
			control_update(instructions[i]->control_code, "05");
			lists.push_back(instructions[i]);
			current[i] = cycle;
			cycle++;
			count = 2;
			continue;
		}
		if(count == 2 && flag0 == 0){
			control_update(instructions[i]->control_code, "04");
			lists.push_back(instructions[i]);
			current[i] = cycle;
			flag0 = 1;
			continue;
		}
		if(count == 2 && flag0 == 1){
			control_update(instructions[i]->control_code, "05");
			lists.push_back(instructions[i]);
			current[i] = cycle;
			cycle++;
			count = 3;
			flag0 = 0;
			continue;
		}
		if(count == 3 && flag1 == 0){
			control_update(instructions[i]->control_code, "04");
			lists.push_back(instructions[i]);
			current[i] = cycle;
			flag1 = 1;
			continue;
		}
		if(count == 3 && flag1 == 1){
			
			control_update(instructions[i]->control_code, "05");
			lists.push_back(instructions[i]);
			current[i] = cycle;
			cycle++;
			flag1 = 0;
			count = 4;
			continue;
		}
		if(count == 4){
			control_update(instructions[i]->control_code, "04");
			lists.push_back(instructions[i]);
			current[i] = cycle;
			cycle++;
			IR* new_nap = new IR();
			new_nap->control_code = "-:-:-:-:00";
			new_nap->operator_ = "NOP;";
			new_nap->is_end = true;
			lists.push_back(new_nap);

			count = 1;
			continue;
		}
	}
	//-------------------------------------------
	//the last syn depency is the last ffma instruction
	// deal with load instruction
	cout << "LD_queue.size() = "<<LD_queue.size() << endl;

	map<int, bool> Ld_hmap;
	set<int>::iterator iter;
	while(!LD_queue.empty()){
		int index = LD_queue.front();
		Ld_hmap[index] = 1;
		cout << "LD_index = "<< index << endl;
		LD_queue.pop();
		//upper_index is statement the first inst can insert
		//lower_index is statement the last inst can inser
		int upper_index = -2;//NOp指令的index为-1，为区别NOP指令，将其index指定为-2
		int upper_bound = -1;
		for(iter = dependents[index]->pre_depency.begin(); iter != dependents[index]->pre_depency.end(); iter++){
			int pre_index = *iter;
			if(current[pre_index] > upper_bound){
				upper_index = pre_index;
				upper_bound = current[pre_index];
			}
		}
		int lower_index = MAX; //
		int lower_bound = MAX;
		for(iter = dependents[index]->depency.begin(); iter != dependents[index]->depency.end(); iter++){
			int pro_index = *iter;
			if(pro_index < lower_index){
				lower_index = pro_index;
			}
		}
		if(upper_index != -2){
			upper_bound = current[upper_index] + dependents[upper_index]->weight; 
		}
		if(lower_index != MAX){
			lower_bound = max(current[lower_index] - dependents[index]->weight, 0);
		}
		cout << "the  upper_index = " << upper_index << "lower_index = " << lower_index  << endl;

		//如果该load指令的上界或下届没有被load到inst list中，则需要循环向上（下）查找其在list中依赖的指令 
		if(upper_index != -2 && !isCaluInst(instructions[upper_index]) && Ld_hmap.count(upper_index) == 0){
			int upper_alter = 0;
			int upper_bound_alter = -1;
			set<int> ::iterator iter_set;
			for(iter_set = dependents[upper_index]->pre_depency.begin(); iter_set != dependents[upper_index]->pre_depency.end(); iter_set++){
				int index = *iter_set;
				if(current[index] > upper_bound_alter  && (isCaluInst(instructions[index])|| Ld_hmap[index] == 1)){
					upper_bound_alter = current[index];
					upper_alter = index;
				}
			}
			upper_index = upper_alter;
			upper_bound = current[upper_index] + dependents[upper_index]->weight; 
		}
		if(lower_index != MAX && !isCaluInst(instructions[lower_index]) && Ld_hmap.count(lower_index) == 0){
			set<int> ::iterator iter_set;
			while(!isCaluInst(instructions[lower_index]) && Ld_hmap.count(lower_index) == 0){
				int lower_alter = MAX;
				int lower_bound_alter = MAX;
				if(dependents[lower_index]->depency.size() == 0){
					break;
				}
				for(iter_set = dependents[lower_index]->depency.begin(); iter_set != dependents[lower_index]->depency.end(); iter_set++){
					int index = *iter_set;
					if(index < lower_alter){
						lower_alter = index;
						cout << "index = " << index << "lower_alter=" << lower_alter << endl;
					}
				}
				lower_index = lower_alter;
			}
			lower_bound = max(current[lower_index] - dependents[index]->weight, 0);

		}
		cout << "the second upper_index = " << upper_index << "lower_index = " << lower_index  << endl;
		
		list<IR* >::iterator iter;
		int lists_index = 0;
		if(upper_index == -2){
			iter = lists.begin();
		}else{
			int i = 0;
			for(iter = lists.begin(); iter != lists.end(); iter++){
				lists_index++;
				if( ((*iter))->index  == upper_index){
					break;
				}
			}
			iter++;
		}

		//complet is true if the ld inst has inserted
		bool complete = false;
		//情况1：在上下界中有合适的基本单元，可以通过替换NOP将访存指令插入
		while(complete == false && iter != lists.end() && (*iter)->index != lower_index){
			cout << "the first step:" << " iter->index = " << (*iter)->index << endl;
			if((*iter)->operator_ == "NOP;" && (*iter)->is_end == true){
			
				int front_step = 0;
				
				list<IR* >::iterator iter_new = iter;
				iter_new--;
				cout << "iter_new orig  " << (*iter_new)->operator_ << endl;
				
				while((front_step < 6) && ((*iter_new)->operator_ == "NOP;" || (*iter_new)->index == -1))
				{
					iter_new--;
					front_step++;
				}
				
				int current_cycle = current[(*iter_new)->index];
				if((current_cycle >= lower_bound) || (current_cycle >= upper_bound && current_cycle <= lower_bound)){
					iter_new = iter;
					lists.insert(iter_new, 1, instructions[index]);
		        	control_update(instructions[index]->control_code, "00");
					current[index] = current_cycle + 1;
					instructions[index]->is_end = true;
					iter_new = lists.erase(iter_new);
					complete = true;
					break;
				}else if(current_cycle < upper_bound && current_cycle > lower_bound){
					iter++;	
					continue;
				}
			}
			iter++;

		}
		
		if(complete == false){
			//has no lower bound
			if(iter == lists.end() ){
				iter--;
				int cycle = current[(*iter)->index]; 
				while((*iter)->index != upper_index && cycle > upper_bound){
					iter--;
					cycle = current[(*iter)->index];
				}
				iter++;
				while(iter != lists.end() ){
					if((*iter)->is_end == false){
						iter++;
					}else{
						break;
					}
				}
				if(iter == lists.end()){
					lists.insert(iter,1,instructions[index]);
			        control_update(instructions[index]->control_code, "00");
					instructions[index]->is_end = true;
					current[index] = current[(*(--iter))->index];
					complete = true;
					
				}else{
					iter++;
					lists.insert(iter,1,instructions[index]);
        			control_update(instructions[index]->control_code, "00");
					current[index] = current[(*iter)->index];
					for(int k = 0; k < 5; k++){
						IR* new_nap = new IR();
						new_nap->control_code = "-:-:-:-:00";
						new_nap->operator_ = "NOP;";
						lists.insert(iter, 1,new_nap);
					}
					IR* new_nap = new IR();
					new_nap->control_code = "-:-:-:-:00";
					new_nap->operator_ = "NOP;";
					new_nap->is_end = true;
					lists.insert(iter, 1, new_nap);

					cout << "-------------------------"<< endl;
					complete = true;
				}

			}
			if(iter != lists.begin()){
				iter--; //此时是检测没有iter到lowerindex时仍没有可以插入的nop时，在其中间找一个合适的位置插入一个block（处理upper，lower不在一个block内的情况）
			}
            //情况2：在上下界中没有找到合适的基本块替换NOP，所以在上下界中找个合适的位置用NOP填充插入一个基本块
			while( (*iter)->index != upper_index && complete == false ){
				cout << "insert 2 factor, iter->index = " << (*iter)->index << endl;	
				if((*iter)->is_end == true && (*iter)->index != -1){
					int current_cycle = current[(*iter)->index];
					cout << "index = " << (*iter)->index <<  "current_cycle = " << current_cycle << endl;
					//insert a block
					if((current_cycle < upper_bound ) ||
							(current_cycle >= upper_bound && current_cycle <= lower_bound)){
							iter++;
							lists.insert(iter,1,instructions[index]);
			                control_update(instructions[index]->control_code, "00");
							current[index] = current_cycle - 1;
							for(int k = 0; k < 5; k++){
								IR* new_nap = new IR();
								new_nap->control_code = "-:-:-:-:00";
								new_nap->operator_ = "NOP;";
								lists.insert(iter, 1, new_nap);
							}
							IR* new_nap_last = new IR();
							new_nap_last->control_code = "-:-:-:-:00";
							new_nap_last->operator_ = "NOP;";
							new_nap_last->is_end = true;
							lists.insert(iter, 1, new_nap_last);
							cout << "-------------------------"<< endl;
							complete = true;
							break;
					}
					if(current_cycle >= upper_bound && current_cycle > lower_bound){
						if(iter == lists.begin()){
							break;
						}
						iter--;
						continue;
					}
					
				}
				if(iter == lists.begin()){
					break;
				}
				iter--;
			}
			//情况3：upper_index , lower_index in one block,在上下界之间插入一个基本块
			if(complete == false){
				cout << "insert 3 factor, iter->index = " << (*iter)->index << endl;	
				//if has no upper_index
				if(iter == lists.begin()){
					lists.insert(iter,1,instructions[index]);
        			control_update(instructions[index]->control_code, "00");
					current[index] = 0;
					for(int i = 0; i < 6; i++){
						IR* new_nap = new IR();
						new_nap->control_code = "-:-:-:-:00";
						new_nap->operator_ = "NOP;";
						lists.insert(iter, 1, new_nap);
					}
					iter--;
					(*iter)->is_end = true;
					complete = true;
				}
				else{
					if((*iter)->issue == true){
						if((*(++iter))->issue == true && (*iter)->index != lower_index){
							iter++;
						}
						
					}else{
						iter++;
					}
					int index_before = (*iter)->index;
					cout << "index_before = " << index_before << endl;
					lists.insert(iter,1,instructions[index]);
	        		control_update(instructions[index]->control_code, "00");
					int step = 0;
					while((*iter)->is_end != true){
						iter++;
						step++;
					}
					cout << "step = " << step << endl;
					for(int k = 0; k < step; k++){
						iter--;
					}
					for(int i = 0; i < (6-step); i++){
						IR* new_nap = new IR();
						new_nap->control_code = "-:-:-:-:00";
						new_nap->operator_ = "NOP;";
						lists.insert(iter, 1, new_nap);
					}
					for(int i = 0; i < (6-step); i++){
						iter--;
					}
					for(int i = 0; i < step; i++){
						IR* new_nap = new IR();
						new_nap->control_code = "-:-:-:-:00";
						new_nap->operator_ = "NOP;";
						lists.insert(iter, 1, new_nap);
					}
					iter--;
					(*iter)->is_end = true;


					//current[index] = current[upper_index] + dependents[upper_index]->weight;	
					current[index] = current[upper_index] + 1;	
					complete = true;
				}
				
			}
		}
		cout << "index = " << index << "cycel = " << current[index] << endl;

	}

	IR* new_nap = new IR();
	new_nap->control_code = "-:-:-:-:14";
	new_nap->operator_ = "EXIT;";
	lists.push_back(new_nap);

				
				
}
//通过检查调度前后的依赖关系来检查调度正确性
void check(){
	list<IR* >::iterator iter;
	ofstream outfile;
	outfile.open("dependent_2_8.txt", ios::out);
	if(outfile.fail()){
		cout << "Fail to write~ "<< endl;
	}
	ofstream outfile1;
	outfile1.open("depend_check.txt", ios::out);
	if(outfile1.fail()){
		cout << "Fail to write~ "<< endl;
	}

	instructions.clear();
	for(iter = lists.begin(); iter != lists.end(); iter++){
		if((*iter)->operator_ == "NOP;" || (*iter)->operator_ == "EXIT;"){
			continue;
		}else{
			instructions.push_back(*iter);
			outfile1 << (*iter)->index << "  " << (*iter)->operator_ << "  " ;
			if((*iter)->dst != NULL){
				outfile1 << (*iter)->dst->const_num << " ";
			}
			if((*iter)->src1 != NULL){
				outfile1 << (*iter)->src1->const_num << " ";
			}
			outfile1 << endl;
		}
	}
	int len = instructions.size();
	cout << "check len = " << len << endl;
	degree.clear();
	dependents.clear();

	depency();
	queue<int> forward;
	for(int i = 0; i < len; i++){
		if(degree[i] == 0){
			forward.push(i);
			cout << "forward index = " << i << endl;
		}
	}
	vector<int> degree_copy(degree);
	while(!forward.empty()){
		int index = forward.front();
		forward.pop();
		set<int>::iterator iter;
		outfile << index << "<-";
		for(iter = dependents[index]->pre_depency.begin(); iter != dependents[index]->pre_depency.end(); iter++){
			outfile << *iter<< " ";
		}
		outfile << endl;
	
	
		for(iter = dependents[index]->depency.begin(); iter != dependents[index]->depency.end(); iter++){
			int d_index = *iter;
			int d_weight = dependents[index]->weight;
			degree_copy[d_index]--;
			if(degree_copy[d_index] == 0){
				forward.push(d_index);
			}
		}
	}
	outfile.close();
}

//输出调度后的asm文件
void out_file(){
	ofstream outfile;
	outfile.open("test_out_asm_128.txt", ios::app);
	ofstream outfile1;
	outfile1.open("test_out_asm_128_1.txt", ios::app);
	if(outfile.fail()){
		cout << "Fail to write~ "<< endl;
	}else{
		list<IR* >::iterator iter;
		int start = 0;
		for(iter = lists.begin(); iter != lists.end(); iter++){
			start++;
			outfile << ((*iter))->control_code << "      " <<( *iter)->operator_ << " " ;
			outfile1 << ((*iter))->control_code << "      " <<( *iter)->operator_ << " " ;
			if((*iter)->dst != NULL){
				outfile << (*iter)->dst->const_num << " ";
				outfile1 << (*iter)->dst->const_num << " ";
			}
			if((*iter)->src1 != NULL){
				outfile << (*iter)->src1->const_num << " ";
				outfile1 << (*iter)->src1->const_num << " ";
			}
			
			if(((*iter))->src2 != NULL){
				outfile << ((*iter))->src2->const_num << " ";
				outfile1 << ((*iter))->src2->const_num << " ";
			}
			if(((*iter))->src3 != NULL){
				outfile << ((*iter))->src3->const_num << " ";
				outfile1 << ((*iter))->src3->const_num << " ";
			}
			outfile << endl;
			outfile1 << endl;
			if(start % 7 == 0){
				outfile1 << "---------------------------" << endl;
			}
		}
	}
	outfile.close();
	outfile1.close();
}
int main(){
	Read_asm();
	cout << "end Read_asm..." << endl;
	depency();
	cout << "end depency..." << endl;
	scheduling();
	cout << "end schedule..." << endl;
	check();
	cout << "end check" << endl;
	out_file();
}


