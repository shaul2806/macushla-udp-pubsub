# Macushla Robotics – UDP Publish/Subscribe Home Assignment

This repository implements a simple multi-node publish/subscribe system over a local network using UDP broadcast.

The system demonstrates:
- Publisher / subscriber architecture
- UDP networking
- Apparent concurrency without threads
- Safety-first control logic
- Cross-platform design (developed on Windows, runnable on Linux)

---

## System Overview

The system consists of **four independent nodes** communicating over UDP port **1111**.

### Message Format
```
<topic>,<data>
```

### Examples
```
mot_vel,-39.4
estop,1
```

---

## Nodes

### Node 1 — Python Publisher (Motor Velocity)
- Publishes messages on topic `mot_vel`
- Random values between `-100.0` and `+100.0`
- Broadcasts via UDP

### Node 2 — C++ Publisher (Emergency Stop)
- Publishes messages on topic `estop`
- Values:
  - `0` → SAFE

  - `1` → STOP
- Publishes at random intervals (every few seconds)

### Node 3 — Python Subscriber / Controller
- Subscribes to `mot_vel` and `estop`
- Uses an event-driven loop (`select`) to remain responsive
- Behavior:
  - When `estop = 1`, motor commands are ignored and velocity is forced to `0`

  - When `estop = 0`, motor commands are accepted

### Node 4 — C++ Subscriber / Safety Logger
- Subscribes only to `estop`
- Implements an internal state machine:
  - UNKNOWN → SAFE → STOP
- Logs only **state transitions**
- Writes logs to `safety_log.txt`
- Each log entry includes milliseconds since node boot

---

## Repository Structure
```
macushla-udp-pubsub/
├── node1_python_publisher/
│ └── mot_vel_publisher.py
├── node2_cpp_publisher/
│ └── estop_publisher.cpp
├── node3_python_subscriber/
│ └── controller.py
├── node4_cpp_subscriber/
│ └── safety_logger.cpp
├── README.md
└── safety_log.txt
```
---

## Build & Run Instructions (Linux)

### Requirements
- Python 3
- g++


### Terminal 1 — Node 4 (Safety Logger)
Start this first.

```bash
g++ node4_cpp_subscriber/safety_logger.cpp -o node4_cpp_subscriber/safety_logger
./node4_cpp_subscriber/safety_logger

```

### Terminal 2 — Node 3 (Controller)
```bash
python3 node3_python_subscriber/controller.py
```

### Terminal 3 — Node 1 (Motor Velocity Publisher)
```bash
python3 node1_python_publisher/mot_vel_publisher.py
```

### Terminal 4 — Node 2 (Emergency Stop Publisher)
```bash
g++ node2_cpp_publisher/estop_publisher.cpp -o node2_cpp_publisher/estop_publisher
./node2_cpp_publisher/estop_publisher

```


## Expected Behavior
- Node 3 prints motor velocity values while the system is SAFE

- When `estop,1` is published:
 - Node 3 stops printing motor velocity and forces velocity to `0`

 - Node 4 logs a transition to STOP

- When `estop,0` is published:
 - Node 3 resumes motor velocity output

 - Node 4 logs a transition back to SAFE

Example log output (`safety_log.txt`):

```
[1532 ms] UNKNOWN -> SAFE

[4210 ms] SAFE -> STOP

[8120 ms] STOP -> SAFE

```

## Notes
- UDP broadcast is used for simplicity and loose coupling between nodes
- No node assumes knowledge of other nodes’ existence
- The architecture resembles real robotic systems where safety channels override motion commands

## Author
Shaul Duek
