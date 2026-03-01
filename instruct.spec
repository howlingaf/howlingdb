1. Header row — the first row is always a header. Field names come from it.
2. Quoted fields — fields wrapped in "..." should have their quotes stripped. Commas
inside quotes are part of the value, not delimiters.
3. Escaped quotes — "" inside a quoted field means a literal " (e.g. Dana ""The
Great"" → Dana "The Great").
4. Missing values — empty fields (like age for Charlie, or score for Dana) should be
represented as null (or your language's equivalent), not an empty string.
5. Type inference — values that look like integers should be parsed as integers,
floats as floats, and everything else as strings. null stays null.

So the expected output for row 4 would be something like:

{ id: 4, name: 'Dana "The Great"', age: 40, score: null }
