#include "aiws/corpus_index.hpp"

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
}

int main() {
    using namespace aiws;

    Chunk c0;
    c0.id = "d1#0";
    c0.document_id = "d1";
    c0.document_order = 0;
    c0.sequence = 0;
    c0.text = "alpha alpha alpha beta";
    c0.token_count = 4;

    Chunk c1;
    c1.id = "d1#1";
    c1.document_id = "d1";
    c1.document_order = 0;
    c1.sequence = 1;
    c1.text = "alpha gamma";
    c1.token_count = 2;

    Chunk c2;
    c2.id = "d2#0";
    c2.document_id = "d2";
    c2.document_order = 1;
    c2.sequence = 0;
    c2.text = "beta beta";
    c2.token_count = 2;

    std::vector<Chunk> chunks{c0, c1, c2};
    CorpusIndex index(chunks);

    check(index.document_frequency("alpha") == 2,
          "document frequency counts distinct chunks containing the term");
    check(index.term_frequency("alpha", "d1#0") == 3,
          "term frequency counts occurrences within one chunk");
    check(index.term_frequency("alpha", "missing#9") == 0,
          "term frequency is zero for a missing chunk id");
    check(index.document_frequency("delta") == 0,
          "document frequency is zero for an unknown term");
    check(index.postings("delta") == nullptr,
          "postings is null for an unknown term");

    const std::vector<CorpusIndex::Posting>* beta_postings = index.postings("beta");
    check(beta_postings != nullptr && beta_postings->size() == 2,
          "postings has one entry per chunk containing the term");
    if (beta_postings != nullptr) {
        std::size_t freq_in_c0 = 0;
        std::size_t freq_in_c2 = 0;
        for (const CorpusIndex::Posting& posting : *beta_postings) {
            if (posting.chunk_index == 0) freq_in_c0 = posting.frequency;
            if (posting.chunk_index == 2) freq_in_c2 = posting.frequency;
        }
        check(freq_in_c0 == 1 && freq_in_c2 == 2,
              "each posting records the correct per-chunk frequency");
    }

    const Chunk* found = index.find_chunk(chunks, "d1#1");
    check(found != nullptr && found->text == "alpha gamma",
          "find_chunk locates the chunk matching the requested id");
    check(index.find_chunk(chunks, "nope#0") == nullptr,
          "find_chunk returns null for an unknown id");

    bool threw = false;
    try {
        (void)index.chunk_index("nope#0");
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "chunk_index throws out_of_range for an unknown id");

    Chunk z;
    z.id = "z#0";
    z.document_id = "z";
    z.text = "zeta";
    z.token_count = 1;
    std::vector<Chunk> other_chunks{z};
    index.build(other_chunks);
    check(index.document_frequency("alpha") == 0 && index.document_frequency("zeta") == 1,
          "rebuilding replaces the previous corpus without accumulating stale state");

    if (failures == 0) {
        std::cout << "All corpus_index tests passed.\n";
        return 0;
    }
    std::cerr << failures << " corpus_index test(s) failed.\n";
    return 1;
}
