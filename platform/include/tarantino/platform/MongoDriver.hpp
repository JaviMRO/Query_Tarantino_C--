#pragma once

namespace tarantino::platform {

/// Starts the MongoDB C++ driver the first time it is called, so that a process has one mongocxx::instance (SPEC §14)
/// whichever services it links: the control service holds the indexer and, in its tests, the query service too.
void startMongoDriver();

}
