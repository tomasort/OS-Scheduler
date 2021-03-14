#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <vector>
#include <unistd.h>
#include <list>
#include <iomanip>

#define CURRENT_TIME = 10000
#define QUANTUM = 10000 //TODO: There is actually no default quantum 
int num_rnums;
int maxprio = 4;
int* randvals = NULL;

static const std::string StateStrings[] = {"CREATED", "READY", "RUNNG", "BLOCK", "PREEMPTED", "DONE"};
int aflag = 0;
int tflag = 0;
int vflag = 0;
int eflag = 0;
typedef enum{
    STATE_CREATED,
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
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
    if (aflag)
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
    int wait_time;
    int current_ts;
    int time_in_current_state;
    int static_priority;
    int dynamic_priority;
    bool preempted;
    int finish_time;
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
        wait_time = 0;
        process_number = number_of_processes++;
        current_ts = arrival_time;
        time_in_current_state = 0;
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
        if (aflag)
            std::cout << "Cpu burst is: " << cb << std::endl;
        return cb; 
    }
    int get_io_burst(){
        int iob = myrandom(io_burst);
        total_io_time += iob;
        if (aflag)
            std::cout << "IO burst is: " << iob << std::endl;
        return iob; 
    }
    std::string to_string(){
        std::string process = "Process #" + std::to_string(process_number);
        process += std::to_string(total_cpu_time);
        process += " TC: " + std::to_string(total_cpu_time);
        process += " CB: " + std::to_string(cpu_burst);
        process += " IO: " + std::to_string(io_burst);
        process += " RemainingTime: " + std::to_string(remaining_cpu_time);
        return process;

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
    std::string to_string(){
        std::string event = "Event #" + std::to_string(event_number);
        event += " TimeStamp: " + std::to_string(timestamp);
        event += " Transition: " + std::to_string(transition);
        event += " Process: " + std::to_string(process->process_number);
        return event;
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
        std::cout << processes[i]->to_string() << std::endl;
    }
}

class Scheduler{
    // The scheduler class is a base class for the different schedulers 
public:
    std::string name;
    Process* current_running_process;
    std::deque<Process*> readyQ;
    int quantum;
    virtual void add_process(Process &p){
    }
    virtual Process* get_next_process(){
        return NULL;
    }
    virtual bool test_preemt(Process &p, int curtime, std::list<Event*> &event_queue){
        return false;
    }
    virtual void print_scheduler(){
        std::cout << "SCHED  (" << readyQ.size() << "):  ";
        for (int i = 0; i < readyQ.size(); i++){
            std::cout << readyQ[i]->process_number << ":" << readyQ[i]->current_ts << "  ";
        }
        std::cout << std::endl;
    }
    virtual std::string to_string(){
        return name;
    }
};

// There are 6 schedulers that we need to implement. 


// FCFS: First Come First Serve
class FCFS : public Scheduler{
public:
    FCFS(){
        name = "FCFS";
        quantum = 10000;
    }
    void add_process(Process &p) override{
        // Adds p to the end of the readyQueue 
        if (p.remaining_cpu_time <= 0){
            return;
        }
        readyQ.push_back(&p);
    }
    Process* get_next_process() override{
        if (aflag)
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
};


// LCFS: I'm guessing this one is Last Come First Served
class LCFS : public Scheduler{
public:
    LCFS(){
        name = "LCFS";
        quantum = 10000;
    }
    void add_process(Process &p) override{
        // Adds p to the begining of the readyQueue 
        if (p.remaining_cpu_time <= 0){
            return;
        }
        readyQ.push_front(&p);
    }
    Process* get_next_process() override{
        if (aflag)
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
};


// SRTF: Shortest Remaining Time First
class SRTF : public Scheduler{
    // choose the process whose remaining time is the shortest
    // This version is non-preemptive
    // Once the cpu-burst is issued, then we finish the process. 
public:
    SRTF(){
        name = "SRTF";
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
        if (aflag)
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
};


// RR: Round Robin
class RR : public Scheduler{
    // Take the cpu away from processes (preemption)
public:
    RR(int q){
        name = "RR";
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
    std::string to_string() override{
        return name + " " + std::to_string(quantum);
    }
};


class PRIO : public Scheduler{
    // Each process has a priority. We can assign a quantum that is longer or shorter based on that priority.
    // I think we ned to mantain two queues. 
    // std::queue<Process*> *activeQ = calloc(sizeof(std::queue<Process*>), MAX_PRIORITY);
    // std::queue<Process*> *expiredQ = calloc(sizeof(std::queue<Process*>), MAX_PRIORITY);
public:
    int maxprio;
    // MLFQ aka priority decay scheduler
    std::deque< std::list<Process*>* >* readyQ;
    std::deque< std::list<Process*>* >* expiredQ;
    std::deque< std::list<Process*>* >* temp;
    PRIO(int q, int mp){
        name = "PRIO";
        quantum = q;
        maxprio = mp;
        readyQ = new std::deque< std::list<Process*>* >[maxprio];
        expiredQ = new std::deque< std::list<Process*>* >[maxprio];
        for (int i = 0; i < maxprio; i++){
            readyQ->push_back(new std::list<Process*>);
            expiredQ->push_back(new std::list<Process*>);
        }
    }
    void add_process(Process &p) override{
        if (p.dynamic_priority < 0){
            p.dynamic_priority = p.static_priority-1;
            expiredQ->at(p.dynamic_priority)->push_back(&p);
            return;
        }
        readyQ->at(p.dynamic_priority)->push_back(&p);
    }

