#include "aiws/retrieval_engine.hpp"
#include "aiws/corpus_index.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool close(double a, double b) {
    return std::abs(a - b) < 1e-9;
}

aiws::Chunk make_chunk(const std::string& id, const std::string& document_id,
                       std::size_t document_order, std::size_t sequence,
                       const std::string& text) {
    aiws::Chunk c;
    c.id = id;
    c.document_id = document_id;
    c.document_order = document_order;
    c.sequence = sequence;
    c.text = text;
    return c;
}
}

int main() {
    using namespace aiws;

    // Fresh 4-chunk / 2-document corpus, independent of the public test fixture.
    // Expected scores hand-derived (TF/IDF/coverage formulas computed with a
    // calculator, not by running RetrievalEngine) for query "cat dog":
    //   c0 = 4.159313066162, c2 = 2.695249291973,
    //   c1 = 1.586366904954, c3 = 1.284300728880
    std::vector<Chunk> chunks{
        make_chunk("p#0", "p", 0, 0, "cat cat dog"),
        make_chunk("p#1", "p", 0, 1, "cat bird"),
        make_chunk("q#0", "q", 1, 0, "dog dog dog bird"),
        make_chunk("q#1", "q", 1, 1, "fish fish dog"),
    };
    CorpusIndex index(chunks);
    RetrievalEngine engine;

    bool negative_k_threw = false;
    try {
        (void)engine.search("cat", -1, chunks, index);
    } catch (const std::invalid_argument&) {
        negative_k_threw = true;
    }
    check(negative_k_threw, "negative k throws invalid_argument");

    check(engine.search("cat", 0, chunks, index).empty(), "k == 0 returns no results");
    check(engine.search("...---", 10, chunks, index).empty(),
          "an empty/punctuation-only query returns no results");
    check(engine.search("elephant", 10, chunks, index).empty(),
          "query terms absent from the corpus contribute nothing and yield no candidates");

    {
        auto results = engine.search("cat dog", 10, chunks, index);
        check(results.size() == 4, "candidates are the union of chunks matching any query term");
        check(results.size() == 4 && results[0].chunk_id == "p#0" && results[1].chunk_id == "q#0" &&
                  results[2].chunk_id == "p#1" && results[3].chunk_id == "q#1",
              "higher term frequency and more matched terms ranks a chunk first");
        check(results.size() == 4 && close(results[0].score, 4.159313066162) &&
                  close(results[1].score, 2.695249291973) &&
                  close(results[2].score, 1.586366904954) &&
                  close(results[3].score, 1.284300728880),
              "scores match the hand-derived TF-IDF/coverage formula");
        check(!results.empty() && results[0].matched_terms == 2,
              "matched_terms reports the number of distinct query terms present in the chunk");

        bool all_canonical = true;
        for (const SearchResult& r : results) {
            if (r.score != std::round(r.score * 1e12) / 1e12) {
                all_canonical = false;
            }
        }
        check(all_canonical, "every returned score already sits on the 12-decimal rounding grid");
    }

    {
        auto results = engine.search("cat dog", 2, chunks, index);
        check(results.size() == 2 && results[0].chunk_id == "p#0" && results[1].chunk_id == "q#0",
              "k truncates to the best k results");
    }

    {
        // Three chunks with identical single-term content -> identical scores,
        // so ordering must fall back to document_order then sequence.
        std::vector<Chunk> tie_chunks{
            make_chunk("a#0", "a", 1, 0, "apple"),
            make_chunk("a#1", "a", 1, 1, "apple"),
            make_chunk("b#0", "b", 0, 0, "apple"),
        };
        CorpusIndex tie_index(tie_chunks);
        auto results = engine.search("apple", 10, tie_chunks, tie_index);
        check(results.size() == 3 && results[0].chunk_id == "b#0" &&
                  results[1].chunk_id == "a#0" && results[2].chunk_id == "a#1",
              "ties are broken by ascending document_order, then ascending sequence");
    }

    if (failures == 0) {
        std::cout << "All retrieval_engine tests passed.\n";
        return 0;
    }
    std::cerr << failures << " retrieval_engine test(s) failed.\n";
    return 1;
}
