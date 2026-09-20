#include "aiws/chunker.hpp"
#include "aiws/text_processor.hpp"
#include <stdexcept>

namespace aiws {

Chunker::Chunker(ChunkingPolicy policy) : policy_(policy) {
    if (policy_.max_tokens == 0 || policy_.overlap >= policy_.max_tokens ||
        policy_.paragraph_window > policy_.max_tokens) {
        throw std::invalid_argument("invalid chunking policy");
    }
}

std::vector<Chunk> Chunker::chunk(const Document& document,
                                    std::size_t document_order) const {
    std::vector<TokenInfo> token_info = TextProcessor::tokenize(document.text());
    if (token_info.empty()) {
        return {};
    }

    std::size_t start = 0;
    std::size_t sequence = 0;
    std::vector<Chunk> chunks;

    auto make_chunk = [&](std::size_t end) {
        Chunk c;
        c.id = document.id() + "#" + std::to_string(sequence);
        c.document_id = document.id();
        c.document_order = document_order;
        c.sequence = sequence;
        c.text = TextProcessor::join(token_info, start, end);
        c.token_count = end - start;
        c.source_begin = token_info[start].begin;
        c.source_end = token_info[end - 1].end;
        return c;
    };

    while (start < token_info.size()) {
        std::size_t remaining = token_info.size() - start;
        std::size_t end;

        if (remaining <= policy_.max_tokens) {
            end = token_info.size();
        } else {
            std::size_t window_begin = start + policy_.max_tokens - policy_.paragraph_window;
            std::size_t window_end = start + policy_.max_tokens;
            end = window_end;
            for (std::size_t k = window_end + 1; k-- > window_begin; ) {
                if (token_info[k - 1].paragraph != token_info[k].paragraph) {
                    end = k;
                    break;
                }
            }
        }

        chunks.push_back(make_chunk(end));
        sequence++;

        if (end == token_info.size()) {
            break;
        }
        start = end - policy_.overlap;
    }

    return chunks;
}

}  // namespace aiws
