#include "aiws/context_builder.hpp"
#include "aiws/text_processor.hpp"

namespace aiws {

std::vector<ContextItem> ContextBuilder::build(const std::vector<SearchResult>& ranked,
                                               std::size_t token_budget) const {
    std::vector<ContextItem> items;
    std::size_t remaining = token_budget;

    for (const SearchResult& result : ranked) {
        if (remaining == 0) {
            break;
        }

        std::vector<std::string> tokens = TextProcessor::terms(result.text);
        ContextItem item;
        item.chunk_id = result.chunk_id;
        item.document_id = result.document_id;
        item.chunk_sequence = result.chunk_sequence;
        item.score = result.score;

        if (tokens.size() <= remaining) {
            item.text = result.text;
            item.token_count = tokens.size();
            item.truncated = false;
            remaining -= tokens.size();
            items.push_back(std::move(item));
            continue;
        }

        item.text = TextProcessor::join(tokens, 0, remaining);
        item.token_count = remaining;
        item.truncated = true;
        items.push_back(std::move(item));
        break;
    }

    return items;
}

}  // namespace aiws
