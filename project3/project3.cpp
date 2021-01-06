#include <iostream>
#include <string.h>
#include <cstdio>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <tuple>
#include <queue>
#include <vector>
#include <set>
#include <map>
#include <deque>
#include <string>
#define rr_timequantum 10
using namespace std;
int Sched_Flag = 0, page_flag = 0;
char *DIR;
int totalEventNum, vmemSize, pmemSize, pageSize; 
char P_NAME[1000][21];// 나중에 동적할당으로 바꿔야 할듯
queue<tuple<int, int, int> > event_queue;

//for memory
queue<tuple<int, int, int, int, int, int>> schedule;
deque<int> access_list;

int P_NUM =0;
void input(int argc, char* argv[]);

// Scheduling 변수
int Running_Process =-1;// -1이란 뜻은 아무것도 돌지 않는다는 뜻
int process_cycle=-1;
int Sched_PID = -1;
int cycle =1;// clock 시작이 1이므로


void alterRunningQueue(int pid);
void SJFRunQueuePop(deque<int> & RunQueue, map<int, tuple<float, int,int> > & PredictCpuBurst);
void PredictSched(int pid, priority_queue<pair<float, int>>& SJFRunQueue, vector<pair<float, int> >& PredictCpuBurst);
int SJFRunQueueTop(deque<int>  RunQueue, map<int, tuple<float, int,int> > PredictCpuBurst);
void PredictSched(int pid, deque<int> & RunQueue, map<int, tuple<float, int,int> > & PredictCpuBurst);
void PageAlloc(int * AIDTable,  int * VBitTable, int max, int page, int aid);
void PrintMem(FILE * memory_out, int * mem, int PageNum);
void PrintVBitMem(FILE * memory_out, int * mem, int at);
void PrintAllMem(FILE * memory_out, set<int,less<int>> VMList,int ** AIDTable, int ** VBitTable, int * PM,int VMPageNum, int PMPageNum);
int MemAlloc(vector<pair<int, int>>& Buddy, int *PM, int *AIDTable, int *VBitTable, int current, int AID, int PMPageNum, int VMPageNum, int page);
void PMDeAlloc(vector<pair<int, int>>& Buddy, int* PM, int current, int AID, int PMPageNum);
void MemReplace(vector<pair<int, int>>& Buddy, int *PM, int *AIDTable, int *VBitTable ,int AID, int PMPageNum, int VMPageNum);
void MemDeAlloc(vector<pair<int, int>>& Buddy, int *PM, int **AIDTable, int **VBitTable ,int AID, int PMPageNum, int VMPageNum);
// Memory allocation

void scheduling();
void memory();
int main(int argc, char* argv[]){
    input(argc, argv);
    scheduling();
    memory();
    delete [] DIR;
    return 0;
}

