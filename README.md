# Log Processor — Concepts Learned
**Project:** Concurrent HTTP Error Detector on Massive Log Files  
**Language:** C++17 | **Platform:** Ubuntu 24.04 LTS  
**Result:** 10GB processed in 14.5 seconds, 722% CPU utilization, 208MB peak RAM

---

## What This Project Does

Scans massive log files (tested on 10GB) for HTTP `Status: 500` errors using a
producer-consumer architecture with 8 threads. Built from scratch without any
external libraries — only C++17 standard library.

**Real world equivalent:** This is exactly what log analysis tools like Splunk,
Datadog, and Grafana Loki do internally — scan gigabytes of logs concurrently
to surface errors and anomalies.

---

## Architecture Overview

```
[Disk] → Producer (1 thread)
              ↓
         [Bounded Queue] (max 10 chunks, 8MB each)
              ↓
    Consumer Threads (7 threads)
    Each scans chunk for "Status: 500"
              ↓
    std::atomic<long> total_global_errors
              ↓
         [Final Count]
```

---

## Concepts Learned

---

### 1. Producer-Consumer Architecture

**What it is**
A concurrency pattern where work is split into two roles:
- **Producer** — generates data (reads from disk)
- **Consumer** — processes data (scans for errors)

Neither knows about the other's internals. They only communicate through a shared queue.

**Why it matters**
Without this pattern, you'd either:
- Read everything into RAM first (memory explosion on 10GB file)
- Process sequentially with no parallelism (slow)

The producer-consumer model lets I/O and CPU work happen simultaneously.

**Your implementation**
```cpp
std::thread producer(produceworker);   // 1 disk reader
std::thread consumer(consumerworker);  // 7 error scanners
// ...
```

---

### 2. Bounded Queue with Backpressure

**What it is**
A queue with a maximum capacity (`queueCapacity = 10`). When full, the producer
is forced to stop and wait.

**Why it matters**
Without a capacity limit, if consumers are slower than the producer, the queue
grows unboundedly — eventually consuming all RAM.

**Your implementation**
```cpp
cv.wait(lock, [] { return bufferqueue.size() < queueCapisity; });
```
When queue hits 10 chunks, producer sleeps. When a consumer pops a chunk, it
wakes the producer. Memory stays bounded regardless of file size.

**Proven result:** 10GB file processed using only 208MB RAM — a 50x memory
efficiency ratio.

---

### 3. Mutex and Critical Sections

**What it is**
A mutex ensures only one thread accesses shared data (`bufferqueue`) at a time.

**Why it matters**
Without a mutex, two threads could simultaneously read/write the queue causing
data corruption — a **data race**.

**Your implementation**
```cpp
std::unique_lock<std::mutex> lock(queuemutex);
bufferqueue.push(std::move(whole));
lock.unlock(); // release before doing heavy work
cv.notify_all();
```

**Key insight applied correctly:** unlock the mutex *before* scanning for errors.
Holding the lock during `string_view::find()` would serialize all threads,
defeating parallelism entirely.

---

### 4. Condition Variables

**What it is**
A signaling mechanism that lets threads sleep efficiently and wake only when
something meaningful happens — no busy waiting.

**Three signals in your code:**
1. **Producer → Consumers:** "New chunk in queue" (`cv.notify_all()`)
2. **Consumer → Producer:** "Space available" (`cv.notify_one()`)
3. **Producer → Consumers:** "No more data" (via `isProducerFinished` flag)

**Your implementation**
```cpp
cv.wait(lock, [] { return !bufferqueue.empty() || isProducerFinished; });
```
Releases lock, sleeps thread, re-acquires lock when woken — all atomically.

---

### 5. std::atomic for Lock-Free Global Counter

**What it is**
An atomic variable guarantees that read-modify-write operations are thread-safe
without needing a mutex.

**Why it matters**
7 consumer threads all increment the same global error counter. Without
`std::atomic`, two threads could read the same value simultaneously, both
increment it, and write back — losing one count. This is a classic **race condition**.

**Your implementation**
```cpp
std::atomic<long> total_global_errors(0);
// ...
total_global_errors += local_error_count; // thread-safe, no mutex needed
```

**Smart pattern you used:** each consumer accumulates into a `local_error_count`
first, then does a single atomic addition at the end. This minimizes contention
on the atomic variable — much better than incrementing it once per match.

---

### 6. string_view for Zero-Copy Scanning

**What it is**
`std::string_view` is a non-owning reference to existing character data. It
doesn't allocate memory — it just points to your existing buffer.

**Why it matters**
You're scanning 8MB chunks. Creating a `std::string` from each chunk would copy
8MB of data just to search it. `string_view` gives you all string operations
with zero copying.

