#include "tarantino/platform/MongoDriver.hpp"

#include <mongocxx/instance.hpp>

namespace tarantino::platform {

void startMongoDriver() {
    static const mongocxx::instance instance{};
}

}
