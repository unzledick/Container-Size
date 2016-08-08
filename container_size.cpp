#include <stdio.h>
#include <stdlib.h>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <float.h>

//=============================================================

#define max(x,y) x>y ? x:y
#define min(x,y) x<y ? x:y
using namespace std ;

//=============================================================

//two day resource usage record
double day1_cpu_usage[62];
long long int day1_mem_usage[62];
double day2_cpu_usage[68];
long long int day2_mem_usage[68];

//total cost of 180 set of container sizes
double day1_result[180];
double day2_result[180];

//minimum resource usage of ticket-monster
double default_cpu = 5;
long long int default_mem = 0.95*1024*1024*1024;

//growth ratio of price as resource doubles
double double_mem_price = 0.185/0.146;
double double_cpu_price = 0.293/0.185;

//=============================================================

//read resource usage from json file
void read_json_tree_from_file(string fname, double* cpu_usage, long long int* mem_usage, int min){
	Json::Reader reader;
	Json::Value root;
	ifstream is;
	is.open(fname, ios::in);
	if (is.is_open()) {
		reader.parse(is, root);
	}
	cpu_usage[min]  = root["Application"][8]["pod"]["PodInfo"][0]["Contents"]["CpuUsage"].asDouble();
	mem_usage[min]  = root["Application"][8]["pod"]["PodInfo"][0]["Contents"]["MemoryUsage"].asDouble();
}

//read from file that stores a set of filename of monitorOutput.json
//one json file stores information of one minute
void read_file(string path,char* filename, double* cpu_usage, long long int*
		mem_usage, int length){

	FILE *fPtr;		 
	string *jsonname = new string[length];
	fPtr = fopen(filename,"r");

	for(int i=0;i<length;i++){
		char temp[12];
		fscanf(fPtr , "%s" , temp);
		jsonname[i].assign(temp);
		jsonname[i] = path +"/data/" + jsonname[i];
		read_json_tree_from_file(jsonname[i],cpu_usage,mem_usage,i);
	}	
	fclose(fPtr); 
}

//calculate the maximum of cpu record
double count_cpu_limit(double* cpu_record,int length){
	double cpu_limit = cpu_record[0];
	for(int i = 0;i<length;i++){
		if(cpu_record[i] > cpu_limit)
			cpu_limit = cpu_record[i];
	}
	return cpu_limit;
}

//calculate the maximum of memory record
long long int count_mem_limit(long long int* mem_record,int length){
	long long int mem_limit = mem_record[0];
	for(int i = 0;i<length;i++){
		if(mem_record[i] > mem_limit)
			mem_limit = mem_record[i];
	}
	return mem_limit;
}

//calculate the price of the input container size
double get_price(int cpu, int mem){
	return mem*pow(double_mem_price/2,log2(mem)) * cpu*pow(double_cpu_price/2,log2(cpu));
}

//calculate the maximum of the required number of container
double choose_max_number(double cpu_size, long long int mem_size, double*
		cpu_usage, long long int* mem_usage, int length){

	double max_number = -1;

	for(int i = 0; i < length; i++){
		double temp_num = max(cpu_usage[i]/cpu_size + 1,
				mem_usage[i]/mem_size + 1);
		if(temp_num > max_number)
			max_number = temp_num;
	}

	return max_number;
}

//calculate the total cost by dynamic programming
double optimal_number(double cpu_size, long long int mem_size, double* cpu_usage,
		long long int* mem_usage, int length){
	
	int max_number = choose_max_number(cpu_size,mem_size,cpu_usage,mem_usage,length);
	double* optimal = (double*)calloc(length*(max_number+1), sizeof(double));
	double pre_number = 0;
	double optimal_answer = DBL_MAX;
	
	// time step
	for(int i = 0; i < length; i++){

		// required number of container
		int temp_num = max((int)((cpu_usage[i]-default_cpu)/cpu_size) + 1, 
				(int)((mem_usage[i]-default_mem)/mem_size) + 1);	
		
		//possible deployed number of container
		for(int j = temp_num; j<= max_number;j++){
			
			//time = 0
			if(i == 0){
				optimal[j] = (double)j;
			}

			//
			else{
				optimal[i*(max_number+1) + j] = DBL_MAX;
				for(int k = pre_number;k <=max_number;k++){
					double temp = optimal[(i-1)*(max_number+1)+k] +
						((double)abs(k-j))/2 + (double)j;

					if(optimal[i*(max_number+1) + j] > temp)
						optimal[i*(max_number+1) + j] = temp;	
				}
			}

			//time = t
			if(i == length-1)
				if(optimal_answer > optimal[i*(max_number+1) + j])	
					optimal_answer = optimal[i*(max_number+1) + j];
		}	
		pre_number = temp_num;	
	}		

	return optimal_answer;
}

