#define _USE_MATH_DEFINES
#include <iostream>
#include <memory>
#include "rest_server.h"
#include "lyradb/database.h"

using namespace lyradb;
using namespace lyradb::server;

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        std::string db_path = "lyradb.db";
        std::string host = "127.0.0.1";
        int port = 8080;

        if (argc > 1) db_path = argv[1];
        if (argc > 2) host = argv[2];
        if (argc > 3) port = std::stoi(argv[3]);

        std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
        std::cout << "  LyraDB - REST API Server" << std::endl;
        std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
        std::cout << std::endl;

        // Create database
        std::cout << "📂 Opening database: " << db_path << std::endl;
        auto db = std::make_shared<Database>(db_path);
        std::cout << "✅ Database loaded" << std::endl;

        // Create and start REST server
        std::cout << "🔧 Initializing REST API server..." << std::endl;
        RestServer server(host, port);
        server.attach_database(db);

        std::cout << std::endl;
        server.start();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
