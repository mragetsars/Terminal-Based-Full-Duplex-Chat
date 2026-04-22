# Terminal-Based Full-Duplex Chat (Ghost Chat)

> **Operating Systems (OS) – University of Tehran – Department of Computer Engineering**

![Language](https://img.shields.io/badge/Language-C-orange) ![Tool](https://img.shields.io/badge/Tool-gcc-blue) ![Status](https://img.shields.io/badge/Status-Completed-success)

## 📌 Overview

This repository contains the source code for **Ghost Chat**, a secure, full-duplex terminal chat room. It was developed as the *First Project: System Calls* for the *Operating Systems* course (Spring 2026) at the University of Tehran.

The primary goal of this project is to move beyond theoretical concepts and interact directly with the Linux operating system kernel. The application heavily relies on native system calls to handle inter-process communication (IPC), process management, and signal handling.

## 🎯 Project Objectives

  - ✅ **Inter-Process Communication (IPC):** Utilizing Linux Message Queues (`msgget`, `msgsnd`, `msgrcv`) to act as a shared mailbox for exchanging messages between separate user environments.
  - ✅ **Process Management:** Leveraging the `fork()` system call to divide the application into concurrent parent and child processes, enabling non-blocking, simultaneous communication.
  - ✅ **Signal Handling:** Coordinating safe process termination and peer-to-peer notifications using `kill()`, `waitpid()`, and POSIX signals.
  - ✅ **Low-Level File I/O:** Strict adherence to using raw system calls (`open`, `read`, `write`, `close`, `unlink`) for file and terminal operations, entirely bypassing standard C libraries like `stdio.h` for I/O tasks.

## 🧠 System Architecture & Phases

The project is structured into four distinct developmental phases:

### 1️⃣ Phase 1: Initial Connection

To allow two separate processes to discover each other, the system uses a hidden temporary file named `ghost_session`.

  * The first user executes the program, creates the message queue, and writes the queue's ID into this session file.
  * The second user reads the ID from the file to seamlessly connect to the same queue.

### 2️⃣ Phase 2: Full-Duplex Chat

Standard terminal input is blocking, meaning the program halts while waiting for keystrokes. To bypass this, the program forks into two processes:

  * **Sender (Parent):** Loops infinitely to read from standard input and push messages to the queue using `msgsnd`.
  * **Receiver (Child):** Loops infinitely, waiting on `msgrcv` to fetch and print incoming messages.
    Messages are distinguished in the shared queue using the `mtype` field, ensuring users don't read their own sent messages.

### 3️⃣ Phase 3: Self-Destructing Messages

Users can send temporary messages using the format `BURN <seconds> <message>`. When a BURN message is detected, a new child process is spawned to track the time. Once the duration expires, the process purges the specific message from the local history text files and refreshes the terminal interface, guaranteeing it is deleted for both the sender and the receiver.

### 4️⃣ Phase 4: Safe Exit

Typing the `EXIT` command triggers a structured cleanup routine. The program utilizes `unlink` to delete the session and history files, and `msgctl` to destroy the message queue. It also transmits a control signal to the connected peer to gracefully terminate their processes, ensuring no orphaned background tasks remain.

## 📂 Repository Structure

The project is organized as follows:

```text
Ghost Chat/
├── Description/           # Project specifications
│   └── OS-CA1.pdf         # Assignment description
├── Source/                # C Source Codes
│   └── ghost_chat.c       # Main application logic
├── README.md
```

## 🛠️ Tools & Verification

  * **Language:** C
  * **Target Environment:** Linux Operating System
  * **Verification Strategy:**
    1.  Compile using standard `gcc`.
    2.  Open two separate terminal windows.
    3.  Execute the binary in the first terminal (Acts as User 1).
    4.  Execute the binary in the second terminal (Acts as User 2).
    5.  Test concurrent messaging, BURN functionality, and safe closure.

## 👤 Author

This project was developed for the **Operating Systems** course at the **University of Tehran**.

  * **[Meraj Rastegar](https://github.com/mragetsars)**