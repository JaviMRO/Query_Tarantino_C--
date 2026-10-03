#include "tarantino/control/ControlFactory.hpp"
#include "tarantino/indexer/IndexerFactory.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/query/QueryFactory.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#ifdef TARANTINO_WITH_MONGO
#include "indexer/tests/adapters/mongo/TemporaryMongoDatabase.hpp"
#endif

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace tarantino::control {
namespace {

namespace files = platform::files;
using model::NextStep;
using model::StepReport;

constexpr int PARALLEL_DOWNLOADS = 3;
constexpr int INDEX_BATCH = 2;
constexpr const char* UNUSED_MONGO = "mongodb://localhost:27017";

std::string uppercaseAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; });
    return text;
}

/// A book as Project Gutenberg publishes it: header, start marker, body, end marker and footer, with CRLF.
std::string gutenbergText(const std::string& title, const std::string& author, const std::string& language,
                          const std::string& body) {
    const std::string marker = " OF THE PROJECT GUTENBERG EBOOK " + uppercaseAscii(title) + " ***";
    const std::vector<std::string> lines{"The Project Gutenberg eBook of " + title, "", "Title: " + title, "",
                                         "Author: " + author, "", "Language: " + language, "", "*** START" + marker, "",
                                         body, "", "*** END" + marker, "", "Updated editions will replace the previous one.", ""};
    std::string text;
    for (std::size_t line = 0; line < lines.size(); ++line) {
        text += (line > 0 ? "\r\n" : "") + lines[line];
    }
    return text;
}

const std::map<int, std::string> MIRROR_BOOKS{
        {11, gutenbergText("Alice's Adventures in Wonderland", "Lewis Carroll", "English",
                           "Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do.")},
        {84, gutenbergText("Frankenstein; Or, The Modern Prometheus", "Mary Wollstonecraft Shelley", "English",
                           "You will rejoice to hear that no disaster has accompanied the commencement of an enterprise which you"
                           " have regarded with such evil forebodings. I arrived here yesterday, and my first task is to"
                           " assure my dear sister of my welfare and increasing confidence in the success of my undertaking.")},
        {2000, gutenbergText("Don Quijote", "Miguel de Cervantes Saavedra", "Spanish",
                             "En un lugar de la Mancha, de cuyo nombre no quiero acordarme, no ha mucho tiempo que vivía un hidalgo"
                             " de los de lanza en astillero, adarga antigua, rocín flaco y galgo corredor.")}};

struct Structures {
    std::string layout;
    std::string index;
    std::string metadata;
};

std::ostream& operator<<(std::ostream& out, const Structures& structures) {
    return out << structures.layout << " datalake, " << structures.index << " index, " << structures.metadata << " metadata";
}

class EndToEndTest : public ::testing::TestWithParam<Structures> {
protected:
    std::filesystem::path mirror() {
        const std::filesystem::path mirror = root / "mirror";
        for (const auto& [bookId, text] : MIRROR_BOOKS) {
            const std::filesystem::path file = mirror / std::to_string(bookId) / ("pg" + std::to_string(bookId) + ".txt");
            files::createDirectories(file.parent_path());
            files::writeString(file, text);
        }
        return mirror;
    }

    std::filesystem::path workload() {
        const std::filesystem::path workload = root / "workload";
        files::createDirectories(workload);
        files::writeString(workload / "stopwords.txt", "the\nof\nand\n");
        return workload;
    }

    static std::vector<std::string> found(query::commands::SearchCommand& search, const std::string& text) {
        std::vector<std::string> books;
        for (const query::model::BookMetadata& book : search.execute(text).books) {
            books.push_back(std::to_string(book.bookId) + " " + book.title.value_or("null") + " by " + book.author.value_or("null"));
        }
        return books;
    }

    testing::TempDirectory root;
#ifdef TARANTINO_WITH_MONGO
    indexer::adapters::TemporaryMongoDatabase mongo;
#endif
};

TEST_P(EndToEndTest, downloadsIndexesAndFindsEveryBookOfAMirror) {
    const Structures& structures = GetParam();
    std::string mongoUri = UNUSED_MONGO;
    if (structures.index == "mongo" || structures.metadata == "mongo") {
#ifdef TARANTINO_WITH_MONGO
        const auto uri = mongo.uri();
        if (!uri) {
            GTEST_SKIP() << indexer::adapters::TemporaryMongoDatabase::unavailable();
        }
        mongoUri = *uri;
#else
        GTEST_SKIP() << "This build has no MongoDB support";
#endif
    }
    const std::filesystem::path workloadDirectory = workload();
    const ControlConfig control{root / "control", workloadDirectory, PARALLEL_DOWNLOADS, INDEX_BATCH};
    const crawler::CrawlerConfig crawler{root / "datalake", structures.layout, mirror()};
    const indexer::IndexerConfig indexer{root / "datalake", structures.layout, root / "datamarts", structures.index,
                                         structures.metadata, mongoUri, workloadDirectory};
    std::ostringstream removed;
    const auto pipeline = ControlFactory::pipeline(control, crawler, indexer, ControlFactory::candidates(control, crawler, std::nullopt), removed);

    std::vector<StepReport> reports;
    for (StepReport report = pipeline->runStep(); !report.idle(); report = pipeline->runStep()) {
        reports.push_back(report);
    }

    std::vector<int> downloaded;
    std::vector<std::size_t> batchSizes;
    for (const StepReport& report : reports) {
        EXPECT_TRUE(report.outcome.succeeded) << report.description();
        if (report.step.action == NextStep::Action::Download) {
            downloaded.insert(downloaded.end(), report.step.bookIds.begin(), report.step.bookIds.end());
        } else {
            batchSizes.push_back(report.step.bookIds.size());
        }
    }
    std::sort(downloaded.begin(), downloaded.end());
    EXPECT_EQ((std::vector<int>{11, 84, 2000}), downloaded);
    EXPECT_EQ((std::vector<std::size_t>{2, 1}), batchSizes);
    const std::vector<std::string> indexedLines = files::readLines(root / "control/indexed_books.txt");
    EXPECT_EQ((std::set<std::string>{"11", "84", "2000"}), std::set<std::string>(indexedLines.begin(), indexedLines.end()));
    query::commands::SearchCommand search = query::QueryFactory::searchCommand(
            query::QueryConfig{root / "datamarts", structures.index, structures.metadata, mongoUri, workloadDirectory});
    EXPECT_EQ((std::vector<std::string>{"11 Alice's Adventures in Wonderland by Lewis Carroll",
                                        "84 Frankenstein; Or, The Modern Prometheus by Mary Wollstonecraft Shelley"}),
              found(search, "sister"));
    EXPECT_EQ(std::vector<std::string>{"11 Alice's Adventures in Wonderland by Lewis Carroll"}, found(search, "tired sister"));
    EXPECT_EQ(std::vector<std::string>{"2000 Don Quijote by Miguel de Cervantes Saavedra"}, found(search, "rocín"));
    EXPECT_TRUE(found(search, "whale").empty());
}

INSTANTIATE_TEST_SUITE_P(Structures, EndToEndTest,
                         ::testing::Values(Structures{"time", "json", "sqlite"}, Structures{"book", "folders", "sqlite"},
                                           Structures{"batch", "mongo", "mongo"}),
                         [](const ::testing::TestParamInfo<Structures>& info) {
                             return info.param.layout + "_datalake_" + info.param.index + "_index_" + info.param.metadata + "_metadata";
                         });

}
}
