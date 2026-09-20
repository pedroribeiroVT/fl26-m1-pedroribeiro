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

    RetrievalEngine engine;

    std::vector<Chunk> basic{
        make_chunk("a#0", "a", 0, 0, "cat cat dog"),
        make_chunk("b#0", "b", 1, 0, "dog"),
    };
    CorpusIndex basic_index(basic);

    bool threw = false;
    try {
        (void)engine.search("cat", -1, basic, basic_index);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "negative k throws invalid_argument");

    check(engine.search("cat", 0, basic, basic_index).empty(), "k == 0 returns no results");
    check(engine.search("...---", 10, basic, basic_index).empty(),
          "a punctuation-only query returns no results");
    check(engine.search("elephant", 10, basic, basic_index).empty(),
          "a term absent from the corpus returns no results");

    {
        // "a" matches both query terms with higher frequency, "b" matches only one.
        std::vector<Chunk> chunks{
            make_chunk("a#0", "a", 0, 0, "cat cat dog"),
            make_chunk("b#0", "b", 1, 0, "cat"),
        };
        CorpusIndex index(chunks);
        auto results = engine.search("cat dog", 10, chunks, index);

        check(results.size() == 2, "both chunks are candidates");
        check(!results.empty() && results[0].chunk_id == "a#0",
              "more matched terms and higher frequency ranks a chunk first");
        check(results.size() == 2 && results[0].score > results[1].score,
              "the higher-ranked chunk has the higher score");
        check(!results.empty() && results[0].matched_terms == 2,
              "matched_terms counts the distinct query terms present in the chunk");
    }

    {
        std::vector<Chunk> chunks{
            make_chunk("a#0", "a", 0, 0, "cat"),
            make_chunk("b#0", "b", 1, 0, "cat"),
            make_chunk("c#0", "c", 2, 0, "cat"),
        };
        CorpusIndex index(chunks);
        auto results = engine.search("cat", 2, chunks, index);
        check(results.size() == 2, "k truncates to the best k results");
    }

    {
        std::vector<Chunk> chunks{
            make_chunk("a#0", "a", 1, 0, "apple"),
            make_chunk("a#1", "a", 1, 1, "apple"),
            make_chunk("b#0", "b", 0, 0, "apple"),
        };
        CorpusIndex index(chunks);
        auto results = engine.search("apple", 10, chunks, index);

        check(results.size() == 3, "all three chunks match");
        check(results.size() == 3 && results[0].chunk_id == "b#0",
              "lower document_order comes first");
        check(results.size() == 3 && results[1].chunk_id == "a#0" && results[2].chunk_id == "a#1",
              "equal document_order falls back to ascending sequence");
    }

    {
        auto results = engine.search("cat dog", 10, basic, basic_index);
        check(!results.empty() && results[0].score == std::round(results[0].score * 1e12) / 1e12,
              "the score is already rounded to 12 decimal places");
    }

    if (failures == 0) {
        std::cout << "All retrieval_engine tests passed.\n";
        return 0;
    }
    std::cerr << failures << " retrieval_engine test(s) failed.\n";
    return 1;
}
