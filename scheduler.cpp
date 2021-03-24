#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <vector>
#include <unistd.h>
#include <list>
#include <iomanip>

#define QUANTUM = 1000000
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
        return cb; 
    }

    int get_io_burst(){
        int iob = myrandom(io_burst);
        total_io_time += iob;
        return iob; 
    }

    std::string to_string(){
        std::string process = "Process #" + std::to_string(process_number);
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
public:
    int maxprio;
    // MLFQ aka priority decay scheduler
    std::deque<std::list<Process*>* >* readyQ;
    std::deque<std::list<Process*>* >* expiredQ;

    std::deque<std::list<Process*>* >* temp;
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
        if (vflag) std::cout << "switched queues" << std::endl;
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
        if (vflag) std::cout << "switched queues" << std::endl;
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
        if (vflag) std::cout << "---> PRIO preemption";
        if (!current_running_process){
            if (vflag) std::cout << "---> NO (no current_process)" << std::endl;
            return false;
        }
        if (curtime == 66){
            int y = 0;
        }
        if (vflag) std::cout << p.process_number << " by " << current_running_process->process_number << " ? "; 
        if (p.dynamic_priority > current_running_process->dynamic_priority){
            if (event_queue.empty()) return true;
            for(std::list<Event*>::iterator it = event_queue.begin(); it != event_queue.end(); ++it){
                if (((*it)->timestamp == curtime) && ((*it)->process == current_running_process)){
                    if (vflag) std::cout << "---> NO" << std::endl;
                    return false;
                }
            }
        }else{
            if (vflag) std::cout << "---> NO" << std::endl;
            return false;
        }
        for(std::list<Event*>::iterator it = event_queue.begin(); it != event_queue.end(); ++it){
            if ((*it) && ((*it)->process == current_running_process)){
                if (vflag) std::cout << " Removing " << (*it)->timestamp << ":" << (*it)->process->process_number << std::endl;
                it = event_queue.erase(it);
            }
        }
        if (vflag) std::cout << "---> YES" << std::endl;
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
    double cpu_utilization; // percentage (0.0 – 100.0) of time at least one process is running
    double io_utilization;  // percentage (0.0 – 100.0) of time at least one process is performing IO
    double throughput;
    int io_time = 0;
    int cpu_time = 0;

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
        cpu_utilization = cpu_time*100.0/current_time;
        io_utilization = io_time*100.0/current_time;
        double avg_turnaround = 0.0;
        double avg_wait_time = 0.0;
        double throughput = 0.0;
        for(int i=0; i < processes.size(); i++){
            Process* proc = processes[i];
            std::cout << std::setw(4) << std::setfill('0') << proc->process_number << ": ";
            std::cout << std::setfill(' ');

            std::cout << std::setw(4) << proc->arrival_time << " " << std::setw(4) << proc->total_cpu_time << " "
            << std::setw(4) << proc->cpu_burst << " " << std::setw(4) << proc->io_burst << " "
            << proc->static_priority ;
            std::cout << " | ";
            std::cout << std::setw(5) << proc->finish_time << " " << std::setw(5) << (proc->finish_time - proc->arrival_time) 
            << std::setw(6) << proc->total_io_time << std::setw(6) << proc->wait_time << std::endl;
            avg_turnaround += proc->finish_time - proc->arrival_time;
            avg_wait_time += proc->wait_time;
        }
        avg_turnaround /= processes.size();
        avg_wait_time /= processes.size();
        // Total sim time, CPU utilization, I/O Utilization, Avg Turnaround time, Avg wait time, Throughput
        printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", current_time, cpu_utilization, io_utilization, avg_turnaround, avg_wait_time, processes.size()/(current_time/100.0));
    }

    void start_simulation(){
        if(vflag) print_eventQueue();
        Event* evt;
        bool CALL_SCHEDULER = false;
        bool used_full_quantum = false;
        // keep track of the time using cpu and io
        int USING_IO = 0;
        int using_io_ts = 0;
        bool CPU_RUNNING = false;
        int using_cpu_ts = 0;

        while((evt = get_event())){
            Process* proc = evt->process;  // This is the process the event is currently working on
            current_time = evt->timestamp;
            int io_burst;
            int cpu_burst;
            int next_timestamp;

            if (proc){ // keep track of the time in each state and the waittime for each process
                proc->time_in_current_state = current_time - proc->current_ts;
                proc->current_ts = current_time;
                // Keep track of the waittime for each process
                if (proc->state == STATE_READY || proc->state == STATE_PREEMPTED) 
                    proc->wait_time += proc->time_in_current_state;

                if (vflag && (evt->transition != TRANS_TO_DONE)) std::cout << current_time << " " << proc->process_number << " " << proc->time_in_current_state << ": " << StateStrings[proc->state] << " -> ";
            }
            switch(evt->transition){

                case TRANS_TO_READY: // create an event for a running process
                    if (proc->state == STATE_BLOCKED){
                        if (USING_IO == 1){
                            io_time += current_time - using_io_ts;
                        }
                        USING_IO--;
                    }
                    if (vflag) std::cout << StateStrings[STATE_READY] << std::endl;
                    // Here we can see where the process changed from blocked to ready (it finished IO?)
                    if (scheduler->test_preemt(*proc, current_time, eventQ)){
                        // if test_preemt returns true put the current running process into preempt
                        put_event(new Event((*scheduler->current_running_process), current_time, TRANS_TO_PREEMPT));
                        if (vflag) std::cout << "Preemption prematurely" << std::endl;
                        used_full_quantum = false; // set it to false because it was preempted
                    }
                    proc->state = STATE_READY;
                    scheduler->add_process(*proc);
                    CALL_SCHEDULER = true;
                    break;

                case TRANS_TO_RUN:
                    proc->state = STATE_RUNNING;
                    cpu_burst = proc->get_cpu_burst(); // get a cpu burst
                    if (vflag) std::cout << StateStrings[proc->state] << " " << "cb=" << cpu_burst << " rem=" << proc->remaining_cpu_time << " prio=" << proc->dynamic_priority << std::endl;
                    process_transition_t transition; // the transition that we want. it could be Preempt Done or blocked
                    next_timestamp = current_time; // we might have to add the quantum or the cpu_burst
                    if (cpu_burst > scheduler->quantum){ // add the full quantum to the timestamp of the event
                        // the quantum will be subtracted from the remaining cpu time in the TRANS_TO_PREEMPT transition
                        next_timestamp += scheduler->quantum;
                        transition = TRANS_TO_PREEMPT;
                        proc->preempted = true;
                    }else{  // add the cpu burst to the timestamp of the event
                        next_timestamp += cpu_burst;
                        proc->remaining_cpu_time -= cpu_burst;
                        transition = TRANS_TO_BLOCK;
                        if (proc->remaining_cpu_time <= 0) 
                            transition = TRANS_TO_DONE;
                        proc->preempted = false;
                    }
                    put_event(new Event(*proc, next_timestamp, transition));
                    scheduler->current_running_process = proc;
                    CPU_RUNNING = true;
                    using_cpu_ts = current_time;
                    break;

                case TRANS_TO_BLOCK: // Create an event for when the process is back to ready
                    cpu_time += current_time - using_cpu_ts;
                    proc->state = STATE_BLOCKED;
                    // We need to generate an io burst
                    io_burst = proc->get_io_burst();
                    if (vflag) std::cout << StateStrings[proc->state] << " " << "ib=" << io_burst << " rem=" << proc->remaining_cpu_time << std::endl;
                    // When a process is made ready (from blocked), set dynamic_priority to static_priority-1
                    proc->dynamic_priority = proc->static_priority-1;
                    // create an event for when process becomes READY again
                    put_event(new Event(*proc, current_time+io_burst, TRANS_TO_READY));
                    // say that there is no process running
                    scheduler->current_running_process = nullptr;
                    CALL_SCHEDULER = true;
                    if (USING_IO == 0){
                        using_io_ts = current_time;
                    }
                    USING_IO++;
                    break;

                case TRANS_TO_PREEMPT: // Create an event for the process that is ready to run
                    cpu_time += current_time - using_cpu_ts;
                    // decrement the priority when the process is preempted
                    proc->dynamic_priority--;
                    proc->state = STATE_PREEMPTED;
                    // determine if the process was able to use the full quantum or if it was preempted before it got to use the entire time
                    if (used_full_quantum){
                        if (vflag) std::cout << "Used full quantum when preempted" << std::endl;
                        // If we used the full quantum then we need to subtract the full quantum from the remaining time
                        proc->remaining_cpu_time -= scheduler->quantum;
                        // subtract the quantum from the current cpu burst (if there is some remaining it will be used in the next event)
                        proc->current_cpu_burst -= scheduler->quantum;
                    }else{ // If we did not use the full quantum then we need to see for how long we ran (this might happen if we are preempted or not)
                        if (vflag) std::cout << "Did not use full quantum when preempted" << std::endl;
                        // TODO: This might be wrong!
                        if (proc->preempted){ // If the process was preempted by another process we need to subtract only the time in the running state 
                            proc->remaining_cpu_time -= proc->time_in_current_state;
                        }else{
                            // We need to see how long we were in the RUN state and see how much cpu burst is left
                            // We need to add back the current_cpu_burst because we subtracted it before. 
                            proc->remaining_cpu_time -= proc->time_in_current_state - proc->current_cpu_burst;
                            proc->preempted = true;
                        }
                        proc->current_cpu_burst -= proc->time_in_current_state;
                        used_full_quantum = true;
                    }
                    if (vflag) std::cout << "PREEMPTED " << "cb=" << proc->current_cpu_burst << " rem=" << proc->remaining_cpu_time << std::endl;
                    // add to runqueue (no event is generated)
                    scheduler->add_process(*proc);
                    scheduler->current_running_process = nullptr;
                    CALL_SCHEDULER = true;
                    break;

                case TRANS_TO_DONE: // Print that the process is done and set the finish time
                    cpu_time += current_time - using_cpu_ts;
                    proc->state = STATE_DONE;
                    proc->finish_time = current_time;
                    if (vflag) std::cout << current_time << " " << proc->process_number << " " << proc->time_in_current_state << ": Done" << std::endl;
                    scheduler->current_running_process = nullptr;
                    CALL_SCHEDULER = true;
                    break;
            }
            delete evt; 
            evt = nullptr;

            if (CALL_SCHEDULER){ // decide if we need to get a new process right now or not
                // see if the next event has the same time as the current event
                if (get_next_event_time() == current_time) continue;
                CALL_SCHEDULER = false;
                // see if there is a process running right now
                if(scheduler->current_running_process == nullptr){
                    if (tflag) scheduler->print_scheduler();
                    // get the next process from the scheduler
                    proc = scheduler->get_next_process();
                    if (proc == nullptr) continue;
                    // If the scheduler returned a process then put it in an event a run it
                    put_event(new Event(*proc, current_time, TRANS_TO_RUN));
                }
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
                        std::sscanf(optarg+1, "%d:%d", &quantum, &maxprio);
                        s = new PRIO(quantum, maxprio);
                        break;
                    case 'E':
                        std::sscanf(optarg+1, "%d:%d", &quantum, &maxprio);
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