#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <vector>
#include <unistd.h>
#include <list>


// TODO: Figure out how to print the required statistics.

#define CURRENT_TIME = 10000
#define QUANTUM = 10000 //TODO: There is actually no default quantum 
int num_rnums;
int maxprio = 4;
int* randvals = NULL;

typedef enum{
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_READY,
    STATE_CREATED,
    STATE_PREEMPTED,
    STATE_DONE
} process_state_t;

typedef enum{
    TRANS_TO_READY,
    TRANS_TO_RUN,
    TRANS_TO_BLOCK,
    TRANS_TO_PREEMPT, 
    TRANS_TO_DONE,
} process_transition_t;

int myrandom(int burst){
    static int ofs = 0;
    if (ofs >= num_rnums){ // We wrap around if we get to the end
        ofs = 0;
    }
    int rand = 1 + (randvals[ofs++] % burst);
    std::cout << "Random: " << rand << std::endl;
    return rand;
}

void read_rfile(std::string file_name){
    std::ifstream file(file_name);
    file >> num_rnums;
    randvals = new int[num_rnums];
    for (int i = 0; i < num_rnums; i++){
        file >> randvals[i];
    }
}

class Process{
public:
    int arrival_time;
    int total_cpu_time;
    int cpu_burst;
    int io_burst;
    // variables for keeping track of everithing the process goes through
    static int number_of_processes;
    int process_number;
    int total_io_time;
    int remaining_cpu_time;
    int current_cpu_burst;
    int static_priority;
    int dynamic_priority;
    bool preempted;
    process_state_t state;
    // Each process has at least 4 parameters
    Process(int at, int tc, int cb, int io){
        arrival_time = at;
        total_cpu_time = tc;
        remaining_cpu_time = tc;
        cpu_burst = cb;
        io_burst = io;
        // The order in which call myrandom matters a lot!
        static_priority = myrandom(maxprio);
        dynamic_priority = static_priority - 1;
        total_io_time = 0;
        state = STATE_CREATED;
        preempted = false;
        process_number = number_of_processes++;
    }
    int get_cpu_burst(){
        if (preempted){
            return current_cpu_burst;
        }
        int cb = myrandom(cpu_burst);
        if (cb > remaining_cpu_time){
            cb = remaining_cpu_time;
        }
        current_cpu_burst = cb;
        std::cout << "Cpu burst is: " << cb << std::endl;
        return cb; 
    }
    int get_io_burst(){
        int iob = myrandom(io_burst);
        total_io_time += iob;
        std::cout << "IO burst is: " << iob << std::endl;
        return iob; 
    }
    void print_process(){
        std::cout << "Process #" << process_number << " "
        << "AT: " << arrival_time << " "
        << "TC: " << total_cpu_time << " "
        << "CB: " << cpu_burst << " "
        << "IO: " << io_burst << " " 
        << "Remaining time: " << remaining_cpu_time
        << std::endl; 
    }
};
int Process::number_of_processes = 0;

class Event{
public:
    static int number_of_events;
    int event_number;
    int timestamp;
    Process* process;
    process_state_t old_state;
    process_state_t new_state;
    process_transition_t transition;
    Event(Process &p, int ts, process_transition_t transition_to){
        process = &p;
        timestamp = ts;
        transition = transition_to;
        event_number = number_of_events++;
    }
    void print_event(){
        std::cout << "Event " << event_number << " "
        << "Timestamp: " << timestamp << " "
        << "Transition: " << transition << " "
        << "Process: " << process->process_number << std::endl;
    }
};
int Event::number_of_events = 0;

std::vector<Process*> processes;

void read_processes(std::string file_name){
    int x, y, z, u;
    std::ifstream file(file_name);
    while (file >> x >> y >> z >> u){
        processes.push_back(new Process(x, y, z, u));

    }
}

void print_processes(){
    std::cout << "Printing Processes: " << std::endl;
    for(int i=0; i < processes.size(); i++){
        processes[i]->print_process();
    }
}

class Scheduler{
    // The scheduler class is a base class for the different schedulers 
private:
public:
    int quantum;
    virtual void add_process(Process &p){
    }
    virtual Process* get_next_process(){
        return NULL;
    }
    virtual void test_preemt(Process &p, int curtime){
    }
};

// There are 6 schedulers that we need to implement. 


// FCFS: First Come First Serve
class FCFS : public Scheduler{
private:
    std::deque<Process*> readyQ;
public:
    FCFS(){
        quantum = 10000;
    }
    void add_process(Process &p) override{
        // Adds p to the end of the readyQueue 
        std::cout << "Adding Process ";
        p.print_process();
        if (p.remaining_cpu_time <= 0){
            return;
        }
        readyQ.push_back(&p);
    }
    Process* get_next_process() override{
        std::cout << "Getting Next process" << std::endl;
        if (readyQ.empty()){
            return NULL;
        }
        Process* next_process;
        do{
            next_process = readyQ.front();
            readyQ.pop_front();
        }while (next_process->remaining_cpu_time <= 0);
        return next_process;
    }
    void test_preemt(Process &p, int curtime) override{
        return;
    }

};


