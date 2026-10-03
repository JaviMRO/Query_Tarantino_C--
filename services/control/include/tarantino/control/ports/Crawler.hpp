#pragma once

#include "tarantino/control/model/Outcome.hpp"

#include <exception>
#include <functional>

namespace tarantino::control::ports {

/// Ingests books for the control layer (SPEC §9).
class Crawler {
public:
    /// A finished download, or a book already in the datalake, waiting to be stored: the caller stores it, so that
    /// only the caller's thread uses the datalake.
    using Download = std::function<model::Outcome()>;

    /// Told, from any thread, that a download ended: with what is left to store, or with the error that stopped it,
    /// as Java's CompletableFuture.whenComplete is.
    using WhenFinished = std::function<void(Download download, std::exception_ptr error)>;

    virtual ~Crawler() = default;

    /// Starts ingesting a book: a book the datalake already holds is not downloaded again, and any other is downloaded
    /// in the background, several at once, without writing anything.
    virtual void ingest(int bookId, WhenFinished whenFinished) = 0;
};

}
