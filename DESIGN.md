# M1 DESIGN.md

## 1. System structure

The system has six pieces. TextProcessor normalizes text and splits it into tokens, and every other component uses it, so a term means the same thing whether it came from a document or a query. Chunker splits a Document into overlapping chunks, 120 tokens max with 20 tokens of overlap, preferring a paragraph break near the limit over cutting mid-paragraph. CorpusIndex is an inverted index: which chunks contain a term, and how often. RetrievalEngine scores and ranks chunks for a query using that index. ContextBuilder turns ranked results into a list of chunks that fit a token budget. ProcessingCore owns a Chunker, the chunk list, and the CorpusIndex, and ties the others together for rebuild, search, and build_context.

## 2. Design decisions

CorpusIndex keeps two hashpmaps, the term to postings, and chunk id to chunk index. They're looked up by different keys, so they're separate maps. A posting stores an index into the chunk vector, not a copy of the chunk, since the chunk data already lives in one place in ProcessingCore. ProcessingCore uses a pimpl, so its private state stays out of the public header. RetrievalEngine and ContextBuilder hold no state between calls, so they're built on the stack where they're used instead of stored as members. Rebuild builds the new chunks and index into local variables first, and only writes them into the stored state at the very end, once every check that could fail has already happened. That order is what makes a failed rebuild leave the previous corpus untouched.

## 3. Correctness and consistency

Documents and queries go through the same normalization, so a term always means the same thing on both sides. Rebuild clears the old index before building the new one, so nothing from a previous corpus survives. Duplicate document ids are checked before any state is touched, so a rebuild either fully succeeds or changes nothing. Scores are rounded to twelve decimal places before comparing, so floating point noise can't flip the ranking order. Ties are broken by document insertion order, then chunk sequence, never by hash map iteration order.

## 4. Testing strategy

There's one test file per component, text processing, chunking, corpus index, retrieval, context building, plus one integration file for ProcessingCore. Each component is tested with its own inputs rather than always going through ProcessingCore, so a bug in one layer can't hide behind another. The cases we cared about most were empty and punctuation-only input, LF versus CRLF blank lines, a paragraph break inside the 100-120 token window versus outside it, the hard 120-token cut and the 20-token overlap, a rebuild not leaking postings from the old corpus, ranking scores worked out by hand before writing the assertion, and for context building a chunk that fits exactly, one that gets truncated, and one that's skipped because the budget already hit zero.

## 5. Alternatives considered

I considered validating terms inside CorpusIndex instead of ProcessingCore, but its lookup methods are marked not to throw, so the validation has to live one layer up, where the raw query text first arrives. I also considered storing RetrievalEngine and ContextBuilder as part of ProcessingCore's stored state, like the chunker and the index. Didn't go through with it because neither holds state between calls, so there's nothing to gain by keeping them around.