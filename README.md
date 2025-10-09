# CS401: Advanced Operating Systems

## Assignment 1 – Process Management and Scheduling Simulator

**Due Date:** 3:00pm,Thursday, 9 October 2025  
**Course:** CS401 – Advanced Operating Systems

## 🧩 Overview

This project implements a **Process Management and Scheduling Simulator** for an Operating System as part of the CS401 course.  
It models how an OS manages process creation, state transitions, and CPU scheduling using modular components written in C.

The simulator creates multiple processes, manages their lifecycle using **Process Control Blocks (PCBs)**, and schedules them using pluggable scheduling algorithms.

## 🏗️ Project Structure

| File                    | Description                                                                                                               |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| **`main.c`**            | Entry point of the simulator. Initializes OS parameters, creates processes, and invokes the scheduler.                    |
| **`os.c`**              | Implements OS-level functions such as initialization and scheduler invocation.                                            |
| **`process.c`**         | Contains all process-related functions (create, run, update PCB, etc.).                                                   |
| **`scheduling_algo.c`** | Implements one or more CPU scheduling algorithms (e.g., Round Robin, FCFS, Priority Scheduling).                          |
| **`scheduling_algo.h`** | Header file declaring scheduling algorithm interfaces.                                                                    |
| **`ScisSos.h`**         | Core header file defining constants, data structures (PCB, Process Table, Ready/Blocked Queues), and function prototypes. |
| **`test_perf.c`**       | Used for testing and performance measurement of different scheduling algorithms.                                          |
| **`Makefile`**          | Builds all components and links them into the final executable.                                                           |

## ⚙️ Key Components

### 1. **Data Structures**

- **Process Instruction** – Represents a single executable statement (`syscall`, `memory reference`).
- **Process Control Block (PCB)** – Stores all process-related metadata:  
  `pid`, `uid`, `priority`, `state`, `program_counter`, `time_slice`, and pointers to code/memory.
- **Process Table** – Holds all active process PCBs indexed by PID.
- **Ready Queue / Blocked Queue** – Lists of process IDs in ready or blocked states.
- **Page Table** – Models memory mapping (placeholder for future memory management).

### 2. **Process Lifecycle**

A process is simulated in three parts:

1. **Metadata:** PID, UID, size, etc.
2. **PCB:** Holds current state and control info.
3. **Instructions:** Sequence of short and long system calls.

During execution:

- A process runs until it hits a **long system call** or exhausts its **time slice**.
- It then yields control back to the **scheduler**.

### 3. **Scheduler**

The scheduler:

1. Scans the process table and updates the ready queue.
2. Invokes a chosen scheduling algorithm to pick the next process.
3. Updates PCBs to reflect state transitions (`RUNNING` ↔ `READY`).
4. Dispatches the selected process for execution.

## 🧠 Scheduling Algorithms

The framework supports plugging in multiple scheduling algorithms.  
The following are implemented and can be tested :

- **First-Come, First-Served (FCFS)**
- **Shortest Job First (SJF)**
- **Round Robin (RR)**
- **Priority Scheduling**

## 🚀 How to Build and Run

### 🧱 Build the Simulator

```bash
make
```

This compiles all `.c` files and generates two executables:

- `run_os` — the main OS simulator
- `test_perf` — performance testing tool

### ▶️ Run the Simulator

```bash
./run_os <scheduler_name>
```

or

```bash
make run_<scheduler_name>
```

Replace `<scheduler_name>` with one of:

```markdown
fcfs | sjf | priority | rr
```

Example:

```bash
./run_os rr
```

or

```bash
make run_rr
```

### 🧪 Run Performance Tests

```bash
./test_perf
```

or

```bash
make run_test_perf
```

This executes the benchmarking module that compares different scheduling strategies under identical workloads.

## 🧑‍💻 Contributors

- **Student Name(s):** Gedela Uday Kiran, L Sri Kasyap
- **Course Instructor:** Prof. Chakravarthy Bhagvati
