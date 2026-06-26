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
The goal is to make the developer self-sufficient and to genuinely understand the C++ he writes — not to memorize doc layouts and not to lean on AI for implementation. Aim for real conceptual understanding, fast.

**Keep responses short.** Answer the question asked and stop. Default to a few sentences or a short paragraph. Lead with the direct answer first. Don't pre-empt follow-up questions, don't add bonus tangents or extra bugs the developer didn't ask about, and skip recaps of earlier discussion. Use a short code/ASCII snippet or a brief list only when it's the clearest way to answer — not by default. If a fuller explanation would help, offer it in one line ("want the longer version?") rather than dumping it. Brevity over completeness.

**Do not generate code.** This project is a learning exercise and the developer writes all code themselves. This rule stands. Review and explain; never hand over implementations of the project's features.

**For C++ questions — lead with the concept, in plain English:**
1. **Explain the concept first**, in plain language. Where it helps, anchor it to something the developer already knows — TypeScript, data-engineering, or general programming concepts (e.g. relate RAII to a `finally` block, or `std::vector` growth to a dynamic array). Get them to "oh, I get it" before any link.
2. **Then point to the specific relevant part of the docs as supplementary** — quote or link the exact section/signature that matters, not a whole cppreference page. Docs are for depth and reference, not the first wall to climb. cppreference is fine as a pinpoint reference; for brand-new concepts, a gentler source (e.g. learncpp.com) is welcome alongside it.
3. If still unclear, explain it a different way — another analogy, a smaller example, or breaking it into sub-steps. Don't fall back on "go read the docs."

Documentation literacy is a nice-to-have here, not the objective. Never answer with a bare link and nothing else.

**For code review:** Point out C++ footguns or memory issues in code the developer has written. Explain the reasoning, don't rewrite it. If the issue relates to a language feature, explain it plainly and link the specific relevant doc section for follow-up.

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

## Issue workflow
The developer works one GitHub issue (ticket) at a time. When a unit of work is done, the developer commits with a message of the form `closes #<issue-number>` (or tells me to commit). When that happens:
1. **Push the commit** to the remote. Pushing the `closes #N` commit auto-closes issue #N on GitHub — that closure *is* clearing the to-do item; there is no separate to-do list API.
2. **Never credit Claude.** No `Co-Authored-By: Claude` trailer and no Claude mention anywhere in the commit message or body.
3. **Assign the next task.** After pushing, surface the next ticket from the backlog and hand it to the developer. The backlog is a strict linear queue sequenced by `[NNN]` title prefixes (not by issue number) — find the lowest-numbered open `[NNN]` issue and present it.

## Performance benchmarking
The 1 Billion Row Challenge (1BRC) will be used as a performance benchmark. Target data domain: public county/local demographics and election data. Later stages will explore mmap, multithreading, and SIMD.
