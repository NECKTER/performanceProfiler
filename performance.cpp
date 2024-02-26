#include "performance.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <ctime>


//------------------------Benchmark Class functions------------------------
std::string Timer::startTime(){
    char time[32];
    const std::time_t timePoint = std::chrono::system_clock::to_time_t(start);
    std::strftime(time, sizeof(time), "%Y_%m_%d_%H_%M_%S", std::localtime(&timePoint));
    return std::string(time);
}

std::string Timer::currentTime(){
    char time[32];
    const std::time_t timePoint = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::strftime(time, sizeof(time), "%Y_%m_%d-%H_%M_%S", std::localtime(&timePoint));
    return std::string(time);
}


//------------------------Benchmark Class functions------------------------
void Benchmark::addRunData(std::chrono::nanoseconds time){
    if (!totalDirty)
        totalDirty = true;
    minRunTime = std::min(minRunTime, time);
    maxRunTime = std::max(maxRunTime, time);
    runTimeData.push_back(time);
}

const std::chrono::nanoseconds Benchmark::getAverage(){
    updateTotal();
    return totalRunTime/runTimeData.size();
}

std::ostream& operator <<(std::ostream& os, Benchmark& b){
    os << "Average," << b.getAverage() << ", Min," << b.getFastestRun() 
       << ", Max," << b.getSlowestRun() << ", Complete Data";
    for (auto elem: b.runTimeData){
        os << ", " << elem;
    }
    os << '\n';
    return os;
}

void Benchmark::updateTotal(){
    if (!totalDirty)
        return;
    totalRunTime = std::chrono::nanoseconds(0);
    for (auto time:runTimeData)
        totalRunTime += time;
    totalDirty = false;
}


//------------------------Profiler Class functions------------------------
std::queue<markerData> Profiler::markers; //Init private static member

void Profiler::write(){
    //Create a json file that can be viewed by chrome://tracing
    std::filesystem::create_directory("ProfilerData");
    std::ofstream outFile("ProfilerData/ProfilerData"+Timer::currentTime() + ".json", std::ios_base::out);
    outFile << "\
{\n\
    \"displayTimeUnit\": \"ns\",\n\
    \"otherData\": {\n\
        \"NTS_Profiler\":\"Profiler v1 by Nicholas Scheele\"\n\
    },\n\
    \"traceEvents\": [{}";

    //TODO: add mutex
    while (markers.size()){
        printMarker(outFile, markers.front());
        markers.pop();
    }
    outFile << "]\n}";
    outFile.close();
}

void Profiler::printMarker(std::ofstream& os, markerData& md){
    os << "\n      ,{\"cat\":\"function\",\"dur\":" << md.duration
                        << ",\"name\":\"" << md.funcName << "\""
                        << ",\"ph\":\"X\",\"pid\":0,\"tid\":"<< md.id
                        << ",\"ts\":" << (md.start - start).count()/1000.0 << "}";
}


//------------------------ProfilerMarker Class functions------------------------
void ProfilerMarker::writeData(){
    //Write the data for the curent Profiler object
    //TODO: add mutex
    Profiler::addMarker({static_cast<double>(start.duration().count())/1000,
                        functionName,
                        std::this_thread::get_id(),
                        start.getStart()});
}