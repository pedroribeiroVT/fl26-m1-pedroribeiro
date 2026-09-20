#include "aiws/corpus_index.hpp"
#include "aiws/text_processor.hpp"

namespace aiws {

CorpusIndex::CorpusIndex(const std::vector<Chunk>& chunks) {
    build(chunks);
}

void CorpusIndex::build(const std::vector<Chunk>& chunks) {
    postings_.clear();
    chunk_by_id_.clear();

    auto count_terms = [](const std::string& text) {
        std::unordered_map<std::string, std::size_t> counts;
        for (const std::string& term : TextProcessor::terms(text)) {
            counts[term]++;
        }
        return counts;
    };

    for (std::size_t i = 0; i < chunks.size(); i++) {
        chunk_by_id_[chunks[i].id] = i;
        for (const auto& [term, freq] : count_terms(chunks[i].text)) {
            postings_[term].push_back(Posting{i, freq});
        }
    }
}

std::size_t CorpusIndex::document_frequency(
    const std::string& normalized_term) const noexcept {
    auto it = postings_.find(normalized_term);
    if (it == postings_.end()) {
        return 0;
    }
    return it->second.size();
}

std::size_t CorpusIndex::term_frequency(
    const std::string& normalized_term,
    const std::string& chunk_id) const noexcept {
    auto chunk_it = chunk_by_id_.find(chunk_id);
    if (chunk_it == chunk_by_id_.end()) {
        return 0;
    }

    auto term_it = postings_.find(normalized_term);
    if (term_it == postings_.end()) {
        return 0;
    }

    for (const Posting& posting : term_it->second) {
        if (posting.chunk_index == chunk_it->second) {
            return posting.frequency;
        }
    }
    return 0;
}

const std::vector<CorpusIndex::Posting>* CorpusIndex::postings(
    const std::string& normalized_term) const noexcept {
    auto it = postings_.find(normalized_term);
    if (it == postings_.end()) {
        return nullptr;
    }
    return &it->second;
}

const Chunk* CorpusIndex::find_chunk(
    const std::vector<Chunk>& chunks,
    const std::string& chunk_id) const noexcept {
    auto it = chunk_by_id_.find(chunk_id);
    if (it == chunk_by_id_.end()) {
        return nullptr;
    }
    return &chunks[it->second];
}

std::size_t CorpusIndex::chunk_index(const std::string& chunk_id) const {
    return chunk_by_id_.at(chunk_id);
}

}  // namespace aiws