    bool emptyQ(){
        for (int i = 0; i < readyQ->size(); i++){
            if(readyQ->at(i)->empty()) continue;
            return false;
        }
        for (int i = 0; i < expiredQ->size(); i++){
            if(expiredQ->at(i)->empty()) continue;
            return false;
        }
        return true;
    }
    Process* get_next_process() override{
        std::list<Process*> *l;
        Process* next_process = NULL;
        for (int i = readyQ->size()-1; i >= 0; i--){
            l = readyQ->at(i);
            if (l->empty()) continue;
            next_process = l->front();
            l->pop_front();
            return next_process;
        }
        std::cout << "switched queues" << std::endl;
        temp = readyQ;
        readyQ = expiredQ;
        expiredQ = temp;
        for (int i = readyQ->size()-1; i >= 0; i--){
            l = readyQ->at(i);
            if (l->empty()) continue;
            next_process = l->front();
            l->pop_front();
            return next_process;
        }
        return next_process;
    }
    void print_scheduler() override{
        std::cout << "{ ";
        for (int i = readyQ->size()-1; i >= 0; i--){
            if(readyQ->at(i)->empty()){
                std::cout << "[]";
                continue;
            }
            std::cout << "[";
            std::list<Process*> *l = readyQ->at(i);
            for(std::list<Process*>::iterator it = l->begin(); it != l->end(); it++){
                std::cout << (*it)->process_number;
            }
            std::cout << "]";
            
        }
        std::cout << "}  :  ";
        std::cout << "{ ";
        for (int i = expiredQ->size()-1; i >= 0; i--){
            if(expiredQ->at(i)->empty()){
                std::cout << "[]";
                continue;
            }            
            std::cout << "[";
            std::list<Process*> *l = expiredQ->at(i);
            for(std::list<Process*>::iterator it = l->begin(); it != l->end(); it++){
                std::cout << (*it)->process_number;
                if (it != l->end()){
                    std::cout << ",";
                }
            }
            std::cout << "]";
        }
        std::cout << "}";
        std::cout << std::endl;
    }
    std::string to_string() override{
        return name + " " + std::to_string(quantum);
    }

};


class PREPRIO : public Scheduler{

public:
    int maxprio;
    std::deque< std::list<Process*>* >* readyQ;
    std::deque< std::list<Process*>* >* expiredQ;
    std::deque< std::list<Process*>* >* temp;
    PREPRIO(int q, int mp){
        name = "PREPRIO";
        quantum = q;
        maxprio = mp;
        readyQ = new std::deque< std::list<Process*>* >[maxprio];
        expiredQ = new std::deque< std::list<Process*>* >[maxprio];
        for (int i = 0; i < maxprio; i++){
            readyQ->push_back(new std::list<Process*>);
            expiredQ->push_back(new std::list<Process*>);
        }
    }

    void add_process(Process &p) override{
        if (p.dynamic_priority < 0){
            p.dynamic_priority = p.static_priority-1;
            expiredQ->at(p.dynamic_priority)->push_back(&p);
            return;
        }
        readyQ->at(p.dynamic_priority)->push_back(&p);
    }

    Process* get_next_process() override{
        std::list<Process*> *l;
        Process* next_process = NULL;
        for (int i = readyQ->size()-1; i >= 0; i--){
            l = readyQ->at(i);
            if (l->empty()) continue;
            next_process = l->front();
            l->pop_front();
            return next_process;
        }
        std::cout << "switched queues" << std::endl;
        temp = readyQ;
        readyQ = expiredQ;
        expiredQ = temp;
        for (int i = readyQ->size()-1; i >= 0; i--){
            l = readyQ->at(i);
            if (l->empty()) continue;
            next_process = l->front();
            l->pop_front();
            return next_process;
        }
        return next_process;
    }

