#include "aiws/processing_core.hpp"
#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <unordered_set>

namespace aiws {

struct ProcessingCore::Impl {
    Chunker chunker;
    std::vector<Chunk> chunks;
    CorpusIndex index;
};

ProcessingCore::ProcessingCore() : impl_(std::make_unique<Impl>()) { }

ProcessingCore::~ProcessingCore() = default;

ProcessingCore::ProcessingCore(ProcessingCore&&) noexcept = default;

ProcessingCore& ProcessingCore::operator=(ProcessingCore&&) noexcept = default;

std::string ProcessingCore::normalize(const std::string& text) {
    return TextProcessor::normalize(text);
}

void ProcessingCore::rebuild(const Workspace& workspace) {
    std::unordered_set<std::string> seen_ids;
    for (const Document& document : workspace.documents()) {
        if (!seen_ids.insert(document.id()).second) {
            throw std::invalid_argument("duplicate document id: " + document.id());
        }
    }

    std::vector<Chunk> new_chunks;
    const std::vector<Document>& documents = workspace.documents();
    for (std::size_t i = 0; i < documents.size(); i++) {
        std::vector<Chunk> doc_chunks = impl_->chunker.chunk(documents[i], i);
        new_chunks.insert(new_chunks.end(), doc_chunks.begin(), doc_chunks.end());
    }
    CorpusIndex new_index(new_chunks);

    impl_->chunks = std::move(new_chunks);
    impl_->index = std::move(new_index);
}

const std::vector<Chunk>& ProcessingCore::chunks() const noexcept {
    return impl_->chunks;
}

std::size_t ProcessingCore::chunk_count() const noexcept {
    return impl_->chunks.size();
}

std::size_t ProcessingCore::document_frequency(const std::string& term) const {
    std::vector<std::string> tokens = TextProcessor::terms(term);
    if (tokens.empty()) {
        return 0;
    }
    if (tokens.size() > 1) {
        throw std::invalid_argument("term must normalize to a single token");
    }
    return impl_->index.document_frequency(tokens[0]);
}

std::size_t ProcessingCore::term_frequency(const std::string& term,
                                           const std::string& chunk_id) const {
    std::vector<std::string> tokens = TextProcessor::terms(term);
    if (tokens.empty()) {
        return 0;
    }
    if (tokens.size() > 1) {
        throw std::invalid_argument("term must normalize to a single token");
    }
    return impl_->index.term_frequency(tokens[0], chunk_id);
}

std::vector<SearchResult> ProcessingCore::search(const std::string& query, int k) const {
    return RetrievalEngine().search(query, k, impl_->chunks, impl_->index);
}

std::vector<ContextItem> ProcessingCore::build_context(const std::string& query,
                                                       int k,
                                                       std::size_t token_budget) const {
    std::vector<SearchResult> ranked = search(query, k);
    return ContextBuilder().build(ranked, token_budget);
}

}  // namespace aiws
