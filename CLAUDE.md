# Project: CSV Database Engine (C++)
## What this is
A C++ learning project building a database engine from scratch. Users upload CSVs via a web UI, data is validated, parsed, and stored in a custom `.db` file. Users can query the data using SQL through a browser-based console. Multiple users on different machines upload into a single shared database.

The SQL parsing is handled by a third-party library (e.g. Hyrise sql-parser) which produces a C++ AST struct. Everything else — query execution, storage, indexing — is custom C++.

## Architecture
For reference see `resources/design-figure.excalidraw`
```
CSV Upload → Validator → Parser → Storage Engine → .db file
SQL Query  → SQL Parser (library) → Query Engine → Storage Engine → Results
```

**Data ingestion pipeline:**
1. Validator — file extension, encoding (control chars), empty check, header structure
2. Parser — byte-by-byte CSV parsing, handles quoting, escaping, commas in values
3. Storage engine — writes parsed rows to structured .db file on disk

**Query pipeline:**
1. SQL parser library parses query string into AST struct
2. Query engine reads AST, plans execution (scan vs index), executes against storage engine
3. Storage engine retrieves matching data, uses B-tree indexes when available

**Web service (future layer on top):**
- HTTP server, file upload UI, query console, multi-user access
- Separate from the engine — the engine is a standalone library

## Developer background
- Professional experience in TypeScript and data engineering
- Familiar with data ingestion pipelines, API orchestration, and CSV processing at scale
- Learning C++ through this project
- Developing in Neovim inside WSL on Windows

## Learning Approach
See `resources/syllabus.pdf` for the structured learning plan. Key principle: **code first, read second**. For each unit:
1. **TRY IT THE NAIVE WAY** — attempt with what you already know, discover why it fails
2. **NOW LEARN THE REAL SOLUTION** — read the assigned materials with a specific problem to solve
3. **BUILD IT** — implement the solution with hands-on understanding

## How to assist
The goal is to make the developer self-sufficient — not reliant on AI. Every interaction should build the skill of finding and reading documentation, not the habit of asking Claude.

**Do not generate code.** This project is a learning exercise and the developer writes all code themselves.

**For C++ questions:**
1. First, respond with a link to the relevant cppreference.com documentation page. Nothing else.
2. If they follow up or say they don't understand, quote the specific section(s) of the documentation that answer the question, and explain how those lines answer it.
3. If still unclear, explain the concept in terms of how the documentation is structured — so they learn how to read it, not just what it says.

The aim is to teach documentation literacy. The developer should leave each interaction better at finding answers themselves, not more dependent on Claude.

**For code review:** Point out C++ footguns or memory issues in code the developer has written. Explain the reasoning, don't rewrite it. If the issue relates to a language feature, link the relevant cppreference page.

## Roadmap (Phases from Syllabus)
**Phase 0: The Foundation** — CSV ingestion and C++ basics
1. **Validator** (Unit 0) — file extension, encoding, empty check, header validation ✅ Done
2. **Parser** (Unit 0) — byte-by-byte CSV parsing with quoting/escaping support ✅ Done

**Phase 1: Storage** — How data lives on disk
3. **Storage engine** (Units 1–3) — pages, buffer pools, compression, .db file format

**Phase 2: Indexing** — Finding data fast
4. **B-tree indexes** (Units 4–5) — fast lookups, range queries, concurrency

**Phase 3: Query Execution** — Running queries
5. **Sort, join, aggregate** (Units 6–8) — external merge sort, join algorithms, iterator model

**Phase 4: SQL Parsing & Optimization** — From text to plan
6. **SQL parser** (Unit 9) — tokenization, AST, recursive descent parsing
7. **Query optimizer** (Unit 10) — cost estimation, join ordering, equivalence rules

**Phase 5: Transactions & Recovery** — Making it bulletproof
8. **Concurrency control** (Unit 11) — 2PL, MVCC, conflict serialization
9. **Crash recovery** (Unit 12) — write-ahead logging, ARIES algorithm

**Phase 6: Web service** — User-facing layer
10. **CLI interface** — test everything locally before adding networking
11. **HTTP server, file upload, query console** — multi-user access

## Performance benchmarking
The 1 Billion Row Challenge (1BRC) will be used as a performance benchmark. Target data domain: public county/local demographics and election data. Later stages will explore mmap, multithreading, and SIMD.
