#include "adapters/index/json/JsonIndexFile.hpp"

#include "tarantino/platform/Files.hpp"
#include "tarantino/platform/Paths.hpp"

#include <nlohmann/json.hpp>

#include <limits>
#include <stdexcept>

namespace tarantino::query::adapters {

namespace {

using Index = platform::StringMap<std::unordered_set<int>>;

/// Reads {"term": [ids], ...} into the index as the parser goes, rejecting anything else.
class IndexReader : public nlohmann::json_sax<nlohmann::json> {
public:
    IndexReader(Index& index, const std::filesystem::path& file) : index_(index), file_(file) {
    }

    bool null() override {
        return unexpected("null");
    }

    bool boolean(bool) override {
        return unexpected("boolean");
    }

    bool number_integer(number_integer_t value) override {
        return value < 0 ? unexpected("negative number") : bookId(static_cast<std::uint64_t>(value));
    }

    bool number_unsigned(number_unsigned_t value) override {
        return bookId(value);
    }

    bool number_float(number_float_t, const string_t&) override {
        return unexpected("decimal number");
    }

    bool string(string_t&) override {
        return unexpected("string");
    }

    bool binary(binary_t&) override {
        return unexpected("binary");
    }

    bool start_object(std::size_t) override {
        if (depth_ != 0) {
            return unexpected("object");
        }
        depth_ = 1;
        return true;
    }

    bool key(string_t& term) override {
        postings_ = &index_[std::move(term)];
        return true;
    }

    bool end_object() override {
        depth_ = 0;
        return true;
    }

    bool start_array(std::size_t elements) override {
        if (depth_ != 1 || postings_ == nullptr) {
            return unexpected("array");
        }
        if (elements != static_cast<std::size_t>(-1)) {
            postings_->reserve(elements);
        }
        depth_ = 2;
        return true;
    }

    bool end_array() override {
        depth_ = 1;
        postings_ = nullptr;
        return true;
    }

    bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception& error) override {
        throw std::runtime_error("Invalid JSON in " + platform::portable(file_) + ": " + error.what());
    }

private:
    bool bookId(std::uint64_t value) {
        if (depth_ != 2 || value > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
            return unexpected("number");
        }
        postings_->insert(static_cast<int>(value));
        return true;
    }

    [[noreturn]] bool unexpected(const char* what) const {
        throw std::runtime_error(std::string("Not an inverted index in ") + platform::portable(file_) + ": unexpected " + what);
    }

    Index& index_;
    const std::filesystem::path& file_;
    std::unordered_set<int>* postings_ = nullptr;
    int depth_ = 0;
};

}

platform::StringMap<std::unordered_set<int>> JsonIndexFile::read(const std::filesystem::path& file) {
    Index index;
    const std::string text = platform::files::readString(file);
    IndexReader reader(index, file);
    nlohmann::json::sax_parse(text, &reader);
    return index;
}

}
