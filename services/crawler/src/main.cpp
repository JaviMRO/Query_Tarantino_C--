#include "tarantino/crawler/CrawlerConfig.hpp"
#include "tarantino/crawler/CrawlerFactory.hpp"
#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Text.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using tarantino::crawler::commands::IngestResult;

/// The book ids of the arguments, all read before any work (SPEC §15).
std::vector<int> bookIds(const std::vector<std::string>& arguments) {
    std::vector<int> ids;
    for (const std::string& argument : arguments) {
        const auto bookId = tarantino::platform::parseDigits(argument);
        if (!bookId || *bookId <= 0) {
            throw std::invalid_argument("Not a book id: " + argument + " (expected a positive integer)");
        }
        ids.push_back(*bookId);
    }
    return ids;
}

/// One line per book, in the words of the control service (SPEC §15).
std::string line(const IngestResult& result) {
    return "[CRAWLER] " + std::to_string(result.bookId) + ": "
           + (result.succeeded() ? "stored in " + tarantino::platform::portable(result.paths->body.parent_path())
                                 : "skipped, " + std::string(name(*result.failure)));
}

}

int main(int argc, char** argv) {
    using namespace tarantino;
    platform::useUtf8Console();
    try {
        const std::vector<int> ids = bookIds(platform::commandLineArguments(argc, argv));
        const crawler::CrawlerConfig config = crawler::CrawlerConfig::fromEnvironment();
        crawler::CrawlerFactory::removeIncompleteWrites(config, std::cout);
        crawler::commands::IngestBookCommand ingest = crawler::CrawlerFactory::ingestCommand(config);
        for (const int bookId : ids) {
            std::cout << line(ingest.execute(bookId)) << std::endl;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cout.flush();
        std::cerr << "crawler: " << error.what() << std::endl;
        return 1;
    }
}
