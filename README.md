# High-Performance Multithreaded Log Processor (C++)

A high-efficiency C++ program designed to process massive files using a **producer–consumer architecture** with multithreading and controlled memory usage.

## 🚀 Key Achievement

* Processed **10 GB file**
* Maintained only **~36 MB RAM usage**
* Implemented **backpressure-controlled pipeline**
* Parallel processing using multiple consumer threads

---

## 🧠 Core Concept

This project uses a **Producer–Consumer pattern**:

* **Producer Thread**

  * Reads file in fixed-size chunks
  * Pushes data into a shared queue

* **Consumer Threads (4x)**

  * Process chunks in parallel
  * Count newline characters efficiently

* **Backpressure Mechanism**

  * Queue capacity is limited (size = 5)
  * Producer pauses when queue is full
  * Prevents memory overflow

---

## ⚙️ Architecture Overview

```
File (massive.txt)
        ↓
   Producer Thread
        ↓
  [ Bounded Queue (size = 5) ]
        ↓
 Multiple Consumer Threads (4x)
        ↓
   Line Count Aggregation
```

---

## 📌 Features

* 📂 Chunk-based file reading (no full file loading)
* 🧵 Multithreaded processing (1 Producer + 4 Consumers)
* 🔒 Thread-safe queue using:

  * `std::mutex`
  * `std::condition_variable`
* ⚡ Backpressure control to limit memory usage
* 📉 Extremely low RAM footprint for large-scale data

---

## 🛠️ Tech Stack

* **Language:** C++
* **Concepts Used:**

  * Multithreading (`std::thread`)
  * Synchronization (`mutex`, `condition_variable`)
  * Producer–Consumer Pattern
  * File Streaming (`ifstream`)
  * Move semantics (`std::move`)

---

## ▶️ How to Run

### 1. Compile

```bash
g++ main.cpp -o processor -pthread
```

### 2. Run

```bash
./processor
```

> Ensure `massive.txt` is present in the same directory.

---

## 📊 Performance

| Metric           | Value                        |
| ---------------- | ---------------------------- |
| File Size Tested | 10 GB                        |
| RAM Usage        | ~36 MB                       |
| Threads Used     | 5 (1 Producer + 4 Consumers) |
| Queue Capacity   | 5 Buffers                    |

---

## 🔍 How It Works (Step-by-Step)

1. Producer reads file in chunks (~2 MB each)
2. Pushes chunks into a bounded queue
3. Consumers wait for available data
4. Each consumer:

   * Pops a chunk
   * Counts `\n` characters
5. Condition variables coordinate thread execution
6. Ensures:

   * Controlled memory usage
   * Efficient parallel processing

---

## ⚠️ Notes

* File path is currently hardcoded (`massive.txt`)
* Simulated delay (`50ms`) added to demonstrate workload
* Output includes total line count and execution time

---

## 📈 Future Improvements

* Accept file path via command-line arguments
* Remove artificial delay for real benchmarking
* Add word/character counting
* Implement thread pool
* Optimize chunk size dynamically

---

## 👨‍💻 Author

Pankaj Raman
