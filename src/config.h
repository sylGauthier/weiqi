#ifndef PROG_H
#define PROG_H

#include "theme.h"

#define W_CONF_PATH_SIZE    512
#define W_NUM_ENGINES       32

enum PlayerType {
    W_HUMAN,
    W_GTP_LOCAL,
    W_GTP_SOCKET
};

struct Engine {
    char* name;
    char* command;
};

struct PlayerConf {
    enum PlayerType type;
    const char* gtpCmd;
};

enum GameMode {
    WQ_SERVER,
    WQ_CLIENT
};

struct Config {
    unsigned char boardSize;
    unsigned char handicap;
    struct PlayerConf white;
    struct PlayerConf black;

    char confpath[W_CONF_PATH_SIZE];
    const char* gtpCmd;
    const char* gameFile;
    const char* sockpath;

    enum GameMode mode;

    struct Engine engines[W_NUM_ENGINES];
    unsigned int numEngines;

    struct InterfaceTheme theme;

    void* file;
};

char* config_find_engine(struct Config* conf, const char* name);

int config_load_defaults(struct Config* conf);
int config_load_config(struct Config* conf);
int config_parse_args(struct Config* conf, unsigned int argc, char** argv);
void config_free(struct Config* config);

#endif