**Your implementation**
```cpp
std::string_view window(buffer.data(), buffer.size());
std::string_view target = "Status: 500";
cursor_pos = window.find(target, cursor_pos);
```

This is the correct systems programming approach — never copy data you don't
need to copy.

---

### 7. Move Semantics (`std::move`)

**What it is**
Transferring ownership of data instead of copying it.

**Why it matters**
Your chunks are 8MB vectors. Without `std::move`, every push/pop would copy
8MB. With 1250 chunks in a 10GB file that's 10GB of unnecessary copying.

**Your implementation**
```cpp
bufferqueue.push(std::move(whole));                          // O(1) not O(n)
std::vector<char> buffer = std::move(bufferqueue.front());  // same
```

After a move, the source becomes empty. Data stays in place, only ownership
transfers.

---

### 8. Chunk-Based File Reading with Boundary Handling

**What it is**
Reading in fixed 8MB blocks with logic to handle patterns (`Status: 500`) that
could be split across chunk boundaries.

**The boundary problem**
If "Status: 500" falls exactly on a chunk boundary:
```
Chunk 1 ends: "...Status: 5"
Chunk 2 starts: "00\nGET /api..."
```
A naive scanner would miss this match entirely.

**Your solution**
Producer finds the last `\n` in each chunk, keeps everything after it as
`leftover`, and prepends it to the next chunk. Consumers always receive
complete, boundary-safe data.

```cpp
auto rit = std::find_if(whole.rbegin(), whole.rend(), [](char c){ return c == '\n'; });
leftover.assign(it, whole.end());
whole.erase(it, whole.end());
```

---

### 9. Sliding Window Pattern for String Search

**What it is**
Instead of searching the entire buffer from the start each time, you advance a
cursor forward after each match.

**Your implementation**
```cpp
size_t cursor_pos = 0;
while(true){
    cursor_pos = window.find(target, cursor_pos);
    if(cursor_pos == std::string_view::npos) break;
    local_error_count++;
    cursor_pos += target.length(); // advance past current match
}
```

This is O(n) — you scan each byte exactly once. Without `cursor_pos +=
target.length()` you'd re-scan already matched positions.

---

### 10. Performance Benchmarking with `/usr/bin/time -v`

**Metrics and meaning**

| Metric | Result | Meaning |
|--------|--------|---------|
| Elapsed time | 14.5s | Real wall clock time |
| User time | 102.73s | Total CPU across all threads |
| System time | 2.44s | Kernel I/O time |
| CPU utilization | 722% | ~7 cores fully used |
| Peak RAM | 208MB | Max memory at any point |
| Major page faults | 0 | No disk swapping |

**Key formula**
`CPU% = (User + System) / Elapsed × 100`
`(102.73 + 2.44) / 14.5 × 100 ≈ 724%` ✓

---

### 11. The Disk I/O Bottleneck — Discovered Empirically

**What you found**
Changing chunk size from 8MB → 16MB barely changed performance. This proved
the bottleneck is disk read speed, not thread synchronization or CPU.

**Why**
- SSD reads at ~500MB/s
- 7 consumers process data faster than producer can supply it
- Consumers spend most time waiting for queue, not processing

**Mental model**
```
Disk (slow pipe) → Producer → Queue → Consumers (fast, starving)
```
This is called **I/O bound** behavior. No amount of extra consumer threads
can exceed disk speed.

---

### 12. Bug Found and Fixed

**The bug**
```cpp
// WRONG — pushing raw disk read, losing boundary logic
bufferqueue.push(std::move(buffer));

// CORRECT — pushing leftover + buffer, properly trimmed
bufferqueue.push(std::move(whole));
```

**Lesson:** Always verify correctness with ground truth.
```bash
wc -l logs.txt                   # verify line counts
grep -c "Status: 500" logs.txt   # verify error counts
```
Multithreaded count matched exactly — **fast AND correct**.

---

## Summary

| Skill | Evidence |
|-------|---------|
| Concurrent programming | 8 threads, correct synchronization |
| Memory efficiency | 10GB file, 208MB RAM |
| Lock-free programming | `std::atomic` for global counter |
| Zero-copy techniques | `std::string_view` for scanning |
| Systems thinking | Chunk size experiments, bottleneck identification |
| Debugging | Found and fixed `buffer` vs `whole` bug |
| Performance measurement | `/usr/bin/time -v`, interpreted all metrics |
| C++17 features | `std::move`, `std::string_view`, lambdas, `std::atomic`, `std::thread` |

**Bottom line:** A production-grade concurrent log analyzer built from scratch.
Scans 10GB in 14.5 seconds using 208MB RAM across 7 parallel threads — with
correct results verified against ground truth.
