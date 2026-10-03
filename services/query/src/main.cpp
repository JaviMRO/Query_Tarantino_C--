#include "tarantino/platform/Environment.hpp"
#include "tarantino/query/QueryConfig.hpp"
#include "tarantino/query/QueryFactory.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

using tarantino::query::model::BookMetadata;
using tarantino::query::model::SearchResult;

/// The words joined with single spaces, as String.join(" ", args).
std::string join(const std::vector<std::string>& words) {
    std::string text;
    for (std::size_t index = 0; index < words.size(); ++index) {
        if (index > 0) {
            text += ' ';
        }
        text += words[index];
    }
    return text;
}

std::string field(const std::optional<std::string>& value) {
    return value.value_or("null");
}

/// Two spaces and [<id>] <title> — <author> (<language>) <path>, a missing field written null (SPEC §15).
std::string line(const BookMetadata& book) {
    return "  [" + std::to_string(book.bookId) + "] " + field(book.title) + " \xE2\x80\x94 " + field(book.author) + " ("
           + field(book.language) + ") " + book.path;
}

void print(const SearchResult& result) {
    std::cout << result.books.size() << " result(s) for \"" << result.query << "\"\n";
    for (const BookMetadata& book : result.books) {
        std::cout << line(book) << '\n';
    }
    std::cout.flush();
}

}

int main(int argc, char** argv) {
    using namespace tarantino;
    platform::useUtf8Console();
    try {
        const std::string query = join(platform::commandLineArguments(argc, argv));
        query::commands::SearchCommand search = query::QueryFactory::searchCommand(query::QueryConfig::fromEnvironment());
        print(search.execute(query));
        return 0;
    } catch (const std::exception& error) {
        std::cout.flush();
        std::cerr << "query: " << error.what() << std::endl;
        return 1;
    }
}
