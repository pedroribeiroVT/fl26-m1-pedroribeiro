#include "aiws/text_processor.hpp"

namespace aiws {

namespace {
    bool is_word_byte(unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }
}

std::vector<TokenInfo> TextProcessor::tokenize(const std::string& text) {
    std::vector<TokenInfo> tokens;
    std::size_t start = 0;
    std::size_t paragraph = 0;
    bool curr_token = false;

    auto flush = [&](std::size_t end) {
        std::string word = text.substr(start, end - start);
        for (char& c : word) {
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }
        tokens.push_back(TokenInfo{word, start, end, paragraph});
        curr_token = false;
    };

    for (std::size_t i = 0; i < text.size(); i++) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (is_word_byte(c)) {
            if (!curr_token) { start = i; curr_token = true; }
        } else {
            if (curr_token) flush(i);
            if (c == '\n') {
                std::size_t j = i + 1;
                while (j < text.size() && (text[j]==' '||text[j]=='\t'||text[j]=='\r')) j++;
                if (j < text.size() && text[j] == '\n') {
                    ++paragraph;
                    i = j;
                }
            }
        }
    }

    if (curr_token) flush(text.size());
    return tokens;
}

std::vector<std::string> TextProcessor::terms(const std::string& text) {
    std::vector<std::string> terms;
    for (const TokenInfo& info : tokenize(text)) {
        terms.push_back(info.token);
    }
    return terms;
}

std::string TextProcessor::normalize(const std::string& text) {
    std::vector<std::string> tokens = terms(text);
    return join(tokens, 0, tokens.size());
}

std::string TextProcessor::join(const std::vector<TokenInfo>& info,
                                std::size_t start,
                                std::size_t end) {
    std::string text;
    for (size_t i = start; i < end && i < info.size(); i++) {
        if (!text.empty()) { text += ' '; }
        text += info[i].token;
    }
    return text;
}

std::string TextProcessor::join(const std::vector<std::string>& tokens,
                                std::size_t start,
                                std::size_t end) {
    std::string text;
    for (size_t i = start; i < end && i < tokens.size(); i++) {
        if (!text.empty()) { text += ' '; }
        text += tokens[i];
    }
    return text;
}

}  // namespace aiws
