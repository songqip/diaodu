#ifndef INST_H
#define INST_H

#include<iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
using namespace std;
class Produce{
	public:
		int get_mark();
		int mark;    // mark == 1 referance register  mark == 2 referance const num
		int reg_index ;
		string const_num;
};
//STRUCT TO store IR
class IR{
	public:
	    IR(): index(-1),issue(false),is_end(false),current_cycle(0),dst(NULL),src1(NULL),src2(NULL),src3(NULL) {}
		int index; //instruction id
		bool issue;//true reference issue dual
		bool is_end; // the last inst of a block
		int current_cycle;
		string control_code; //control code
		string operator_; //operator
		Produce * dst; //output operand
		Produce * src1;
		Produce * src2;
		Produce * src3;

	
};

class Depend_node{
	public:
		Depend_node():index(-1),weight(0) {}
		int index;
		set<int> depency; //store the dependency instruction index
		set<int> pre_depency;//store the predecessors
		int weight;
};
/*vector<IR*> instructions;
vector<Depend_node* > dependents;
vector<int> degree;
map<string, int> hmap;*/
//contract.insert(map<string, int>:: value_type("FFMA",9);
#endif
