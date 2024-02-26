#pragma once
#include <chrono>
#include <ostream>
#include <string>
#include <source_location>
#include <queue>
#include <thread> //threadID

namespace benchmarkMacros{
    #define BENCHMARK_TEST(func) {\
        Benchmark benchmark;\
        TEST(func, benchmark)\
        std::cout << benchmark;\
    }
    #define TEST(func, benchmark) {\
        while (!benchmark.isFull()){\
            Timer start;\
            func;\
            benchmark.addRunData(start.duration());\
        }\
    }
}

class Benchmark {
public:
    Benchmark(){runTimeData.reserve(32);};
    Benchmark(int runs){runTimeData.reserve(runs);};
    // ~Benchmark();

    bool isFull(){return runTimeData.size() == runTimeData.capacity();}
    int getNumberOfRuns(){return runTimeData.size();}
    void addRunData(std::chrono::nanoseconds time);
    const std::chrono::nanoseconds getAverage();
    const std::chrono::nanoseconds &getFastestRun(){return minRunTime;}
    const std::chrono::nanoseconds &getSlowestRun(){return maxRunTime;}
    
    friend std::ostream& operator <<(std::ostream& os, Benchmark& b);
private:
    std::vector<std::chrono::nanoseconds> runTimeData;
    std::chrono::nanoseconds totalRunTime{};
    std::chrono::nanoseconds minRunTime{INT_MAX};
    std::chrono::nanoseconds maxRunTime{0};

    bool totalDirty{false};
    void updateTotal();
};

class Timer {
public:
    /// @brief Create a Timer object and initialize the starting time point
    Timer():start(std::chrono::system_clock::now()){};

    /// @brief re-sets the start of the Timer to the current time
    void set(){start = std::chrono::system_clock::now();}

    /// @brief Returns a referance to the underlying time_point associated with the Timer
    const std::chrono::time_point<std::chrono::system_clock> &getStart(){return start;}

    /// @brief Returns the amount of time in microseconds that this timer has been running
    std::chrono::nanoseconds duration(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()-start);}

    /// @brief Returns the start time in string format (local time)
    std::string startTime();

    /// @brief Returns the current time (local time)
    static std::string currentTime();
    
    /// @brief Returns the difference between two timer objects in nano seconds
    std::chrono::nanoseconds operator -(Timer& t) const{return std::chrono::duration_cast<std::chrono::nanoseconds>(start-t.getStart());}

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

struct markerData {
    const double duration;
    const std::string funcName;
    const std::thread::id id;
    std::chrono::time_point<std::chrono::system_clock> start;
    friend std::ostream& operator <<(std::ostream& os, markerData& md){
        os << "\n      ,{\"cat\":\"function\",\"dur\":" << md.duration
                        << ",\"name\":\"" << md.funcName << "\""
                        << ",\"ph\":\"X\",\"pid\":0,\"tid\":"<< md.id
                        << ",\"ts\":" << md.start << "}";
        return os;
    }
};

class ProfilerMarker {
public:
    ProfilerMarker(std::source_location loc = std::source_location::current()):functionName{loc.function_name()}{start.set();}
    ~ProfilerMarker(){writeData();}

    /// @brief Return the timer object associated with this profiler marker
    Timer& getTimer(){return start;}

private:
    Timer start;
    std::string functionName;
    //Write the data for this Profile Marker to the outputFile
    void writeData();
};

//Information on the json formatting for chrome://tracing
//https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
class Profiler {
public:
    Profiler(){p = new ProfilerMarker();}
    Profiler(const Profiler& pf)=delete; //Remove copy consructor
    ~Profiler(){start = p->getTimer().getStart();
                delete p;
                write();}

    /// @brief Add marker data to the profiler
    static void addMarker(markerData data){markers.push(data);}

private:
    ProfilerMarker *p;
    static std::queue<markerData> markers;
    std::chrono::time_point<std::chrono::system_clock> start;

    //This should only be called once to End the profiler data file
    void write();

    //Turns the marker data into json format with the start relitive to the Profiler start
    void printMarker(std::ofstream& os, markerData& md);
};