#include "tarantino/indexer/commands/IndexBookCommand.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <map>

namespace tarantino::indexer::commands {
namespace {

using model::Book;
using model::BookText;
using model::TermOccurrences;

const std::filesystem::path BODY = std::filesystem::path("datalake") / "1342" / "body.txt";

class FunctionDatalake : public ports::DatalakeReader {
public:
    explicit FunctionDatalake(std::function<std::optional<BookText>(int)> books) : books_(std::move(books)) {
    }

    std::optional<BookText> bookText(int bookId) override {
        return books_(bookId);
    }

private:
    std::function<std::optional<BookText>(int)> books_;
};

class RecordingIndex : public ports::InvertedIndexStorage {
public:
    void add(const TermOccurrences& occurrences) override {
        added.push_back(occurrences);
    }

    void flush() override {
        ++flushes;
    }

    std::vector<TermOccurrences> added;
    int flushes = 0;
};

class RecordingMetadata : public ports::MetadataStorage {
public:
    void save(const Book& book) override {
        saved.push_back(book);
    }

    std::vector<Book> saved;
};

class IndexBookCommandTest : public ::testing::Test {
protected:
    IndexBookCommand command(std::function<std::optional<BookText>(int)> books) {
        return IndexBookCommand(std::make_shared<FunctionDatalake>(std::move(books)), model::HeaderParser(),
                                model::Tokenizer(platform::StringSet{"the"}), index, metadata);
    }

    std::shared_ptr<RecordingIndex> index = std::make_shared<RecordingIndex>();
    std::shared_ptr<RecordingMetadata> metadata = std::make_shared<RecordingMetadata>();
};

TEST_F(IndexBookCommandTest, savesMetadataAndFlushesTermsOfStoredBooks) {
    const BookText text{1342, "Title: Pride\nAuthor: Austen", "The island, the island!", BODY};

    const IndexResult result = command([&](int) { return std::optional(text); }).execute(1342);

    EXPECT_EQ(IndexResult::success(1342, 1), result);
    EXPECT_EQ((std::vector<Book>{Book{1342, "Pride", "Austen", std::nullopt, BODY}}), metadata->saved);
    EXPECT_EQ((std::vector<TermOccurrences>{TermOccurrences{1342, {{"island", 2}}}}), index->added);
    EXPECT_EQ(1, index->flushes);
}

TEST_F(IndexBookCommandTest, reportsBooksMissingFromTheDatalakeWithoutWriting) {
    EXPECT_EQ(IndexResult::notFound(7), command([](int) { return std::optional<BookText>(); }).execute(7));
    EXPECT_TRUE(metadata->saved.empty());
    EXPECT_TRUE(index->added.empty());
}

TEST_F(IndexBookCommandTest, indexesABatchWithOneFlushAndReportsEachBook) {
    const std::map<int, BookText> stored{{1, BookText{1, "Title: One", "whale", "1.body.txt"}},
                                         {3, BookText{3, "Title: Three", "island whale", "3.body.txt"}}};

    const std::vector<IndexResult> results = command([&](int bookId) {
        const auto found = stored.find(bookId);
        return found == stored.end() ? std::nullopt : std::optional(found->second);
    }).execute(std::vector<int>{1, 2, 3});

    EXPECT_EQ((std::vector<IndexResult>{IndexResult::success(1, 1), IndexResult::notFound(2), IndexResult::success(3, 2)}), results);
    ASSERT_EQ(2u, metadata->saved.size());
    EXPECT_EQ(1, metadata->saved[0].bookId);
    EXPECT_EQ(3, metadata->saved[1].bookId);
    EXPECT_EQ(1, index->flushes);
}

TEST_F(IndexBookCommandTest, doesNotFlushWhenNoBookOfTheBatchIsFound) {
    command([](int) { return std::optional<BookText>(); }).execute(std::vector<int>{7, 8});

    EXPECT_EQ(0, index->flushes);
}

}
}