// LCFS: I'm guessing this one is Last Come First Served
class LCFS : public Scheduler{
private:
    std::deque<Process*> readyQ;
public:
    LCFS(){
        std::cout << "Creating LCFS" << std::endl;
        quantum = 10000;
    }
    void add_process(Process &p) override{
        // Adds p to the begining of the readyQueue 
        std::cout << "Adding Process ";
        p.print_process();
        if (p.remaining_cpu_time <= 0){
            return;
        }
        readyQ.push_front(&p);
    }
    Process* get_next_process() override{
        std::cout << "Getting Next process" << std::endl;
        if (readyQ.empty()){
            return NULL;
        }
        Process* next_process;
        do{
            next_process = readyQ.front();
            readyQ.pop_front();
        }while (next_process->remaining_cpu_time <= 0);
        return next_process;
    }
    void test_preemt(Process &p, int curtime) override{
    }


};


// SRTF: Shortest Remaining Time First
class SRTF : public Scheduler{
    // choose the process whose remaining time is the shortest
    // This version is non-preemptive
    // Once the cpu-burst is issued, then we finish the process. 
private:
    std::deque<Process*> readyQ;
public:
    SRTF(){
        quantum = 10000;
    }
    void add_process(Process &p) override{
        // Add the process where it is supposed to be according to the remaining time
        for(std::deque<Process*>::iterator it = readyQ.begin(); it != readyQ.end(); it++){
            if (p.remaining_cpu_time < (*it)->remaining_cpu_time){
                readyQ.insert(it, &p);
                return;
            }
        }
        readyQ.push_back(&p);
    }
    Process* get_next_process() override{
        std::cout << "Getting Next process" << std::endl;
        if (readyQ.empty()){
            return NULL;
        }
        Process* next_process;
        do{
            next_process = readyQ.front();
            readyQ.pop_front();
        }while (next_process->remaining_cpu_time <= 0);
        return next_process;
        return NULL;
    }
    void test_preemt(Process &p, int curtime) override{
    }

};


// RR: Round Robin
class RR : public Scheduler{
    // Take the cpu away from processes (preemption)
private:
    std::deque<Process*> readyQ;
public:
    RR(int q){
        quantum = q;
    }
    void add_process(Process &p) override{
        // Adds p to the end of the readyQueue 
        if (p.remaining_cpu_time <= 0){
            return;
        }
        readyQ.push_back(&p);
    }
    Process* get_next_process() override{
        if (readyQ.empty()){
            return NULL;
        }
        Process* next_process;
        do{
            next_process = readyQ.front();
            readyQ.pop_front();
        }while (next_process->remaining_cpu_time <= 0);
        return next_process;
    }
    void test_preemt(Process &p, int curtime) override{
    }


};


// TODO: PRIO: Priority Scheduler 
class PRIO : public Scheduler{
    // Each process has a priority. We can assign a quantum that is longer or shorter based on that priority.
    // I think we ned to mantain two queues. 
    // std::queue<Process*> *activeQ = calloc(sizeof(std::queue<Process*>), MAX_PRIORITY);
    // std::queue<Process*> *expiredQ = calloc(sizeof(std::queue<Process*>), MAX_PRIORITY);
public:
    void add_process(Process &p) override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process &p, int curtime) override{
    }

};


// TODO: PREPRIO: PREemptive PRIO
class PREPRIO : public Scheduler{

public:
    void add_process(Process &p) override{
    }
    Process* get_next_process() override{
        return NULL;
    }
    void test_preemt(Process &p, int curtime) override{
    }

};


class Simulation{
public:
    std::list<Event*> eventQ;
    Scheduler* scheduler;
    int current_time;
    Simulation(Scheduler* s){
        scheduler = s;
        current_time = 0;
        // Add the processes to the event Q as they are created
        for (int i = 0; i < processes.size(); i++){
            // We set the timestamp of the events equal to the arrival time of the process
            eventQ.push_back(new Event(*processes[i], processes[i]->arrival_time, TRANS_TO_READY));
        }
    }

    void put_event(Event* e){
        // We need to put the event in its correct position in the eventQ
        for(std::list<Event*>::iterator it = eventQ.begin(); it != eventQ.end(); it++){
            if ((*e).timestamp < (*it)->timestamp){
                eventQ.insert(it, e);
                return;
            }
        }
        eventQ.push_back(e);
    }

    Event* get_event(){
        if(eventQ.empty()){
            return NULL;
        }
        Event* evt = eventQ.front();
        eventQ.pop_front();
        return evt;
    }

    int get_next_event_time(){
        if (eventQ.empty()){
            return -1; // return an invalid timestamp
        }
        int time = eventQ.front()->timestamp;
        return time;
    }

    void print_eventQueue(){
        int i = 0;
        std::cout << "The event Queue is: " << std::endl;
        if (eventQ.empty()){
            std::cout << "\t empty" << std::endl;
        }
        for(std::list<Event*>::iterator it = eventQ.begin(); it != eventQ.end(); it++){
            std::cout << "\t";
            (*it)->print_event();
        }
    }

