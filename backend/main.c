#include "database.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);

    const char *db_path = getenv("CUTCRAFT_DB_PATH");
    if (!db_path || !db_path[0]) db_path = "cutcraft.db";

    int port = 18088;
    const char *env_port = getenv("CUTCRAFT_C_PORT");
    if (env_port && env_port[0]) port = atoi(env_port);
    if (argc > 1) port = atoi(argv[1]);

    if (!db_init(db_path)) {
        fprintf(stderr, "Failed to initialize SQLite database at %s\n", db_path);
        return 1;
    }

    db_seed_initial_data();
    int rc = server_start(port);
    db_close();
    return rc;
}
