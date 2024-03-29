#ifndef PROG_H
#define PROG_H

#include "theme.h"

#define W_CONF_PATH_SIZE    512
#define W_NUM_ENGINES       32
#define W_NUM_THEMES        32

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
    int boardSize;
    int handicap;
    struct PlayerConf white;
    struct PlayerConf black;
    int randomPlayer;

    char confpath[W_CONF_PATH_SIZE];
    const char* gtpCmd;
    const char* gameFile;
    const char* sockpath;

    enum GameMode mode;

    struct Engine engines[W_NUM_ENGINES];
    unsigned int numEngines;

    struct InterfaceTheme themes[W_NUM_THEMES];
    unsigned int numThemes, curTheme;

    void* file;
};

char* config_find_engine(struct Config* conf, const char* name);
void config_randomize_players(struct Config* conf);

int config_load_defaults(struct Config* conf);
int config_load_config(struct Config* conf);
int config_parse_args(struct Config* conf, unsigned int argc, char** argv);
void config_free(struct Config* config);

#endif
