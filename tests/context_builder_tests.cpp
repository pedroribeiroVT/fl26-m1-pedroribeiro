#include "aiws/context_builder.hpp"

#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

aiws::SearchResult make_result(const std::string& chunk_id, const std::string& document_id,
                               std::size_t chunk_sequence, const std::string& text,
                               double score) {
    aiws::SearchResult r;
    r.chunk_id = chunk_id;
    r.document_id = document_id;
    r.chunk_sequence = chunk_sequence;
    r.text = text;
    r.score = score;
    return r;
}
}

int main() {
    using namespace aiws;

    ContextBuilder builder;

    {
        std::vector<SearchResult> ranked{make_result("d#0", "d", 0, "alpha beta gamma", 1.0)};
        check(builder.build(ranked, 0).empty(), "a zero token budget returns no context items");
    }

    {
        std::vector<SearchResult> ranked{make_result("d#0", "d", 0, "alpha beta gamma", 1.0)};
        auto items = builder.build(ranked, 10);
        check(items.size() == 1 && items[0].token_count == 3 && !items[0].truncated &&
                  items[0].text == "alpha beta gamma",
              "a chunk that fully fits the budget is included untruncated");
    }

    {
        std::vector<SearchResult> ranked{make_result("d#0", "d", 0, "alpha beta gamma", 1.0)};
        auto items = builder.build(ranked, 3);
        check(items.size() == 1 && items[0].token_count == 3 && !items[0].truncated,
              "a chunk whose token count exactly equals the remaining budget is not truncated");
    }

    {
        std::vector<SearchResult> ranked{
            make_result("d#0", "d", 0, "alpha beta gamma", 2.0),
            make_result("d#1", "d", 1, "delta epsilon", 1.0),
        };
        auto items = builder.build(ranked, 2);
        check(items.size() == 1 && items[0].chunk_id == "d#0" && items[0].truncated &&
                  items[0].token_count == 2 && items[0].text == "alpha beta",
              "a chunk that doesn't fully fit is truncated to the largest prefix and processing stops");
    }

    {
        std::vector<SearchResult> ranked{
            make_result("d#0", "d", 0, "w0 w1 w2", 3.0),
            make_result("d#1", "d", 1, "w3 w4 w5", 2.0),
            make_result("d#2", "d", 2, "w6 w7 w8", 1.0),
        };
        auto items = builder.build(ranked, 6);
        check(items.size() == 2 && !items[0].truncated && !items[1].truncated,
              "chunks accumulate whole while they fit exactly within the budget");
        check(items.size() == 2,
              "a chunk reached after the budget is exactly exhausted is excluded outright");
    }

    {
        std::vector<SearchResult> ranked{make_result("d#5", "doc", 5, "alpha beta", 4.5)};
        auto items = builder.build(ranked, 10);
        check(items.size() == 1 && items[0].chunk_id == "d#5" && items[0].document_id == "doc" &&
                  items[0].chunk_sequence == 5 && items[0].score == 4.5,
              "context items preserve source attribution from the search result");
    }

    if (failures == 0) {
        std::cout << "All context_builder tests passed.\n";
        return 0;
    }
    std::cerr << failures << " context_builder test(s) failed.\n";
    return 1;
}