    bool test_preemt(Process &p, int curtime, std::list<Event*> &event_queue) override{
        std::cout << "---> PRIO preemption";
        if (!current_running_process){
            std::cout << "---> NO (no current_process)" << std::endl;
            return false;
        }
        std::cout << p.process_number << " by " << current_running_process->process_number
        << " ? "; 
        if (p.dynamic_priority > current_running_process->dynamic_priority){
            for(std::list<Event*>::iterator it = event_queue.begin(); it != event_queue.end(); it++){
                if (((*it)->timestamp == curtime) && ((*it)->process == current_running_process)){
                    std::cout << "---> NO" << std::endl;
                    return false;
                }
            }
        }else{
            std::cout << "---> NO" << std::endl;
            return false;
        }
        for(std::list<Event*>::iterator it = event_queue.begin(); it != event_queue.end(); it++){
            if ((*it)->process == current_running_process){
                std::cout << " Removing " << (*it)->timestamp << ":" << (*it)->process->process_number << std::endl;
                event_queue.erase(it);
            }
        }
        std::cout << "---> YES" << std::endl;
        return true;

    }

    void print_scheduler() override{
        std::cout << "{ ";
        for (int i = readyQ->size()-1; i >= 0; i--){
            if(readyQ->at(i)->empty()){
                std::cout << "[]";
                continue;
            }
            std::cout << "[";
            std::list<Process*> *l = readyQ->at(i);
            for(std::list<Process*>::iterator it = l->begin(); it != l->end(); it++){
                std::cout << (*it)->process_number;
            }
            std::cout << "]";
            
        }
        std::cout << "}  :  ";
        std::cout << "{ ";
        for (int i = expiredQ->size()-1; i >= 0; i--){
            if(expiredQ->at(i)->empty()){
                std::cout << "[]";
                continue;
            }            
            std::cout << "[";
            std::list<Process*> *l = expiredQ->at(i);
            for(std::list<Process*>::iterator it = l->begin(); it != l->end(); it++){
                std::cout << (*it)->process_number;
                if (it != l->end()){
                    std::cout << ",";
                }
            }
            std::cout << "]";
        }
        std::cout << "}";
        std::cout << std::endl;
    }
    std::string to_string() override{
        return name + " " + std::to_string(quantum);
    }

};


class Simulation{
public:
    std::list<Event*> eventQ;
    Scheduler* scheduler;
    int current_time;
    float cpu_utilization; // percentage (0.0 – 100.0) of time at least one process is running
    float io_utilization;  // percentage (0.0 – 100.0) of time at least one process is performing IO
    float throughput;
    int idle_cpu_time = 0;
    int idle_io_time = 0;
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
    // void remove_events(Process *p){
    //     // Removes all events related to a process
    //     for(std::list<Event*>::iterator it = eventQ.begin(); it != eventQ.end(); it++){
    //         // If the event has p as a process
    //         if ((*it)->process == p)
    //             eventQ.erase(it);
    //             return;
    //         }
    //     }
    //     eventQ.push_back(e);
    // }

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
        std::cout << "ShowEventQ:  ";
        if (eventQ.empty()){
            std::cout << "empty" << std::endl;
        }
        for(std::list<Event*>::iterator it = eventQ.begin(); it != eventQ.end(); it++){
            std::cout << (*it)->timestamp << ":" << (*it)->process->process_number << "  ";
        }
        std::cout << std::endl;
    }

    void print_results(){
        std::cout << scheduler->to_string() << std::endl;
        float avg_turnaround = 0.0;
        float avg_wait_time = 0.0;
        float throughput = 0.0;
        for(int i=0; i < processes.size(); i++){
            Process* proc = processes[i];
            std::cout << std::setw(4) << std::setfill('0') << proc->process_number << ": ";
            std::cout << std::setfill(' ');

            std::cout << std::setw(4) << proc->arrival_time << " " << std::setw(4) << proc->total_cpu_time << " "
            << std::setw(4) << proc->cpu_burst << " " << std::setw(4) << proc->io_burst << " "
            << proc->static_priority ;
            std::cout << " | ";
            std::cout << std::setw(5) << proc->finish_time << " " << std::setw(5) << (proc->finish_time - proc->arrival_time) 
            << std::setw(5) << proc->total_io_time << std::setw(5) << proc->wait_time << std::endl;
            avg_turnaround += proc->finish_time - proc->arrival_time;
            avg_wait_time += proc->wait_time;
        }
        avg_turnaround /= processes.size();
        avg_wait_time /= processes.size();
        // Total sim time, CPU utilization, I/O Utilization, Avg Turnaround time, Avg wait time, Throughput
        std::cout << "SUM: " << current_time << " " 
        << std::setprecision(2) << std::fixed << (current_time - idle_cpu_time)*100.0/current_time << " "
        << std::setprecision(2) << std::fixed << (current_time - idle_io_time)*100.0/current_time << " " 
        << std::setprecision(2) << std::fixed << avg_turnaround << " " 
        << std::setprecision(2) << std::fixed << avg_wait_time << " " 
        << std::setprecision(3) << std::fixed << processes.size()/(current_time/100.0) << " " 
        << std::endl;
    }

