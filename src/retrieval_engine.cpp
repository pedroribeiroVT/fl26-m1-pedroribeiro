#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace aiws {

double RetrievalEngine::canonical_score(double value) {
    constexpr double kScale = 1e12;
    return std::round(value * kScale) / kScale;
}

std::vector<SearchResult> RetrievalEngine::search(const std::string& query,
                                                   int k,
                                                   const std::vector<Chunk>& chunks,
                                                   const CorpusIndex& index) const {
    if (k < 0) {
        throw std::invalid_argument("k must not be negative");
    }

    std::vector<std::string> query_terms;
    std::unordered_set<std::string> seen_terms;
    for (const std::string& term : TextProcessor::terms(query)) {
        if (seen_terms.insert(term).second) {
            query_terms.push_back(term);
        }
    }
    if (query_terms.empty() || k == 0) {
        return {};
    }

    struct Accumulator {
        double base{};
        std::size_t matched{};
    };
    std::unordered_map<std::size_t, Accumulator> accumulators;

    std::size_t total_chunks = chunks.size();
    for (const std::string& term : query_terms) {
        const std::vector<CorpusIndex::Posting>* postings = index.postings(term);
        if (postings == nullptr) {
            continue;
        }
        std::size_t df = index.document_frequency(term);
        double idf = std::log(static_cast<double>(total_chunks + 1) /
                               static_cast<double>(df + 1)) + 1.0;
        for (const CorpusIndex::Posting& posting : *postings) {
            double tf = 1.0 + std::log(static_cast<double>(posting.frequency));
            Accumulator& acc = accumulators[posting.chunk_index];
            acc.base += tf * idf;
            acc.matched++;
        }
    }

    std::size_t query_term_count = query_terms.size();
    std::vector<std::pair<std::size_t, SearchResult>> candidates;
    candidates.reserve(accumulators.size());
    for (const auto& [chunk_index, acc] : accumulators) {
        const Chunk& c = chunks[chunk_index];
        double coverage = 1.0 + 0.10 * static_cast<double>(acc.matched) /
                                     static_cast<double>(query_term_count);
        SearchResult result;
        result.chunk_id = c.id;
        result.document_id = c.document_id;
        result.chunk_sequence = c.sequence;
        result.text = c.text;
        result.score = canonical_score(acc.base * coverage);
        result.matched_terms = acc.matched;
        candidates.push_back({chunk_index, result});
    }

    std::sort(candidates.begin(), candidates.end(),
              [&chunks](const auto& a, const auto& b) {
        if (a.second.score != b.second.score) {
            return a.second.score > b.second.score;
        }
        const Chunk& ca = chunks[a.first];
        const Chunk& cb = chunks[b.first];
        if (ca.document_order != cb.document_order) {
            return ca.document_order < cb.document_order;
        }
        return ca.sequence < cb.sequence;
    });

    if (candidates.size() > static_cast<std::size_t>(k)) {
        candidates.resize(static_cast<std::size_t>(k));
    }

    std::vector<SearchResult> results;
    results.reserve(candidates.size());
    for (auto& [chunk_index, result] : candidates) {
        results.push_back(std::move(result));
    }
    return results;
}

}  // namespace aiws
