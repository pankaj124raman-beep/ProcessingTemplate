#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main() {
    // 1. The blueprint for our log lines
    // Notice there is exactly ONE "Status: 500" error in this block.
    std::string log_template =
        "INFO [2026-04-28 14:00:01] User login successful. Status: 200\n"
        "INFO [2026-04-28 14:00:02] Page loaded successfully. Status: 200\n"
        "ERROR [2026-04-28 14:00:03] Database timeout. Status: 500\n"
        "INFO [2026-04-28 14:00:04] Image uploaded. Status: 200\n"
        "INFO [2026-04-28 14:00:05] Session refreshed. Status: 200\n";

    const unsigned long long TARGET_SIZE = 10ULL * 1024 * 1024 * 1024; // 10 Gigabytes
    unsigned long long current_size = 0;

    // 2. Build a massive 10MB chunk in RAM to make SSD writing lightning fast
    std::string chunk;
    chunk.reserve(10 * 1024 * 1024); // Reserve 10MB
    
    while (chunk.size() < 10 * 1024 * 1024) {
        chunk += log_template;
    }

    // 3. Count exactly how many errors are in this 10MB chunk
    long errors_per_chunk = 0;
    size_t pos = 0;
    while ((pos = chunk.find("Status: 500", pos)) != std::string::npos) {
        errors_per_chunk++;
        pos += 11; // target length
    }

    // 4. Open the file and start blasting it to the hard drive
    std::ofstream outfile("massive_log.txt", std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to open file for writing!\n";
        return 1;
    }

    std::cout << "Generating 10GB log file. Your disk will spin up now...\n";
    long total_errors_generated = 0;

    while (current_size < TARGET_SIZE) {
        outfile.write(chunk.data(), chunk.size());
        current_size += chunk.size();
        total_errors_generated += errors_per_chunk;

        // Print progress to the terminal every ~1GB
        if (current_size % (1ULL * 1024 * 1024 * 1024) < chunk.size()) {
            std::cout << "Generated " << current_size / (1024 * 1024 * 1024) << " GB...\n";
        }
    }

    outfile.close();
    
    // 5. THE TRUTH
    std::cout << "\n========================================\n";
    std::cout << "GENERATION COMPLETE!\n";
    std::cout << "File Size: ~10 GB\n";
    std::cout << "THE EXACT NUMBER OF ERRORS IS: " << total_errors_generated << "\n";
    std::cout << "========================================\n";
    std::cout << "Run your log processor. If it doesn't match this number exactly, your pipeline is broken!\n";

    return 0;
}