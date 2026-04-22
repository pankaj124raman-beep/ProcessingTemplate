//goal is to open a file 
// read everything in it line by line
// hold the count of how many lines/words (not sure yet) are there
#include <fstream> // library to open and read a file
#include <string> // to handle the text content in the file
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <windows.h>
#include <psapi.h>

std::queue<std::vector<char>> bufferqueue;
std::mutex queuemutex;
std::condition_variable cv; // shoulder tap to wakeup sleeping threads

 bool isProducerFinished = false;
 const int queueCapisity = 10;

 void produceworker(){
    std::ifstream file("massive_log.txt");
    const int chunksize = 2050403;
    std::vector<char>leftover;
    
    while(true){
        std::vector<char>buffer(chunksize);
        file.read(buffer.data(),buffer.size());
        std::streamsize bytes = file.gcount();
        if(bytes == 0)break;
        
        buffer.resize(bytes); // shrink buffer to the size of bytes that we needed;

        std::vector<char>whole;

        whole.reserve(leftover.size() + buffer.size());
        whole.insert(whole.end(),leftover.begin(),leftover.end());
        whole.insert(whole.end(),buffer.begin(),buffer.end());

        auto rit = std::find_if(whole.rbegin(),whole.rend(),[](char c){return c == '\n';});
        
        if(rit !=whole.rend()){
            auto it = rit.base();
            leftover.assign(it,whole.end());
            whole.erase(it,whole.end());
        }else{
            leftover = std::move(whole);
            continue;
        }

        std::unique_lock<std::mutex> lock(queuemutex);

        cv.wait(lock, [] { return bufferqueue.size() < queueCapisity; }); //// BACKPRESSURE: If the queue has 5 items, drop the lock and go to sleep!
        
        // 3. Put the data on the belt
        bufferqueue.push(std::move(buffer));

       
        lock.unlock();
        cv.notify_all();
    }

    if(!leftover.empty()){
    std::unique_lock<std::mutex> lock(queuemutex);
    cv.wait(lock,[]{return bufferqueue.size() < queueCapisity;});
    bufferqueue.push(std::move(leftover));
    lock.unlock();
    cv.notify_all();
    }

    // Tell the Consumer we are done
    std::unique_lock<std::mutex> lock(queuemutex);
    isProducerFinished = true;
    lock.unlock();
    cv.notify_all();

 }

  void consumerworker(){
    long long totalines = 0;
    while(true){
    std::unique_lock<std::mutex> lock(queuemutex);
    // Wait until there is data to process, OR the Producer is completely finished
        cv.wait(lock, [] { return !bufferqueue.empty() || isProducerFinished; });
  
         // 2. If the queue is empty AND Producer is finished, we are done!
        if (bufferqueue.empty() && isProducerFinished) {
            break;
        }

        std::vector<char> buffer = std::move(bufferqueue.front());
        bufferqueue.pop();
       
        // 4. Unlock the Queue immediately and tap the Producer ("Hey, I made space!")
        lock.unlock();
        cv.notify_one();

        // 5. Do the heavy lifting (Line counting + our 50ms nap simulation)
        totalines += std::count(buffer.begin(), buffer.end(), '\n');
        
        // SIMULATING SLOW PROCESSING:
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

       
  }
   std::cout << "Total lines calculated by Consumer: " << totalines << "\n";
}

int main(){
auto start_time = std::chrono::high_resolution_clock::now();

    
    std::thread producer(produceworker);
    std::thread consumer(consumerworker);
    std::thread consumer2(consumerworker);
    std::thread consumer3(consumerworker);
    std::thread consumer4(consumerworker);
    std::thread consumer5(consumerworker);

    
    producer.join();
    consumer.join();
    consumer2.join();
    consumer3.join();
    consumer4.join();
    consumer5.join();

    // 3. Stop the clock
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        // PeakWorkingSetSize is the absolute maximum RAM used during the entire run
        double peakRamMB = pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
        std::cout << "Peak RAM usage: " << peakRamMB << " MB\n";
    }
    std::cout << "Total processing time: " << duration.count() << " seconds.\n";
    
return 0;
}