//calculate the total cost by greedy heuristic
double heuristic(double cpu_size, long long int mem_size, double* cpu_usage,
		long long int* mem_usage, int length){
	int pre_number = 0;
	double total_number = 0;
	
	for(int i = 0; i < length;i++){
		double deploy_number;
		
		// required number of container
		int need = max((int)((cpu_usage[i]-default_cpu)/cpu_size) + 1, (int)((mem_usage[i]-default_mem)/mem_size) + 1);
		
		//total cost
		total_number += (double)need + fabs((double)pre_number-(double)need)/2;
		
		deploy_number = need;
		pre_number = deploy_number;
	}	
	return total_number; 

}

//select the best container size
void choose_size(double cpu_limit,long long int mem_limit, double* cpu_usage,
		long long int* mem_usage, int length, double* day_result){

	double temp_cpu;
	long long int temp_mem;
	double price;
	double cost_d, cost_h;
	int min_d_cpu,min_h_cpu,min_d_mem,min_h_mem;
	double min_d = DBL_MAX, min_h = DBL_MAX;
	int num = 0;

	// 20 settings of cpu size
	for(int i = 1;i<=20;i++){
		temp_cpu = cpu_limit/10*i;
		if(temp_cpu >default_cpu){
			temp_cpu -= default_cpu;
			
			// 20 settings of mem size
			for(int j = 1;j<=20;j++){
				temp_mem = mem_limit/10*j;
				if(temp_mem > default_mem){
					temp_mem -= default_mem;

					//calculate total cost by dp and heuristic
					cost_d = optimal_number(temp_cpu,temp_mem,cpu_usage,mem_usage,length);
					cost_h = heuristic(temp_cpu,temp_mem,cpu_usage,mem_usage,length);
					
					//price of container size
					price = get_price(i,j);

					//store total cost of different container settings
					day_result[num] = cost_h * price;
					num += 1;
					
					//calculate the percentage of total cost between heuristic and dp
					//double percent = (cost_h-cost_d)/cost_d;
					//printf("cpu = %d,mem = %d,percent = %f\n",i,j,percent);
					//printf("%d  %f %f \n",num,cost_d,cost_h);
					
					//select maximum of total cost
					if(price*cost_d <  min_d){
						min_d = price*cost_d;
						min_d_cpu = i;
						min_d_mem = j;
					}
					if(price*cost_h <  min_h){
						min_h = price*cost_h;
						min_h_cpu = i;
						min_h_mem =j;
					}
				}
			}	
		}	
	}

	//print result
	printf("num = %d\n",num);
	printf("cpu = %d,mem = %d, optimal = %f\n",min_d_cpu,min_d_mem,min_d);
	printf("cpu = %d,mem = %d, heuristic = %f\n",min_h_cpu,min_h_mem,min_h);

}


//print result as gnuplot input
void print_record(){
	
	// day1 and day2 resource usage record 
	for(int i = 0;i<60;i++){
		printf(" %d %f %f\n",i,day1_cpu_usage[i],day2_cpu_usage[i]);
		printf(" %d %f %f\n",i,((double)day1_mem_usage[i])/1024/1024,((double)day2_mem_usage[i])/1024/1024);
	}	

	// compare day1 and day2 total cost of different container sizes
	for(int i = 0;i<180;i++){
		printf(" %d %f %f\n",i,day1_result[i],day2_result[i]);
	}	
}	

int main(){

	//read input from file
	read_file("day_one",(char*)"input.txt",day1_cpu_usage,day1_mem_usage,62);
	read_file("day_two",(char*)"input2.txt",day2_cpu_usage,day2_mem_usage,68);

	//calculate maximum resource usage	
	double cpu_limit = count_cpu_limit(day1_cpu_usage,62);
	long long int mem_limit = count_mem_limit(day1_mem_usage,62);

	//select best container size
	choose_size(cpu_limit,mem_limit,day1_cpu_usage,day1_mem_usage,62,day1_result);
	//choose_size(cpu_limit,mem_limit,day2_cpu_usage,day2_mem_usage,68,day2_result);
	//print_record();
	
	return 0;  
}
