#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main() {


    std::ofstream outfile("massive_log.txt", std::ios::binary);

    //. Create a standard dummy log line
    // We'll use a realistic 2026 timestamp. This specific string is exactly 100 bytes long (including the \n)
    std::string baseLine = "INFO [2026-04-18 12:00:00] User action processed successfully. Status: 200 OK. Transaction ID: 894\n";

    // 3. Build a 1 Megabyte chunk in RAM first
    std::string oneMegabyteChunk = "";
    // 1 MB is roughly 10,485 lines of 100 bytes
    for (int i = 0; i < 10485; ++i) {
        oneMegabyteChunk += baseLine;
    }

    // 4. Blast that 1MB chunk to the hard drive 10,240 times (which equals exactly 10 Gigabytes)
    int targetGigabytes = 10;
    int chunksNeeded = targetGigabytes * 1024; // 10,240 Megabytes

    for (int i = 0; i < chunksNeeded; ++i) {
        outfile.write(oneMegabyteChunk.data(), oneMegabyteChunk.size());
        
        
        if (i % 1024 == 0) {
            std::cout << "Generated " << (i / 1024) << " GB...\n";
        }
    }

    outfile.close();
    std::cout << "Done! massive_log.txt is ready for the ultimate test.\n";
    
    return 0;
}