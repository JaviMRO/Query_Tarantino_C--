#pragma once

#include <string>
#include <unordered_set>
#include <utility>

namespace tarantino::query::ports {

/// Reads the postings of the inverted index of SPEC §8.1, in one of its structures.
class InvertedIndexReader {
public:
    /// The ids of a term, as a hash set (Java's Set<Integer>): a view of the set an index keeps in memory, as json
    /// returns its own, or a set read for this lookup.
    class Postings {
    public:
        static Postings of(const std::unordered_set<int>& kept) {
            Postings postings;
            postings.kept_ = &kept;
            return postings;
        }

        static Postings owning(std::unordered_set<int> ids) {
            Postings postings;
            postings.owned_ = std::move(ids);
            return postings;
        }

        static Postings none() {
            return Postings();
        }

        const std::unordered_set<int>& ids() const {
            return kept_ != nullptr ? *kept_ : owned_;
        }

    private:
        Postings() = default;

        std::unordered_set<int> owned_;
        const std::unordered_set<int>* kept_ = nullptr;
    };

    virtual ~InvertedIndexReader() = default;

    /// The ids of the books that contain the term; none for an unknown term.
    virtual Postings postings(const std::string& term) = 0;
};

}
