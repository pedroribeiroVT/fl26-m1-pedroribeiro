#include "aiws/processing_core.hpp"

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
}

int main() {
    using namespace aiws;

    ProcessingCore core;

    {
        Workspace ws;
        ws.add_document(Document{"x", "T", "hello world"});
        core.rebuild(ws);

        bool df_threw = false;
        try {
            (void)core.document_frequency("hello world");
        } catch (const std::invalid_argument&) {
            df_threw = true;
        }
        check(df_threw, "document_frequency throws for a term that normalizes to more than one token");

        bool tf_threw = false;
        try {
            (void)core.term_frequency("hello world", "x#0");
        } catch (const std::invalid_argument&) {
            tf_threw = true;
        }
        check(tf_threw, "term_frequency throws for a term that normalizes to more than one token");

        check(core.document_frequency("...") == 0,
              "document_frequency of a term that normalizes to no tokens is zero");
    }

    {
        Workspace ws;
        ws.add_document(Document{"dup", "T", "alpha"});
        ws.add_document(Document{"dup", "T", "beta"});
        bool threw = false;
        try {
            core.rebuild(ws);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "rebuild rejects a workspace with duplicate document ids");
    }

    {
        Workspace valid_ws;
        valid_ws.add_document(Document{"keep", "T", "hello"});
        core.rebuild(valid_ws);
        std::size_t count_before = core.chunk_count();
        std::string text_before = count_before > 0 ? core.chunks()[0].text : "";

        Workspace bad_ws;
        bad_ws.add_document(Document{"dup", "T", "one"});
        bad_ws.add_document(Document{"dup", "T", "two"});
        bool threw = false;
        try {
            core.rebuild(bad_ws);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "the failing rebuild still throws");
        check(core.chunk_count() == count_before &&
                  (core.chunks().empty() || core.chunks()[0].text == text_before),
              "a failed rebuild leaves the previously valid corpus unchanged");
    }

    {
        Workspace ws_a;
        ws_a.add_document(Document{"d1", "T", "zeta"});
        ws_a.add_document(Document{"d2", "T", "eta"});
        core.rebuild(ws_a);
        check(core.document_frequency("zeta") == 1, "zeta is indexed after the first rebuild");

        Workspace ws_b;
        ws_b.add_document(Document{"d3", "T", "eta"});
        core.rebuild(ws_b);
        check(core.document_frequency("zeta") == 0 && core.document_frequency("eta") == 1,
              "repeated rebuild does not leak terms from documents that were removed");
    }

    {
        Workspace ws;
        ws.add_document(Document{"a", "T", "cat cat dog"});
        ws.add_document(Document{"b", "T", "cat bird"});
        ws.add_document(Document{"c", "T", "dog dog dog bird"});
        core.rebuild(ws);

        auto results = core.search("cat dog", 10);
        check(results.size() == 3 && results[0].document_id == "a" &&
                  results[1].document_id == "c" && results[2].document_id == "b",
              "search results span multiple documents in the expected ranked order");
        check(results.size() == 3 && close(results[0].score, 3.814709077169) &&
                  close(results[1].score, 2.837462692202) &&
                  close(results[2].score, 1.352066176074),
              "end-to-end scores match the hand-derived TF-IDF/coverage formula");

        auto ctx = core.build_context("cat dog", 10, 5);
        check(ctx.size() == 2 && ctx[0].document_id == "a" && !ctx[0].truncated &&
                  ctx[0].token_count == 3,
              "the first, fully-fitting chunk is included whole");
        check(ctx.size() == 2 && ctx[1].document_id == "c" && ctx[1].truncated &&
                  ctx[1].token_count == 2,
              "the next chunk is truncated to the remaining budget and processing stops there");

        auto results_again = core.search("cat dog", 10);
        bool identical = results.size() == results_again.size();
        for (std::size_t i = 0; identical && i < results.size(); i++) {
            identical = results[i].chunk_id == results_again[i].chunk_id &&
                        results[i].score == results_again[i].score;
        }
        check(identical, "repeating the same query is deterministic");
    }

    if (failures == 0) {
        std::cout << "All processing_core tests passed.\n";
        return 0;
    }
    std::cerr << failures << " processing_core test(s) failed.\n";
    return 1;
}