    void start_simulation(){
        std::cout << "Starting the simulation" << std::endl;
        Event* evt;
        bool CALL_SCHEDULER = false;
        Process* current_process = nullptr;
        while((evt = get_event())){
            Process* proc = evt->process;  // This is the process the event is currently working on
            current_time = evt->timestamp;
            int io_burst;
            int cpu_burst;
            int new_timestamp;
            switch(evt->transition){
                case TRANS_TO_READY:
                    // When a process is made ready (from blocked), set dynamic_priority to static_priority-1
                    // must come from BLOCKED or from PREEMTION or from CREATED
                    std::cout << "Transitioning Process to ready ";
                    proc->print_process();
                    scheduler->add_process(*proc);
                    CALL_SCHEDULER = true;
                    break;
                case TRANS_TO_RUN:
                    std::cout << "Transitioning Process to run ";
                    proc->print_process();
                    cpu_burst = proc->get_cpu_burst();
                    process_transition_t transition;
                    new_timestamp = current_time; 
                    if (cpu_burst > scheduler->quantum){
                        transition = TRANS_TO_PREEMPT;
                        new_timestamp += scheduler->quantum;
                        proc->remaining_cpu_time = proc->remaining_cpu_time - scheduler->quantum;
                        proc->preempted = true;
                        proc->current_cpu_burst -= scheduler->quantum;
                    }else{  
                        proc->preempted = false;
                        proc->remaining_cpu_time = proc->remaining_cpu_time - cpu_burst;
                        if (proc->remaining_cpu_time <= 0){
                            transition = TRANS_TO_DONE;
                        }else{
                            transition = TRANS_TO_BLOCK;
                        }
                        new_timestamp += cpu_burst;
                    }
                    put_event(new Event(*proc, new_timestamp, transition));
                    current_process = proc;
                    break;
                case TRANS_TO_BLOCK:
                    std::cout << "Transitioning Process to block ";
                    proc->print_process();
                    // create an event for when process becomes READY again
                    // We need to generate an io burst
                    io_burst = proc->get_io_burst();
                    put_event(new Event(*proc, current_time + io_burst, TRANS_TO_READY));
                    CALL_SCHEDULER = true;
                    current_process = nullptr;
                    break;
                case TRANS_TO_PREEMPT:
                    // When a process has to be preempted, the dynamic priority is decremented. (dynamic_priority--)
                    // When the dynamic priority reaches -1, then reset it to static_priority-1
                    std::cout << "Transitioning Process to preempt ";
                    proc->print_process();
                    // add to runqueue (no event is generated)
                    scheduler->add_process(*proc);
                    CALL_SCHEDULER = true;
                    current_process = nullptr;
                    break;
                case TRANS_TO_DONE:
                    std::cout << "Transitioning Process to Done ";
                    proc->print_process();
                    CALL_SCHEDULER = true;
                    current_process = nullptr;
                    break;
            }
            delete evt; 
            evt = nullptr;
            if (CALL_SCHEDULER){
                if (get_next_event_time() == current_time){
                    std::cout << "the current time is: " << current_time << std::endl;
                    print_eventQueue();
                    continue;
                }
                CALL_SCHEDULER = false;
                if(current_process == nullptr){
                    std::cout << "Calling the scheduler " << std::endl;
                    proc = scheduler->get_next_process();
                    if (proc == nullptr){
                        std::cout << "The scheduler returned nothing" << std::endl;
                        std::cout << "The current time is: " << current_time << std::endl;
                        print_eventQueue();
                        continue;
                    }
                    std::cout << "The scheduler returned the next process wich is: ";
                    proc->print_process();
                    put_event(new Event(*proc, current_time, TRANS_TO_RUN));
                }
            }
            std::cout << "The current time is: " << current_time << std::endl;
            print_eventQueue();
        }
    }
};

int main(int argc, char** argv){
    int c;
    char l;
    Scheduler* s;
    std::string usage = "<program> [-v] [-t] [-e][-s<schedspec>] inputfile randfile";
    int quantum;
    while ((c = getopt(argc, argv, "s:vte")) != -1){
        switch(c){
            case 's':
                l = optarg[0];
                switch(l){
                    case 'F':
                        s = new FCFS();
                        break;
                    case 'L':
                        s = new LCFS();
                        break;
                    case 'S':
                        s = new SRTF();
                        break;
                    case 'R':
                        std:sscanf(optarg+1, "%d", &quantum);
                        s = new RR(quantum);
                        break;
                    case 'P':
                        std::sscanf(optarg + 1, "%d:%d", &quantum, &maxprio);
                        s = new PRIO();
                        break;
                    case 'E':
                        std::sscanf(optarg + 1, "%d:%d", &quantum, &maxprio);
                        s = new PREPRIO();
                        break;
                }
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
    Simulation sim(s);
    sim.start_simulation();
    return 0;
}