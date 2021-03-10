#include <iostream>
#include <deque>
#include <list>

class Process{
public:
    int arrival_time;
    int total_cpu_time;
    int cpu_burst;
    int io_burst;
    Process(int at, int tc, int cb, int io){
        arrival_time = at;
        total_cpu_time = tc;
        cpu_burst = cb;
        io_burst = io;
    }
};

class Scheduler{
public:
    int x;
    Scheduler(int myx){
        x = myx;
    }
    virtual void add_process(){
        std::cout << "Hello world" << std::endl;
    }
    virtual Process* get_next_process(){
        return NULL;
    }
    virtual void test_preemt(Process *p, int curtime){
    }
};

class Event{
public:
    int timestamp;
    Process* process;
    Event(int ts){
        timestamp = ts;
    }
};

class Simulation{
public:
    std::deque<Event*> eventQ;
    Scheduler* scheduler;
    Simulation(Scheduler &s){
        scheduler = &s;
    }
    void start_simulation(){
    }
};


int main(){
    // Scheduler myScheduler = Scheduler(10);
    // Simulation s = Simulation(myScheduler);
    // std::cout << s.scheduler->x << std::endl;
    // myScheduler.x++;
    // std::cout << s.scheduler->x << std::endl;
    // myScheduler.add_process();
    // s.scheduler->add_process();
    std::list<Event*> eventQ;
    eventQ.push_back(new Event(1));
    eventQ.push_back(new Event(2));
    eventQ.push_back(new Event(3));
    eventQ.push_back(new Event(4));
    eventQ.push_back(new Event(7));
    eventQ.push_back(new Event(9));
    eventQ.push_back(new Event(10));
    eventQ.push_back(new Event(12));
    eventQ.push_back(new Event(19));
    Event e(20);
    for(std::list<Event*>::iterator it = eventQ.begin(); it != eventQ.end(); ++it){
        std::cout << (*it)->timestamp << std::endl;
        if (e.timestamp < (*it)->timestamp){
            std::cout << "inserting" << std::endl;
            eventQ.insert(it, &e);
            break;
        }
        if(it == eventQ.end()){
            std::cout << "We are in the endgame now" << std::endl;
            eventQ.push_back(&e);
        }
    }
    for(std::list<Event*>::iterator it = eventQ.begin(); it != eventQ.end(); ++it){
        std::cout << (*it)->timestamp << std::endl;
    }
}

