#include "aiws/chunker.hpp"

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

std::string numbered_words(int n) {
    std::string s;
    for (int i = 0; i < n; ++i) {
        if (!s.empty()) s += ' ';
        s += "w" + std::to_string(i);
    }
    return s;
}

std::string two_paragraph_words(int first, int total) {
    std::string s = numbered_words(first);
    s += "\n\n";
    for (int i = first; i < total; ++i) {
        s += "w" + std::to_string(i) + " ";
    }
    return s;
}
}

int main() {
    using namespace aiws;

    Chunker chunker;

    check(chunker.chunk(Document{"d", "T", ""}, 0).empty(),
          "empty document produces no chunks");
    check(chunker.chunk(Document{"d", "T", "...---"}, 0).empty(),
          "punctuation-only document produces no chunks");

    {
        std::string text = "alpha beta gamma";
        auto chunks = chunker.chunk(Document{"d", "T", text}, 0);
        check(chunks.size() == 1, "short document produces exactly one chunk");
        check(!chunks.empty() && chunks[0].token_count == 3,
              "short chunk contains every token");
        check(!chunks.empty() && chunks[0].source_begin == 0 &&
                  chunks[0].source_end == text.size(),
              "short chunk's source span covers the whole document text");
    }

    {
        auto chunks = chunker.chunk(Document{"d", "T", numbered_words(200)}, 0);
        check(chunks.size() >= 2 && chunks[0].id == "d#0" && chunks[1].id == "d#1",
              "chunk ids use <document-id>#<sequence>");
    }

    {
        auto chunks = chunker.chunk(Document{"d", "T", numbered_words(121)}, 0);
        check(chunks.size() == 2, "121 single-paragraph tokens split into two chunks");
        check(chunks.size() == 2 && chunks[0].token_count == 120 && chunks[1].token_count == 21,
              "hard 120-token limit applies when no paragraph boundary qualifies");
        check(chunks.size() == 2 && chunks[0].text.substr(chunks[0].text.rfind(' ') + 1) == "w119",
              "the first chunk's last token is index 119 (120 tokens, 0-indexed)");
        check(chunks.size() == 2 && chunks[1].text.substr(0, chunks[1].text.find(' ')) == "w100",
              "the next chunk begins 20 tokens before the previous chunk's end");
    }

    {
        // paragraph boundary at token index 110, inside the [100,120] window.
        auto chunks = chunker.chunk(Document{"d", "T", two_paragraph_words(110, 130)}, 0);
        check(!chunks.empty() && chunks[0].token_count == 110,
              "a paragraph boundary inside the [100,120] window is preferred over the hard limit");
    }

    {
        // paragraph boundary at token index 50, outside the [100,120] window.
        auto chunks = chunker.chunk(Document{"d", "T", two_paragraph_words(50, 140)}, 0);
        check(!chunks.empty() && chunks[0].token_count == 120,
              "a paragraph boundary outside the [100,120] window is ignored");
    }

    {
        Document doc{"d", "T", numbered_words(121)};
        auto first = chunker.chunk(doc, 0);
        auto second = chunker.chunk(doc, 0);
        check(first.size() == second.size(), "reprocessing yields the same chunk count");
        bool identical = first.size() == second.size();
        for (std::size_t i = 0; identical && i < first.size(); i++) {
            identical = first[i].id == second[i].id && first[i].text == second[i].text &&
                        first[i].source_begin == second[i].source_begin &&
                        first[i].source_end == second[i].source_end;
        }
        check(identical, "reprocessing the same document is deterministic");
    }

    if (failures == 0) {
        std::cout << "All chunker tests passed.\n";
        return 0;
    }
    std::cerr << failures << " chunker test(s) failed.\n";
    return 1;
}