void input(int argc, char* argv[]){
    // input 받는 부분
    DIR = new char[1000];
    getcwd(DIR,1000); // 현재 위치 받기
    for(int i =0; i<argc;i++){
        char * token;
        token = strtok(argv[i], "-=");
        if(!strcmp(token,"sched")){
            token = strtok(NULL,"=");
            // 조건문 만들기 fcfs, rr, sjf-simple, sjf-exponential
            if(!strcmp(token,"rr")){
                Sched_Flag = 1;
            }else if(!strcmp(token,"sjf-simple")){
                Sched_Flag = 2;
            }else if(!strcmp(token,"sjf-exponential")){
                Sched_Flag = 3;
            }
        }else if(!strcmp(token,"page")){
            token = strtok(NULL,"=");
            // 조건문 만들기 fifo lru lru-sampled lfu mfu optimal
            if(!strcmp(token,"lru")){
                page_flag = 1;
            }else if(!strcmp(token,"lru-sampled")){
                page_flag = 2;
            }else if(!strcmp(token,"lfu")){
                page_flag = 3;
            }else if(!strcmp(token,"mfu")){
                page_flag = 4;
            }else if(!strcmp(token,"optimal")){
                page_flag = 5;
            }
        }else if(!strcmp(token,"dir")){// 절대경로
            DIR = strtok(NULL," ");
        }
    }
    strcat(DIR,"/");
    //이제 입력파일 읽어오는 부분
    char *input_DIR = new char[1000];
    strcpy(input_DIR,DIR);
    strcat(input_DIR, "input");
    FILE *fp = fopen(input_DIR, "r");
    if(fp == NULL){
        printf("파일 에러");
    }
    fscanf(fp, "%d\t%d\t%d\t%d ",&totalEventNum,&vmemSize,&pmemSize,&pageSize);
    for(int i=0;i<totalEventNum;i++){
        int time, num;
        char str[21];
        fscanf(fp, "%d\t%s",&time,str);
        if(!strcmp(str,"INPUT")){
            fscanf(fp, "%d", &num);
            event_queue.push(make_tuple(time,0,num));// 가운데꺼가 0이면 IO
        }else{
            // pid 하나가 생성,
            strcpy(P_NAME[P_NUM],str);
            event_queue.push(make_tuple(time,1,P_NUM++));// 가운데꺼가 1이면 process 생성
        }
    }
    fclose(fp);
    delete [] input_DIR;
}
//sjf 관련 함수
void alterRunningQueue(int pid){
    Running_Process = pid;
    process_cycle =0;
    Sched_PID = Running_Process;
}
void SJFRunQueuePop(deque<int> & RunQueue, map<int, tuple<float, int,int> > & PredictCpuBurst){
    int pid = RunQueue[0];
    float SJF_min = get<0>(PredictCpuBurst.find(pid)->second)-get<2>(PredictCpuBurst.find(pid)->second);
    // printf("cycle: %d\n", cycle);
    for(deque<int>::size_type i = 0; i < RunQueue.size();i++){
        // printf("pid: %d sjf-ex: %f\n",PredictCpuBurst.find(RunQueue[i])->first, get<0>(PredictCpuBurst.find(RunQueue[i])->second) - get<2>(PredictCpuBurst.find(RunQueue[i])->second)  );
        if(get<0>(PredictCpuBurst.find(RunQueue[i])->second) - get<2>(PredictCpuBurst.find(RunQueue[i])->second) < SJF_min){
            SJF_min = get<0>(PredictCpuBurst.find(RunQueue[i])->second);
            pid = RunQueue[i];
        }
    }
    for(deque<int>::size_type i = 0; i < RunQueue.size();i++){
        if( RunQueue[i] == pid){
            RunQueue.erase(RunQueue.begin()+i);
            break;
        }
    }
    alterRunningQueue(pid);
}
int SJFRunQueueTop(deque<int>  RunQueue, map<int, tuple<float, int,int> > PredictCpuBurst){
    int pid = -1;
    if(!RunQueue.empty()){
        pid = RunQueue[0];
    }
    float Min_cpu = get<0>(PredictCpuBurst.find(pid)->second);
    for(deque<int>::iterator iter = RunQueue.begin(); iter != RunQueue.end();iter++){
        if( get<0>(PredictCpuBurst.find(*iter)->second) < Min_cpu){
            Min_cpu = get<0>(PredictCpuBurst.find(*iter)->second);
            pid = PredictCpuBurst.find(*iter)->first;
        }
    }
    return pid;
}
void PredictSched(int pid, deque<int> & RunQueue, map<int, tuple<float, int,int> > & PredictCpuBurst){
    int  n_1, SJF_TOP = INT32_MAX, cpu_burst;
    if(!RunQueue.empty()){
        SJF_TOP= SJFRunQueueTop(RunQueue, PredictCpuBurst);
    }
    float Sn_1, Sn;
    Sn_1 = get<0>(PredictCpuBurst.find(Running_Process)->second);
    n_1 = get<1>(PredictCpuBurst.find(Running_Process)->second);
    cpu_burst = process_cycle + get<2>(PredictCpuBurst.find(Running_Process)->second);
    //IO 작업, Sleep, process 생성시 비교
    if(get<0>(PredictCpuBurst.find(pid)->second)< Sn_1 - cpu_burst){// cpu process 교체
        if(process_cycle >0){// 한 사이클이라도 돌았으면 PredictCpuBurst 갱신
            get<2>(PredictCpuBurst.find(Running_Process)->second) += process_cycle;
        }
        RunQueue.push_back(Running_Process);
        alterRunningQueue(pid);
        // 현재 프로세스 바뀌었으니 갱신
        Sn_1 = get<0>(PredictCpuBurst.find(Running_Process)->second);
        n_1 = get<1>(PredictCpuBurst.find(Running_Process)->second);
        cpu_burst = process_cycle + get<2>(PredictCpuBurst.find(Running_Process)->second);
    }else{ // cpu process 교체 X
        RunQueue.push_back(pid);
    }
    // Runqueue와 비교
    
    if(SJF_TOP != INT32_MAX && get<0>(PredictCpuBurst.find(SJF_TOP)->second) < Sn_1 - cpu_burst){// Runqueue와 비교시 process 교체
        if(process_cycle >0){ //한 사이클이라도 돌았으면 PredictCpuBurst 갱신
            get<2>(PredictCpuBurst.find(SJF_TOP)->second) += process_cycle;
        }
        RunQueue.push_back(Running_Process);
        alterRunningQueue(SJF_TOP);
    }
}




// page allocation
// 이 함수는 배열로 구현된 버디 시스템을 전회순회하면서 맞는 공간이 있는지 찾고, 공간이 있다면 할당하고 공간이 없다면 할당하지 않는다.
//  반환값은 공간을 할당했는지 여부이다.
void PageAlloc(int * AIDTable,  int * VBitTable, int max, int page, int aid){
    int count =0, at;
    int dp[max];
    if(AIDTable[0] != 0){
        dp[0] = 0;
    }else{
        dp[0] = 1;
    }
    for(int i = 1; i<max;i++){
        if(AIDTable[i] != 0){
            dp[i] = 0;
        }else{
            dp[i] = dp[i-1]+1;
        }
        if(dp[i] == page){
            at = i;
            break;
        }
    }
    for( int i = at -page+1; i<=at;i++){
        AIDTable[i] = aid;
        VBitTable[i] = 0;
    }
}
void PrintMem(FILE * memory_out, int * mem, int PageNum){
    for(int i= 0; i<PageNum;i++){
        if(mem[i] == 0)
            fprintf(memory_out,"-");
        else
            fprintf(memory_out,"%d",mem[i]);
        if(((i+1)%4)==0){
            fprintf(memory_out,"|");
        }
    }
    fprintf(memory_out,"\n");
}
void PrintVBitMem(FILE * memory_out, int * mem, int at){
    for(int i= 0; i<at;i++){
        if(mem[i] == -1)
            fprintf(memory_out,"-");
        else
            fprintf(memory_out,"%d",mem[i]);
        if(((i+1)%4)==0){
            fprintf(memory_out,"|");
        }
    }
    fprintf(memory_out,"\n");
}
void PrintAllMem(FILE * memory_out, set<int,less<int>> VMList,int ** AIDTable, int ** VBitTable, int * PM,int VMPageNum, int PMPageNum){
    fprintf(memory_out,">>Physical Memory :\t\t\t|");
    PrintMem(memory_out,PM,PMPageNum);
    for(set<int,less<int>>::iterator iter = VMList.begin(); iter != VMList.end(); iter++){
        fprintf(memory_out, "pid[%d] Page Table(AID) :\t|",*iter);
        PrintMem(memory_out, AIDTable[*iter],VMPageNum);
        fprintf(memory_out, "pid[%d] Page Table(VALID) :\t|",*iter);
        PrintVBitMem(memory_out,VBitTable[*iter], VMPageNum);
    }
    fprintf(memory_out,"\n");
}

