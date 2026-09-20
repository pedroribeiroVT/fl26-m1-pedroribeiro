#include "aiws/text_processor.hpp"

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
}

int main() {
    using namespace aiws;

    check(TextProcessor::normalize("Hello, WORLD! 2026") == "hello world 2026",
          "normalize lowercases letters, strips punctuation, keeps digits");
    check(TextProcessor::normalize("R2-D2") == "r2 d2",
          "normalize splits on the boundary between letters/digits and separators");
    check(TextProcessor::normalize("...\t---").empty(),
          "normalize of punctuation-only input is empty");
    check(TextProcessor::normalize("").empty(),
          "normalize of empty input is empty");

    auto terms = TextProcessor::terms("a,,,b");
    check(terms.size() == 2 && terms[0] == "a" && terms[1] == "b",
          "consecutive separators do not create empty tokens");

    auto tokens = TextProcessor::tokenize("ab cd");
    check(tokens.size() == 2, "tokenize splits on whitespace");
    check(tokens[0].token == "ab" && tokens[0].begin == 0 && tokens[0].end == 2,
          "tokenize records the first token's source byte offsets");
    check(tokens[1].token == "cd" && tokens[1].begin == 3 && tokens[1].end == 5,
          "tokenize records the second token's source byte offsets");

    auto lf_tokens = TextProcessor::tokenize("alpha\n\nbeta");
    check(lf_tokens.size() == 2 && lf_tokens[0].paragraph == 0 && lf_tokens[1].paragraph == 1,
          "a blank line (LF) starts a new paragraph");

    auto crlf_tokens = TextProcessor::tokenize("alpha\r\n\r\nbeta");
    check(crlf_tokens.size() == 2 && crlf_tokens[0].paragraph == 0 && crlf_tokens[1].paragraph == 1,
          "a blank line (CRLF) starts a new paragraph, consistent with LF");

    auto single_newline_tokens = TextProcessor::tokenize("alpha\nbeta");
    check(single_newline_tokens.size() == 2 &&
              single_newline_tokens[0].paragraph == 0 && single_newline_tokens[1].paragraph == 0,
          "a single newline does not start a new paragraph");

    std::vector<std::string> words{"a", "b", "c"};
    check(TextProcessor::join(words, 0, 2) == "a b",
          "join reconstructs a partial token range with single spaces");

    if (failures == 0) {
        std::cout << "All text_processor tests passed.\n";
        return 0;
    }
    std::cerr << failures << " text_processor test(s) failed.\n";
    return 1;
}
