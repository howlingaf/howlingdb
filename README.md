# DBMS

High-performance CSV ingestion engine written in C++.

> **Project board:** [View Kanban →](https://github.com/users/howlingfantods-dev/projects/1)

---

## Overview

A database engine built from scratch, focusing on efficient CSV ingestion and low-level disk management. The project explores core database internals: page-based storage, disk management, record serialization, and buffer management.

## Architecture

- **DiskManager** — handles raw page reads/writes to disk
- **Page layout** — fixed-size pages with slot-based record storage
- **Record serialization** — reading and writing variable-length records within pages

## Build

```bash
clang++ -std=c++17 main.cpp -o dbms
```

## Progress

Development is tracked on the [project board](https://github.com/users/howlingfantods-dev/projects/1).