int MemAlloc(vector<pair<int, int>>& Buddy, int *PM, int *AIDTable, int *VBitTable, int current, int AID, int PMPageNum, int VMPageNum, int page){// 공간에 할당할 수 있는 지 여부 true: 할당할 수 없음, false: 할당할 수 있음
    int temp = log2(current),  current_size =  PMPageNum/pow(2,temp), child_size = PMPageNum/pow(2,temp+1);
    int left = 2*current, right = 2*current+1, child_left, child_right;
    if(Buddy[current].first == current_size){
        return 0;
    }
    if(page <= child_size){
        if(Buddy[left].first >= Buddy[right].first){
            child_left = MemAlloc(Buddy, PM, AIDTable, VBitTable, left, AID, PMPageNum, VMPageNum, page);
            if(child_left != 0){
                return child_left;
            }
            child_right = MemAlloc(Buddy, PM, AIDTable, VBitTable, right, AID, PMPageNum, VMPageNum, page);
            return child_right;
        }else{
            child_right =  MemAlloc(Buddy, PM, AIDTable, VBitTable, right, AID, PMPageNum, VMPageNum, page);
            if(child_right != 0){
                return child_right;
            }
            child_left = MemAlloc(Buddy, PM, AIDTable, VBitTable, left, AID, PMPageNum, VMPageNum, page);
            return child_left;
        }
    }else if( page <= current_size && Buddy[current].first == 0){
        Buddy[current].first = current_size;
        Buddy[current].second = AID;
        int temp = (int)(current/2);
        while(temp >=1){
            Buddy[temp].first += current_size;
            temp /= 2;
        }
        for(int i=0; i<VMPageNum;i++){
            if(AIDTable[i] == AID){
                VBitTable[i] = 1;
            }
        }
        temp = current;
        while( temp < PMPageNum){
            temp *= 2;
        }
        int at = temp - PMPageNum;
        for(int i =at ; i< at+current_size; i++){
            PM[i] = AID;
        }
        return current_size;
    }
    return 0;
}

void PMDeAlloc(vector<pair<int, int>>& Buddy, int* PM, int current, int AID, int PMPageNum){
    int temp = log2(current),  current_size =  PMPageNum/pow(2,temp);
    int left = 2*current, right = 2*current+1;
    if(Buddy[current].second == AID){
        Buddy[current].first = 0;
        Buddy[current].second =0;
        int temp = (int(current/2));
        while(temp >=1){
            Buddy[temp].first -= current_size;
            temp /= 2;
        }
        for(int i =0; i<PMPageNum; i++){
            if(PM[i] == AID)
                PM[i] = 0;
        }
        return;
    }else if(current <PMPageNum){
        PMDeAlloc(Buddy, PM, left, AID, PMPageNum);
        PMDeAlloc(Buddy, PM, right, AID, PMPageNum);
    }
}

void MemReplace(vector<pair<int, int>>& Buddy, int *PM, int *AIDTable, int *VBitTable ,int AID, int PMPageNum, int VMPageNum){
    PMDeAlloc(Buddy, PM, 1, AID, PMPageNum);
    for(int i=0; i<VMPageNum;i++){
        if(AIDTable[i] == AID){
            VBitTable[i] = 0;
        }
    }
}

