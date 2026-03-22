# Project: CSV Database Engine (C++)
## What this is
A C++ learning project building a database engine from scratch. Users upload CSVs via a web UI, data is validated, parsed, and stored in a custom `.db` file. Users can query the data using SQL through a browser-based console. Multiple users on different machines upload into a single shared database.

The SQL parsing is handled by a third-party library (e.g. Hyrise sql-parser) which produces a C++ AST struct. Everything else — query execution, storage, indexing — is custom C++.

## Architecture
For reference see `design-figure.excalidraw`
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

## How to assist
**Do not generate code.** This project is a learning exercise and the developer writes all code themselves.

Instead, assist by:
- Pointing to relevant cppreference.com documentation first — let the developer read it
- If they follow up, help them find the specific lines in the docs that answer their question
- Clarifying *why* something works or doesn't when the developer shares their own code
- Discussing tradeoffs between approaches so the developer can make their own decision
- Pointing out C++ footguns or memory issues in code the developer has written
- Suggesting what to look into next without prescribing the implementation

When reviewing developer-written code, give honest feedback on correctness and idiomatic C++ — explain the reasoning, don't rewrite it.

## Roadmap
1. **Validator** — file extension, encoding, empty check, header validation
2. **Parser** — byte-by-byte CSV parsing with quoting/escaping support
3. **Storage engine** — structured .db file format, page layout, write parsed rows to disk
4. **Query engine** — execute parsed SQL ASTs: table scans, index lookups, joins, aggregation
5. **Indexing** — B-trees for fast lookups
6. **CLI interface** — test everything locally before adding networking
7. **Web service** — HTTP server, file upload, query console, multi-user

## Performance benchmarking
The 1 Billion Row Challenge (1BRC) will be used as a performance benchmark. Target data domain: public county/local demographics and election data. Later stages will explore mmap, multithreading, and SIMD.
