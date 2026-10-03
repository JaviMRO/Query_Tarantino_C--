#include "indexer/tests/benchmarking/support/IndexFixture.hpp"

#include "crawler/tests/benchmarking/support/environment/BenchmarkPaths.hpp"
#include "tarantino/crawler/model/book/GutenbergText.hpp"
#include "tarantino/indexer/adapters/stopwords/FileStopwordsLoader.hpp"

namespace tarantino::indexer::benchmarking {

using crawler::benchmarking::BenchmarkDataset;
using crawler::benchmarking::BenchmarkPaths;

IndexFixture::IndexFixture(BenchmarkDataset dataset, model::Tokenizer tokenizer)
    : dataset_(std::move(dataset)), tokenizer_(std::move(tokenizer)) {
}

IndexFixture IndexFixture::fromEnvironment() {
    const std::filesystem::path stopwords = BenchmarkPaths::workload() / "stopwords.txt";
    return IndexFixture(BenchmarkDataset::fromEnvironment(), model::Tokenizer(adapters::FileStopwordsLoader(stopwords).stopwords()));
}

BenchmarkDataset& IndexFixture::dataset() {
    return dataset_;
}

void IndexFixture::index(ports::InvertedIndexStorage& invertedIndex, const std::vector<int>& ids) {
    add(invertedIndex, ids);
    invertedIndex.flush();
}

void IndexFixture::add(ports::InvertedIndexStorage& invertedIndex, const std::vector<int>& ids) {
    for (const int bookId : ids) {
        invertedIndex.add(occurrences(bookId));
    }
}

void IndexFixture::save(ports::MetadataStorage& metadata, const std::vector<model::Book>& books) {
    for (const model::Book& book : books) {
        metadata.save(book);
    }
}

std::vector<model::Book> IndexFixture::books(const std::vector<int>& ids) {
    std::vector<model::Book> parsed;
    parsed.reserve(ids.size());
    for (const int bookId : ids) {
        parsed.push_back(headerParser_.book(bookText(bookId)));
    }
    return parsed;
}

std::uint64_t IndexFixture::vocabularySize(const std::vector<int>& ids) {
    platform::StringSet vocabulary;
    for (const int bookId : ids) {
        for (auto& [term, frequency] : occurrences(bookId).frequencies) {
            vocabulary.insert(term);
        }
    }
    return vocabulary.size();
}

platform::StringSet IndexFixture::terms(int bookId) {
    platform::StringSet terms;
    for (auto& [term, frequency] : occurrences(bookId).frequencies) {
        terms.insert(term);
    }
    return terms;
}

model::TermOccurrences IndexFixture::occurrences(int bookId) {
    return tokenizer_.occurrences(bookId, bookText(bookId).body);
}

model::BookText IndexFixture::bookText(int bookId) {
    crawler::model::BookText split = crawler::model::GutenbergText::bookText(bookId, dataset_.rawText(bookId));
    return model::BookText{bookId, std::move(split.header), std::move(split.body),
                           std::filesystem::path("datalake") / (std::to_string(bookId) + ".body.txt")};
}

}