    // void force_preemption(){
    //     if ()
    //     for(std::list<Event*>::iterator it = eventQ.begin(); it != eventQ.end(); it++){
    //         std::cout << (*it)->timestamp << ":" << (*it)->process->process_number << "  ";
    //     }
    // }


    void start_simulation(){
        Event* evt;
        bool CALL_SCHEDULER = false;
        int last_time_not_running = current_time;
        if(vflag)
            print_eventQueue();
        bool CPU_RUNNING = true;
        bool IO_RUNNING = true;
        bool used_full_quantum = true;
        int using_io = 0;
        int last_time_not_using_io = current_time;
        while((evt = get_event())){
            Process* proc = evt->process;  // This is the process the event is currently working on
            current_time = evt->timestamp;
            int io_burst;
            int cpu_burst;
            int new_timestamp;
            if (proc){
                proc->time_in_current_state = current_time - proc->current_ts;
                proc->current_ts = current_time;
                if (proc->state == STATE_READY || proc->state == STATE_PREEMPTED) 
                    proc->wait_time += proc->time_in_current_state;
                if (vflag && (evt->transition != TRANS_TO_DONE)){
                    std::cout << current_time << " " << proc->process_number << " " << proc->time_in_current_state 
                    << ": " << StateStrings[proc->state] << " -> ";
                }
            }
            switch(evt->transition){
                case TRANS_TO_READY:
                    // When a process is made ready (from blocked), set dynamic_priority to static_priority-1
                    // must come from BLOCKED or from PREEMTION or from CREATED
                    if (vflag)
                        std::cout << StateStrings[STATE_READY] << std::endl;
                    if (proc->state == STATE_BLOCKED){
                        using_io--;
                    }
                    if (using_io <= 0){
                        if (IO_RUNNING){
                            last_time_not_using_io = current_time;
                        }
                        using_io = 0;
                        IO_RUNNING = false;
                    }
                    if (scheduler->test_preemt(*proc, current_time, eventQ)){
                        put_event(new Event((*scheduler->current_running_process), current_time, TRANS_TO_PREEMPT));
                        std::cout << "Preemption prematurely" << std::endl;
                        used_full_quantum = false;
                        // put_event(new Event(*proc, current_time, TRANS_TO_RUN));
                        // break;
                    }
                    proc->state = STATE_READY;
                    scheduler->add_process(*proc);
                    CALL_SCHEDULER = true;
                    break;
                case TRANS_TO_RUN:
                    proc->state = STATE_RUNNING;
                    cpu_burst = proc->get_cpu_burst();
                    if (vflag)
                        std::cout << StateStrings[proc->state] << " "
                        << "cb=" << cpu_burst << " rem=" << proc->remaining_cpu_time 
                        << " prio=" << proc->dynamic_priority << std::endl;
                    process_transition_t transition;
                    new_timestamp = current_time; 
                    if (cpu_burst > scheduler->quantum){
                        transition = TRANS_TO_PREEMPT;
                        new_timestamp += scheduler->quantum;
                        proc->preempted = true;
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
                    scheduler->current_running_process = proc;
                    break;
                case TRANS_TO_BLOCK:
                    proc->state = STATE_BLOCKED;
                    // create an event for when process becomes READY again
                    // We need to generate an io burst
                    io_burst = proc->get_io_burst();
                    if (vflag)
                        std::cout << StateStrings[proc->state] << " "
                        << "ib=" << io_burst << " rem=" << proc->remaining_cpu_time << std::endl;
                    put_event(new Event(*proc, current_time + io_burst, TRANS_TO_READY));
                    if (using_io <= 0){
                        IO_RUNNING = true;
                        // std::cout << "Something started using io" << std::endl;
                        // std::cout << "Nothing was using IO since: " << last_time_not_using_io << std::endl;
                        // std::cout << "Adding " << current_time - last_time_not_using_io << std::endl;
                        idle_io_time += current_time - last_time_not_using_io;
                    }
                    using_io ++;
                    proc->dynamic_priority = proc->static_priority-1;
                    CALL_SCHEDULER = true;
                    scheduler->current_running_process = nullptr;
                    break;
                case TRANS_TO_PREEMPT:
                    proc->dynamic_priority--;
                    if (used_full_quantum){
                        proc->remaining_cpu_time = proc->remaining_cpu_time - scheduler->quantum;
                        std::cout << "Used full quantum when preempted" << std::endl;
                        proc->current_cpu_burst -= scheduler->quantum;
                    }else{
                        std::cout << "did not use full quantum when preempted" << std::endl;
                        if (!proc->preempted){
                            proc->remaining_cpu_time -= proc->time_in_current_state - proc->current_cpu_burst;
                            proc->preempted = true;
                        }else{
                            proc->remaining_cpu_time -= proc->time_in_current_state;
                        }
                        proc->current_cpu_burst -= proc->time_in_current_state;
                        used_full_quantum = true;
                    }
                    proc->state = STATE_PREEMPTED;
                    if (vflag){
                        std::cout << "PREEMPTED "
                        << "cb=" << proc->current_cpu_burst << " rem=" << proc->remaining_cpu_time << std::endl;
                    }
                    // add to runqueue (no event is generated)
                    scheduler->add_process(*proc);
                    CALL_SCHEDULER = true;
                    scheduler->current_running_process = nullptr;
                    break;
                case TRANS_TO_DONE:
                    proc->state = STATE_DONE;
                    proc->finish_time = current_time;
                    if (using_io <= 0){
                        // std::cout << "Nothing was using IO since: " << last_time_not_using_io << std::endl;
                        // std::cout << "Adding " << current_time - last_time_not_using_io << std::endl;
                        idle_io_time += current_time - last_time_not_using_io;
                        last_time_not_using_io = current_time;
                    }
                    if (vflag)
                        std::cout << current_time << " " << proc->process_number << " " << proc->time_in_current_state 
                        << ": Done" << std::endl;
                    CALL_SCHEDULER = true;
                    scheduler->current_running_process = nullptr;
                    break;
            }
            delete evt; 
            evt = nullptr;
            // std::cout << "Current time is: " << current_time << std::endl;
            if (scheduler->current_running_process == nullptr){
                if (CPU_RUNNING)
                    last_time_not_running = current_time;
                CPU_RUNNING = false;
                // std::cout << "Nothing Running since " << last_time_not_running << std::endl;
            }else{
                if (!CPU_RUNNING){
                    idle_cpu_time += current_time - last_time_not_running;
                    CPU_RUNNING = true;
                    // std::cout << "Something Started" << std::endl;
                }else{
                    // std::cout << "Something is running" << std::endl;
                }
            }
            if (CALL_SCHEDULER){
                if (get_next_event_time() == current_time){
                    continue;
                }
                CALL_SCHEDULER = false;
                if(scheduler->current_running_process == nullptr){
                    if (tflag)
                        scheduler->print_scheduler();
                    if (aflag)
                        std::cout << "Calling the scheduler " << std::endl;
                    proc = scheduler->get_next_process();
                    if (proc == nullptr){
                        if (aflag){
                            std::cout << "The scheduler returned nothing" << std::endl;
                            std::cout << "The current time is: " << current_time << std::endl;
                            print_eventQueue();
                        }
                        continue;
                    }
                    if (aflag){
                        std::cout << "The scheduler returned the next process wich is: " << proc->process_number <<  std::endl;
                    }
                    put_event(new Event(*proc, current_time, TRANS_TO_RUN));
                }
            }
            if (aflag){
                std::cout << "The current time is: " << current_time << std::endl;
                print_eventQueue();
            }
        }
    }
};

int main(int argc, char** argv){
    int c;
    char l;
    Scheduler* s;
    std::string usage = "<program> [-v] [-t] [-e][-s<schedspec>] inputfile randfile";
    int quantum;
    while ((c = getopt(argc, argv, "as:vte")) != -1){
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
                        s = new PRIO(quantum, maxprio);
                        break;
                    case 'E':
                        std::sscanf(optarg + 1, "%d:%d", &quantum, &maxprio);
                        s = new PREPRIO(quantum, maxprio);
                        break;
                }
                break;
            case 'v':
                vflag = 1;
                break;
            case 't':
                tflag = 1;
                break;
            case 'e':
                eflag = 1;
                break;
            case 'a':
                aflag = 1;
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
    Simulation sim(s);
    sim.start_simulation();
    sim.print_results();
    return 0;
}