void MemDeAlloc(vector<pair<int, int>>& Buddy, int *PM, int **AIDTable, int **VBitTable ,int AID, int PMPageNum, int VMPageNum){
    PMDeAlloc(Buddy, PM, 1, AID, PMPageNum);
    for(int i=0; i<P_NUM;i++){
        for(int j = 0; j<VMPageNum; j++){
            if(AIDTable[i][j] == AID){
                AIDTable[i][j] =0;
                VBitTable[i][j] = -1;
            }
        }
    }
}
void scheduling(){
    deque<pair<int, int>> SleepList;// Arg1은 sleep이 끝나는 cycle수, Arg2은 pid 그 이유는 find로 cycle 수를 빨리 찾기 위해
    vector<pair<int,int>> PROGRAM_Line(P_NUM); // Arg1: 진행한 라인, Arg2: 총 명령어 수
    vector<vector<pair<int, int>>> PROGRAM_Opcode(P_NUM);
    deque<int> RunQueue; //FCFS 스케줄링일때 Arg1: pid 번호 
    set<int> IOWaitList;// Arg1은 pid
    set<int> IODoneList; // Arg1은 pid

    //sjf 구현을 위해 필요한 것
    map<int, tuple<float, int,int> > PredictCpuBurst;// Arg1 pid, Arg2 S[count], Arg3 count Arg4: 이때까지 못더한 cycle
    float Sn_1, Sn;
    int n_1;
    for(int i=0;i<P_NUM;i++){// PredictCpuBurst 초기화
        PredictCpuBurst.insert(make_pair(i,make_tuple(float(5),0,0)));
    }
    //LOCK 구현하기 위해 필요한 것
    map<int, int> LockList;// Lock을 구현하기 위해 만들었는데 이거저거 더 만들어야 할 거 같다.
    vector<vector<int> > BusyWaiting(P_NUM);
    vector<int> LockAllID; // 이중 배열 같은 느낌으로 
    for(int i=0;i<P_NUM;i++){
        BusyWaiting.push_back(LockAllID); 
    }
    // scheduler.txt 파일에 쓰기
    FILE *fp[P_NUM];
    char *Sched_DIR = new char[1000];
    strcpy(Sched_DIR,DIR);
    strcat(Sched_DIR,"scheduler.txt");
    FILE* scheduler_out = fopen(Sched_DIR,"w");
    char *Program_DIR =new char[1000];
    bool flag = false;
    // program 파일 먼저 열어두고, instruction 개수만 읽기
    for(int i=0;i<P_NUM;i++){
        strcpy(Program_DIR,DIR);
        strcat(Program_DIR,P_NAME[i]);
        fp[i] = fopen(Program_DIR,"r");
        if(fp[i] == NULL){
            printf("%d번재 파일 에러",i);
        }
        int num;
        fscanf(fp[i],"%d",&num);
        PROGRAM_Line[i].first = 0;
        PROGRAM_Line[i].second = num;
    }
    for (int i = 0; i< P_NUM; i++){
        int opcode, Arg;
        for(int j = 0; j< get<1>(PROGRAM_Line[i]); j++){
            fscanf(fp[i], "%d %d", &opcode, &Arg);
            PROGRAM_Opcode[i].push_back(make_pair(opcode, Arg));
        }
        fclose(fp[i]);
    }
    bool Sched_End = false; // scheduling하는 while이 끝났는지 안끝났는지 판단.
    while(!Sched_End){
    // while(cycle <47){ // whiletest 용
        Sched_PID = -1;
        int process_time = get<0>(event_queue.front());
        int process_status = get<1>(event_queue.front());
        int process_ID = get<2>(event_queue.front());
        int CreatePID = -1, DeadPID = -1; // memory 구현을 위한 변수
        if(Running_Process != -1){//프로그램이 종료될때를 위해 만든 if문
            int current_line = get<0>(PROGRAM_Line[Running_Process]);
            int max_line = get<1>(PROGRAM_Line[Running_Process]);
            if(current_line == max_line){// 프로그램이 종료될때
            // memory 관련
                DeadPID = Running_Process;
                PredictCpuBurst.erase(PredictCpuBurst.find(Running_Process));
                if(!RunQueue.empty()){
                    switch(Sched_Flag){
                        case 0: case 1:
                            alterRunningQueue(RunQueue.front());
                            RunQueue.pop_front();
                            break;
                        case 2: case 3:
                            SJFRunQueuePop(RunQueue, PredictCpuBurst);
                            break;
                    }
                }else{
                    Running_Process = -1;
                }
            }
        }
        //rr scheduler일시
        if(Sched_Flag ==1 && process_cycle >= rr_timequantum){
            RunQueue.push_back(Running_Process);
            alterRunningQueue(RunQueue.front());
            RunQueue.pop_front();
        }
        // sleep -> IO작업 시행 -> Process 생성
        // sleep에서 깨어났을 시 
        if(!SleepList.empty()){
            int  stack =0; // pid, at
            for(int  i =0 ; i<SleepList.size();i++){
                int pid = SleepList[i].second;
                if(SleepList[i].first == cycle){
                    stack++;
                    switch(Sched_Flag){
                        case 0: case 1:
                            RunQueue.push_back(pid);
                            break;
                        case 2: case 3:
                            if(Running_Process != -1){// 현재 프로세스가 있을 때
                                PredictSched(pid,RunQueue,PredictCpuBurst);
                            }else{// 현재 프로세스가 없을 시
                                int SJF_TOP = SJFRunQueueTop(RunQueue, PredictCpuBurst);
                                if(get<0>(PredictCpuBurst.find(SJF_TOP)->second) -get<2>(PredictCpuBurst.find(SJF_TOP)->second) < get<0>(PredictCpuBurst.find(pid)->second) - get<2>(PredictCpuBurst.find(pid)->second)){// Runqueue와 비교시 process 교체
                                    SJFRunQueuePop(RunQueue,PredictCpuBurst);
                                    RunQueue.push_back(pid);
                                }else
                                    alterRunningQueue(pid);
                            }
                            break;
                    }
                }
            }
            for(int i = 0; i<stack;i++){
                SleepList.pop_front();
            }
            
        }
        if(process_time == cycle ){  // IOwait 및 Process 생성 작업 시행
            event_queue.pop();
            if(process_status == 0){// IO wait
                set<int>::iterator iter = IOWaitList.find(process_ID);
                if(iter != IOWaitList.end()){// 찾고 있는 것이 있을 때
                    switch(Sched_Flag){
                        case 0: case 1:
                            IOWaitList.erase(process_ID);
                            RunQueue.push_back(process_ID);
                            break;
                        case 2: case 3: 
                            IOWaitList.erase(process_ID);
                            PredictSched(process_ID,RunQueue,PredictCpuBurst);
                            break;
                    }
                }else{// 찾고 있는 것이 없을 때
                    switch(Sched_Flag){
                        case 0: case 1:
                            IODoneList.insert(process_ID);
                            break;
                        case 2: case 3:
                            IODoneList.insert(process_ID);
                            break;
                    }
                }
            }else{// Process 생성
                CreatePID = process_ID;
                if(Running_Process == -1){
                    if(!RunQueue.empty()){
                        RunQueue.push_back(process_ID);
                    }else{
                        alterRunningQueue(process_ID);
                    }
                }else{// 현재 진행하는 프로세스가 있을 때 scheduler 작동
                    switch(Sched_Flag){
                        case 0: case 1: //fcfs, rr
                            RunQueue.push_back(process_ID);
                            break;
                        case 2: case 3: // simple sjf
                            PredictSched(process_ID,RunQueue,PredictCpuBurst);
                            break;
                    }
                }
            }
        }
        //running process가 없을시 runqueue에 앞에 있는 거 집어넣기
        if(Running_Process == -1){
            // 순서 sleep io 작업 process 생성 작업 시행, 
            switch(Sched_Flag){
                case 0: case 1:
                    if(!RunQueue.empty()){
                        int process_ID = RunQueue.front();
                        alterRunningQueue(process_ID);
                        RunQueue.pop_front();
                    }
                    break;
                case 2: case 3:
                    if(!RunQueue.empty()){
                        SJFRunQueuePop(RunQueue, PredictCpuBurst);
                    }
                    break;
            }
        }
        int opcode =-1, Arg= -1, line =PROGRAM_Line[Running_Process].first;        
        if(Running_Process != -1 && BusyWaiting[Running_Process].size() ==0){ // 프로그램 라인 읽기
        //busy waiting 일시도 추가해야할 듯
            opcode = PROGRAM_Opcode[Running_Process][line].first;
            Arg = PROGRAM_Opcode[Running_Process][line].second;
            PROGRAM_Line[Running_Process].first += 1;
            line = PROGRAM_Line[Running_Process].first;
        }
        
        fprintf(scheduler_out,"[%d cycle] Scheduled Process:",cycle);// Line 1
        if( Sched_PID != -1)// 스케줄링된 process 유뮤 확인
            fprintf(scheduler_out, "%d %s\n", Sched_PID , P_NAME[Running_Process]);
        else
            fprintf(scheduler_out, "None\n");

        fprintf(scheduler_out, "Running Process: ");// Line 2
        if( BusyWaiting[Running_Process].size() == 0 &&Running_Process != -1)
            fprintf(scheduler_out, "Process#%d running code %s line %d(op %d, arg %d)\n",Running_Process,P_NAME[Running_Process],line, opcode, Arg);
        else if(BusyWaiting[Running_Process].size() != 0 && Running_Process != -1){
            opcode = PROGRAM_Opcode[Running_Process][line].first;
            Arg = PROGRAM_Opcode[Running_Process][line].second;
            fprintf(scheduler_out, "Process#%d running code %s line %d(op %d, arg %d)\n",Running_Process,P_NAME[Running_Process],line, 6, BusyWaiting[Running_Process].back());
        }else
            fprintf(scheduler_out,"None\n");
        // cycle 수 더하기
        schedule.push(make_tuple(cycle,Running_Process,opcode,Arg,CreatePID, DeadPID));
        cycle++;
        if(Running_Process != -1)
            process_cycle++;
        switch (opcode)
        {
            case -1:
                break;
            case 0: case 2 : case3 ://memory allocation
                break;
            case 1:
                access_list.push_back(Arg);
                break;
            case 4: // 
            {
                float a =0.6;
                int at;
                SleepList.push_back(make_pair( (cycle) + Arg-1,Running_Process));
                sort(SleepList.begin(), SleepList.end());
                Sn_1 = get<0>(PredictCpuBurst.find(Running_Process)->second);
                float cpu_burst = (float)process_cycle + get<2>(PredictCpuBurst.find(Running_Process)->second);
                n_1 = get<1>(PredictCpuBurst.find(Running_Process)->second);
                if(Sched_Flag ==2){
                    Sn = (n_1 >0) ? (Sn_1*n_1+(float)cpu_burst)/(n_1+1) : cpu_burst;
                }else if(Sched_Flag == 3){
                    Sn = (n_1 >0) ? float(Sn_1*(1-a)+ (float)cpu_burst*a) : cpu_burst;
                }
                get<0>(PredictCpuBurst.find(Running_Process)->second)= Sn;
                get<1>(PredictCpuBurst.find(Running_Process)->second) += 1;
                get<2>(PredictCpuBurst.find(Running_Process)->second) = 0;
                Running_Process = -1;
                break;
            }
            case 5: // IOWAIT
            {
                float a =0.6;
                set<int>::iterator iter = IODoneList.find(Running_Process);
                if(iter != IODoneList.end()){
                    IODoneList.erase(Running_Process);
                }else{
                    IOWaitList.insert(Running_Process);
                    Sn_1 = get<0>(PredictCpuBurst.find(Running_Process)->second);
                    float cpu_burst = (float)process_cycle + get<2>(PredictCpuBurst.find(Running_Process)->second);
                    n_1 = get<1>(PredictCpuBurst.find(Running_Process)->second);
                    if(Sched_Flag == 2){
                        Sn = (n_1 >0) ? (Sn_1*n_1+cpu_burst)/(n_1+1) : cpu_burst;
                    }else if(Sched_Flag == 3){
                        Sn = (n_1 >0) ? float(Sn_1*(1-a)+ cpu_burst*a) : cpu_burst;
                    }
                    get<0>(PredictCpuBurst.find(Running_Process)->second) =Sn;
                    get<1>(PredictCpuBurst.find(Running_Process)->second) +=1;
                    get<2>(PredictCpuBurst.find(Running_Process)->second) = 0;
                    Running_Process = -1;
                }
                break;
            }
            case 6://lock
            {
                map<int,int>::iterator iter = LockList.find(Arg);
                if(iter != LockList.end()){// 다른 process에서 lock을 걸었을 때
                    // 이때 busy waiting 구현
                    // BusyWaiting[Running_Process] = Arg;
                    BusyWaiting[Running_Process].push_back(Arg);
                }else{// 다른 process에서 lock을 안했을 시
                    LockList.insert(make_pair(Arg,Running_Process));
                }
                break;
            }
            case 7: // unlock
                LockList.erase(Arg);
                vector<int>::iterator iter;
                for(int i =0;i<P_NUM;i++){
                    for(iter = BusyWaiting[i].begin();iter != BusyWaiting[i].end();iter++){
                        if(*iter == Arg){
                            BusyWaiting[i].erase(iter);
                        }
                    }
                }
                break;
        }
        fprintf(scheduler_out, "RunQueue: ");// Line3
        if(RunQueue.empty())
            fprintf(scheduler_out, "Empty\n");
        else{
            for(deque<int>::iterator iter = RunQueue.begin(); iter!=RunQueue.end();iter++){
                fprintf(scheduler_out,"%d(%s) ",*iter, P_NAME[*iter]);
            }
            fprintf(scheduler_out, "\n");
        }
        
        fprintf(scheduler_out, "SleepList: "); // Line 4
        if(SleepList.empty())
            fprintf(scheduler_out,"Empty\n");
        else{
            for(auto iter = SleepList.begin();iter != SleepList.end();iter++){
                int pid = iter->second;
                fprintf(scheduler_out, "%d(%s) ",pid,P_NAME[pid]);
            }
            fprintf(scheduler_out, "\n");
        }

        fprintf(scheduler_out, "IOWait List: "); // Line 5
        if (IOWaitList.empty())
            fprintf(scheduler_out, "Empty\n");
        else{
            for(set<int>::iterator iter = IOWaitList.begin(); iter!= IOWaitList.end(); ++iter ){
                int pid = *iter;
                fprintf(scheduler_out, "%d(%s)", pid, P_NAME[pid]);
            }
            fprintf(scheduler_out, "\n");
        }
        // 매 사이클이 끝나면 다음 사이클과 구분하기 위해 개행문자 필요
        fprintf(scheduler_out, "\n");
        //while문이 끝날지 안끝날지 판단
        Sched_End = true;
        for(int i=0;i<P_NUM;i++){
            int current_line = PROGRAM_Line[i].first;
            int max_line = PROGRAM_Line[i].second;
            if(current_line != max_line){
                Sched_End = false;
                break;
            }
        }
    }
    for(int i=0;i<P_NUM;i++){ // 파일 닫기
        fclose(fp[i]);
    }
    fclose(scheduler_out);
    delete [] Sched_DIR;
    delete [] Program_DIR;
}

