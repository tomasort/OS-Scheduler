#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <vector>
#include <unistd.h>

#define CURRENT_TIME = 10000
#define QUANTUM = 10000
#define MAX_PRIORITY = 10;
int* randvals = NULL;

typedef enum{
    STATE_RUNNING,
    STATE_BLOCKED,
} process_state_t;

int myrandom(int burst){
    static int ofs = 0;
    return 1 + (randvals[ofs++] % burst);
}

void read_rfile(std::string file_name){
    std::ifstream file(file_name);
    int size;
    file >> size;
    randvals = new int[size];
    for (int i = 0; i < size; i++){
        file >> randvals[i];
    }
}

class Event{

};

class Process{
    // Holds a dynamic priority and a static priority?
    // When a process has to be preempted, the dynamic priority is decremented. (dynamic_priority--)
    // When the dynamic priority reaches -1, then reset it to static_priority-1
    // When a process is made ready (from blocked), set dynamic_priority to static_priority-1
public:
    int arrival_time;
    int total_cpu_time;
    int cpu_burst;
    int io_burst;
    // Each process has at least 4 parameters
    Process(int at, int tc, int cb, int io){
        arrival_time = at;
        total_cpu_time = tc;
        cpu_burst = cb;
        io_burst = io;
    }
};

std::vector<Process*> processes;

void read_processes(std::string file_name){
    int x, y, z, u;
    std::ifstream file(file_name);
    while (file >> x >> y >> z >> u){
        processes.push_back(new Process(x, y, z, u));
    }
}

void print_processes(){
    for(int i=0; i < processes.size(); i++){
        std::cout << "AT: " << processes[i]->arrival_time << " ";
        std::cout << "TC: " << processes[i]->total_cpu_time << " ";
        std::cout << "CB: " << processes[i]->cpu_burst << " ";
        std::cout << "IO: " << processes[i]->io_burst << std::endl;
    }
}

class Scheduler{
    // The scheduler class is a base class for the different schedulers 
    // TODO: Assign static_priority using myrandom(maxprio)
    // TODO: The scheduler maintains a ReadyQueue where all processes that are ready to run are maintained. 
    // I think the base class should have the Quantum value 
public:
    virtual void add_process(){
    }
    virtual Process* get_next_process(){
        return NULL;
    }
    virtual void test_preemt(Process *p, int curtime){
    }
};

// There are 6 schedulers that we need to implement. 

// TODO: FCFS: First Come First Serve
class FCFS : public Scheduler{
public:
    void add_process() override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process *p, int curtime) override{
    }

};
// TODO: LCFS: I'm guessing this one is Last Come First Served
class LCFS : public Scheduler{
public:
    void add_process() override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process *p, int curtime) override{
    }


};
// TODO: SRTF: Shortest Remaining Time First
class SRTF : public Scheduler{
    // choose the process whose remaining time is the shortest
    // This version is non-preemptive
    // Once the cpu-burst is issued, then we finish the process. 
public:
    void add_process() override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process *p, int curtime) override{
    }

};
// TODO: RR: Round Robin
class RR : public Scheduler{
    // Take the cpu away from processes (preemption)
public:
    void add_process() override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process *p, int curtime) override{
    }


};
// TODO: PRIO: Priority Scheduler 
class PRIO : public Scheduler{
    // Each process has a priority. We can assign a quantum that is longer or shorter based on that priority.
    // I think we ned to mantain two queues. 
    // std::queue<Process*> *activeQ = calloc(sizeof(std::queue<Process*>), MAX_PRIORITY);
    // std::queue<Process*> *expiredQ = calloc(sizeof(std::queue<Process*>), MAX_PRIORITY);
public:
    void add_process() override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process *p, int curtime) override{
    }

};
// TODO: PREPRIO: PREemptive PRIO
class PREPRIO : public Scheduler{

public:
    void add_process() override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process *p, int curtime) override{
    }

};

// TODO: Implement the simulation code. It is probably better as a class
void simulation(){
}

int main(int argc, char** argv){
    int c;
    Scheduler s;
    std::string usage = "<program> [-v] [-t] [-e][-s<schedspec>] inputfile randfile";
    while ((c = getopt(argc, argv, "s:vte")) != -1){
        switch(c){
            case 's':
                std::cout << "This is the argument for s is " << optarg << std::endl;
                break;
            case 'v':
                break;
            case 't':
                break;
            case 'e':
                break;
        }
    }
    if (optind+1 > argc){
        std::cout << "There is not input file" << std::endl;
        std::cout << usage << std::endl;
        exit(0);
    }
    if (optind+2 > argc){
        std::cout << "There is not random number file" << std::endl;
        std::cout << usage << std::endl;
        exit(0);
    }
    std::string input_name = argv[optind];
    std::string random_file = argv[optind+1];
    read_rfile(random_file);
    read_processes(input_name);
    print_processes();
    return 0;
}