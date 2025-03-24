# SensorGateway

SensorGateway is a modular and multi-threaded C project designed to simulate and manage sensor data collection, processing, and logging in a distributed system. It showcases systems programming concepts such as inter-process communication (IPC), socket-based networking, and concurrency.

## Project Structure

The system simulates the following key components:
- **Sensor nodes** communicating with a central process over TCP
- **Main process** with the following responsibilities:
  - **Connection Manager**: handles incoming sensor connections via TCP sockets
  - **Data Manager**: stores incoming sensor readings in shared memory
  - **Storage Manager**: periodically writes data to `data.csv`
- **Logging process**: receives log events from other processes through IPC using unnamed pipes and stores them in a separate log file

## Technologies & Concepts Used
- Multi-threaded TCP server using POSIX threads (`pthreads`)
- Inter-process communication via unnamed pipes
- Socket programming for network communication
- Generic doubly linked list for shared data handling
- Modular C code with `Makefile` support
- Process synchronization and clean architecture