void memory(){
     //Memory 구현 변수
    int pagefault =0;
    set<int, less<int> > VMList; // 어떤 process ID 메모리를 표현해야하는지 체크
    int **AIDTable =new int *[P_NUM];
    int **VBitTable = new int *[P_NUM];
    int *PM; 
    int VMPageNum = vmemSize/pageSize, PMPageNum = pmemSize/pageSize;
    vector<pair<int,int>> Buddy(2*PMPageNum);
    for(int i=0;i<P_NUM;i++){
        AIDTable[i] = new int[VMPageNum];
        VBitTable[i] = new int[VMPageNum]; 
    }
    PM = new int[PMPageNum]; // PMPageNum을 했다.
    int AID_NUM =0;
    vector<int > AIDPageCount;
    AIDPageCount.push_back(0);
    deque<tuple<int, int, int> > AllocedPage; // Arg1: AID, Arg2: PID, Arg3: 참조된 횟수 or reference bit(lru-sample일시) lru-sample용 
    set<int> AIDReference;
    //AIDTable,  PM 0으로 초기화, VBit은 -1로 초기화
    for(int i=0;i<P_NUM;i++){
        for(int j=0;j<VMPageNum;j++){
            AIDTable[i][j] =0;
            VBitTable[i][j] =-1;
        }
        
    }
    for(int i = 0; i<2*PMPageNum;i++){
        if(i<PMPageNum){
            PM[i] = 0;
        }
        Buddy[i].first = 0;
        Buddy[i].second = 0;
    }
    char *Mem_DIR = new char[1000];
    strcpy(Mem_DIR,DIR);
    strcat(Mem_DIR, "memory.txt");
    FILE* memory_out = fopen(Mem_DIR,"w");
    while(!schedule.empty()){
        int cycle = get<0>(schedule.front()), Running_Process = get<1>(schedule.front()), opcode = get<2>(schedule.front()), Arg = get<3>(schedule.front()), CreatePid = get<4>(schedule.front()), DeadPid = get<5>(schedule.front());
        if(DeadPid != -1){
            VMList.erase(DeadPid);
            for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size();){// 미리 pm에 할당되어 있는지 체크
                if(get<1>(AllocedPage[i]) == DeadPid){
                    int DeAllocAID = get<0>(AllocedPage[i]);
                    MemDeAlloc(Buddy, PM, AIDTable,VBitTable,DeAllocAID,PMPageNum,VMPageNum);
                    AllocedPage.erase(AllocedPage.begin() + i);
                }else{
                    i++;
                }
            }
        }
        if(CreatePid != -1){
            VMList.insert(CreatePid);
        }
        fprintf(memory_out, "[%d cycle] Input: Pid [%d] Function ",cycle, Running_Process);
        switch (opcode)
        {
            case -1:
                fprintf(memory_out,"[NO-OP]\n");
                break;
            case 0: //memory allocation
            {
                fprintf(memory_out,"[ALLOCATION] Alloc ID [%d] Page Num[%d]\n",++AID_NUM,Arg);
                AIDPageCount.push_back(Arg);
                PageAlloc(AIDTable[Running_Process],VBitTable[Running_Process], VMPageNum, Arg, AID_NUM);
                break;
            }
            case 1: // memory access
            {
                access_list.pop_front();
                // buddy system
                bool AIDExist = false;
                int AID =Arg;
                for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size();i++){// 미리 pm에 할당되어 있는지 체크
                    if(get<0>(AllocedPage[i]) == AID){
                        AIDExist = true;
                    }
                }
                if(!AIDExist){// PM에 AID가 존재하지않아 메모리 할당시켜야 할때
                    //can_alloc_fage는 page를 할당할만한 공간을 찾았을 때 위치도 기억한다.
                    pagefault++;
                    int can_alloc_flag = MemAlloc(Buddy, PM, AIDTable[Running_Process], VBitTable[Running_Process], 1, AID, PMPageNum, VMPageNum, AIDPageCount[AID]);
                    if(can_alloc_flag == 0) {// 페이지 교체 알고리즘
                        switch(page_flag){
                            case 0: case 1:// fifo, lru
                                while(1){
                                    int DeAllocAID = get<0>(AllocedPage.front()), DeAllocPID = get<1>(AllocedPage.front());
                                    MemReplace(Buddy, PM,AIDTable[DeAllocPID],VBitTable[DeAllocPID], DeAllocAID, PMPageNum, VMPageNum);
                                    AllocedPage.pop_front();
                                    can_alloc_flag = MemAlloc(Buddy, PM, AIDTable[Running_Process], VBitTable[Running_Process], 1, AID, PMPageNum, VMPageNum, AIDPageCount[AID]);
                                    if(can_alloc_flag != 0){
                                        break;
                                    }
                                    pagefault++;
                                }
                                break;
                            case 2: // lru-sample
                                while(1){
                                    int  at, DeAllocAID, DeAllocPID, target;
                                    deque<tuple<int,int,int>>temp = AllocedPage;
                                    // reference bit 있으면 체에 걸러진다.
                                    for(auto iter = AIDReference.begin(); iter != AIDReference.end();iter++){
                                        for(deque<tuple<int,int,int>>::size_type i =0; i <temp.size(); i++){
                                            if(*iter == get<0>(temp[i])){
                                                temp.erase(temp.begin() +i);
                                                break;
                                            }
                                        }
                                    }
                                    if(!temp.empty()){// temp에서 최소값을 구한다.
                                        target = get<2>(temp[0]);
                                        for(deque<tuple<int,int,int>>::size_type i=0; i< temp.size();i++){
                                            target = min(target, get<2>(temp[i]));
                                        }
                                        for(deque<tuple<int, int, int>>::size_type i=0; i< temp.size();i++){
                                            if(get<2>(temp[i]) == target){
                                                DeAllocAID = get<0>(temp[i]);
                                                DeAllocPID = get<1>(temp[i]);
                                            }
                                        }
                                        for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                            if(get<0>(AllocedPage[i]) == DeAllocAID){
                                                at = i;
                                                break;
                                            }
                                        }
                                        AllocedPage.erase(AllocedPage.begin()+at);
                                    }else{//AllocPage에서 reference bit의 최소값을 구한다.
                                        target = get<2>(AllocedPage[0]);
                                        for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                            target = min(target, get<2>(AllocedPage[i]));
                                        }
                                        for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                            if(get<2>(AllocedPage[i]) == target){
                                                at = i;
                                                DeAllocAID = get<0>(AllocedPage[i]);
                                                DeAllocPID = get<1>(AllocedPage[i]);
                                                break;
                                            }
                                        }
                                        AllocedPage.erase(AllocedPage.begin()+at);
                                    }
                                    MemReplace(Buddy, PM,AIDTable[DeAllocPID],VBitTable[DeAllocPID], DeAllocAID, PMPageNum, VMPageNum);
                                    can_alloc_flag = MemAlloc(Buddy, PM, AIDTable[Running_Process], VBitTable[Running_Process], 1, AID, PMPageNum, VMPageNum, AIDPageCount[AID]);
                                    if(can_alloc_flag != 0){
                                        break;
                                    }
                                    pagefault++;
                                }
                                break;
                            case 3: case 4: // lfu, mfu
                                while(1){
                                    int at, DeAllocAID, DeAllocPID, target = get<2>(AllocedPage.front());
                                    for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                        if(page_flag== 3){//lfu
                                            target = min(target, get<2>(AllocedPage[i]));
                                        }else{
                                            target = max(target, get<2>(AllocedPage[i]));
                                        }
                                    }
                                    for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                        if(get<2>(AllocedPage[i]) == target){
                                            at = i;
                                            DeAllocAID = get<0>(AllocedPage[i]);
                                            DeAllocPID = get<1>(AllocedPage[i]);
                                            break;
                                        }
                                    }
                                    AllocedPage.erase(AllocedPage.begin()+at);
                                    MemReplace(Buddy, PM,AIDTable[DeAllocPID],VBitTable[DeAllocPID], DeAllocAID, PMPageNum, VMPageNum);
                                    can_alloc_flag = MemAlloc(Buddy, PM, AIDTable[Running_Process], VBitTable[Running_Process], 1, AID, PMPageNum, VMPageNum, AIDPageCount[AID]);
                                    if(can_alloc_flag != 0){
                                        break;
                                    }
                                    pagefault++;
                                }
                                break;
                            case 5: // optimal
                                
                                while(1){
                                    int at = 0, DeAllocAID = get<0>(AllocedPage[0]) , DeAllocPID = get<1>(AllocedPage[0]), max_count = 0;
                                    for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                        int count = access_list.size();
                                        for(deque<int>::size_type j = 0 ; j < access_list.size(); j++){
                                            if( access_list[j] == get<0>(AllocedPage[i])){
                                                count = j;
                                                break;
                                            }
                                        }
                                        if( count > max_count ){
                                            max_count = count;
                                            DeAllocAID = get<0>(AllocedPage[i]);
                                            DeAllocPID = get<1>(AllocedPage[i]);
                                            at = i;
                                        }
                                    }
                                    AllocedPage.erase(AllocedPage.begin()+at);
                                    MemReplace(Buddy, PM,AIDTable[DeAllocPID],VBitTable[DeAllocPID], DeAllocAID, PMPageNum, VMPageNum);
                                    can_alloc_flag = MemAlloc(Buddy, PM, AIDTable[Running_Process], VBitTable[Running_Process], 1, AID, PMPageNum, VMPageNum, AIDPageCount[AID]);
                                    if(can_alloc_flag != 0){
                                        break;
                                    }
                                    pagefault++;
                                }
                                break;
                        }
                    }
                    if(page_flag != 2){
                        AllocedPage.push_back(make_tuple(AID,Running_Process,1));
                    }else{ // lru-sample
                        AIDReference.insert(AID);
                        AllocedPage.push_back(make_tuple(AID,Running_Process,0));
                    }
                    if(page_flag != 0 && page_flag != 1){
                        sort(AllocedPage.begin(),AllocedPage.end());
                    }
                }else{// page에 존재하지만 deque 순서 바꾸고 access count 늘려야 함
                    switch(page_flag){
                        case 1:  // lru, lfu, mfu
                            int at, PID, count;
                            for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                if(get<0>(AllocedPage[i]) == AID){
                                    at = i;
                                    PID = get<1>(AllocedPage[i]);
                                    count = get<2>(AllocedPage[i])+1;
                                    break;
                                }
                            }
                            AllocedPage.erase(AllocedPage.begin() + at);
                            AllocedPage.push_back(make_tuple(AID,PID,count));
                            break;
                        case 2: // lru-sample
                            // access 됬는지 체크
                            AIDReference.insert(AID);
                            break;
                        case 3: case 4:
                            for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size(); i++){
                                if(get<0>(AllocedPage[i]) == AID){
                                    get<2>(AllocedPage[i]) +=1;
                                    break;
                                }
                            }
                            break;
                        case 5: // optimal
                            break;
                    }
                }
                fprintf(memory_out,"[ACCESS] Alloc ID [%d] Page Num[%d]\n",AID,AIDPageCount[AID]);
                break;
            }
            case 2: // memory release
            {
                int DeAllocAID = Arg, DeAllocPID;
                for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size();i++){// 미리 pm에 할당되어 있는지 체크
                    if(get<0>(AllocedPage[i]) == DeAllocAID){
                        DeAllocPID = get<1>(AllocedPage[i]);
                        AllocedPage.erase(AllocedPage.begin() + i);
                    }
                }

                MemDeAlloc(Buddy, PM,AIDTable,VBitTable, DeAllocAID, PMPageNum, VMPageNum);
                fprintf(memory_out,"[RELEASE] Alloc ID [%d] Page Num[%d]\n",Arg,AIDPageCount[Arg]);
                break;
            }
            case 3: // Non-memory instruction
                fprintf(memory_out,"[NONMEMORY]\n");
                break;
            case 4: // 
                fprintf(memory_out,"[SLEEP]\n");
                break;
            case 5: // IOWAIT
                fprintf(memory_out,"[IOWAIT]\n");
                break;
            case 6://lock
                fprintf(memory_out,"[LOCK]\n");
                break;
            case 7: // unlock
                fprintf(memory_out,"[UNLOCK]\n");
                break;
        }
        //lru-sample
        if(page_flag == 2 && (cycle) %8 ==0){
            for(deque<tuple<int,int,int>>::size_type i =0; i <AllocedPage.size();i++){// 미리 pm에 할당되어 있는지 체크
                get<2>(AllocedPage[i]) /= 2;
                for(auto iter = AIDReference.begin(); iter != AIDReference.end();iter++){
                    if(get<0>(AllocedPage[i]) == *iter){
                        get<2>(AllocedPage[i]) += pow(2,7);
                    }
                }
            }
            AIDReference.clear();
        }

        //Showmemory
        PrintAllMem(memory_out,VMList,AIDTable,VBitTable,PM,VMPageNum,PMPageNum);
        schedule.pop();
    }
    for(int i=0;i<P_NUM;i++){
        delete [] AIDTable[i];
        delete [] VBitTable[i];
    }
    delete [] AIDTable;
    delete [] VBitTable;
    delete [] PM;
    PM = new int[PMPageNum]; // PMPageNum을 했다.
    fprintf(memory_out,"page fault: %d",pagefault);
    fclose(memory_out);
}