#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <jansson.h>

#include "config.h"
#include "cmd.h"

#define SET_VEC3(v, a, b, c) v[0] = (a); v[1] = (b); v[2] = (c);

static void load_default_theme(struct InterfaceTheme* theme) {
    memset(theme, 0, sizeof(*theme));
    strcpy(theme->name, "default");
    theme->style = W_UI_NICE;
    strcpy(theme->board.texture, "wood.png");

    SET_VEC3(theme->backgroundColor, 0.3, 0.3, 0.3);
    SET_VEC3(theme->ambientColor, 40, 40, 40);
    SET_VEC3(theme->sunDirection, -0.5, 0.1, -0.8);
    SET_VEC3(theme->sunColor, 0.5, 0.5, 0.5);
    normalize3(theme->sunDirection);
    theme->shadow = 1;
    theme->occlusion = 1;

    SET_VEC3(theme->bStone.color, 0, 0, 0);
    SET_VEC3(theme->wStone.color, 1, 1, 1);
    SET_VEC3(theme->pointer.color, 0, 0, 0);
    SET_VEC3(theme->board.color, 0.65, 0.58, 0.29);
    SET_VEC3(theme->lmvp.color, 0.6, 0.2, 0.2);

    theme->board.roughness = 0.3;
    theme->board.metalness = 0.;
    theme->wStone.roughness = 0.3;
    theme->wStone.metalness = 0;
    theme->bStone.roughness = 0.3;
    theme->bStone.metalness = 0;

    theme->boardThickness = 0.01;
    theme->gridScale = 0.;
    theme->stoneThickness = 0.3;
    theme->pointerSize = 0.01;
    theme->fov = 60;

    theme->coordinates = 0;
    strcpy(theme->font, "font.ttf");
}

int config_load_defaults(struct Config* config) {
    memset(config, 0, sizeof(*config));
    config->boardSize = 19;
    config->handicap = 0;
    config->black.type = W_HUMAN;
    config->white.type = W_HUMAN;
    config->mode = WQ_SERVER;
    config->numEngines = 0;
    load_default_theme(&config->themes[0]);
    config->numThemes = 1;
    config->curTheme = 0;
    return 1;
}

static int add_engine(struct Config* config,
                      const char* name,
                      const char* cmd) {
    struct Engine* e;

    if (config->numEngines >= W_NUM_ENGINES) return 0;
    e = config->engines + config->numEngines;
    if (       !(e->name = malloc(strlen(name) + 1))
            || !(e->command = malloc(strlen(cmd) + 1))) {
        free(e->name);
        return 0;
    }
    strcpy(e->name, name);
    strcpy(e->command, cmd);
    config->numEngines++;
    return 1;
}

char* config_find_engine(struct Config* config, const char* name) {
    unsigned int i;
    for (i = 0; i < config->numEngines; i++) {
        if (!strcmp(name, config->engines[i].name)) {
            return config->engines[i].command;
        }
    }
    return NULL;
}

void config_randomize_players(struct Config* config) {
    srand(time(NULL));
    if (rand() % 2) {
        struct PlayerConf tmp = config->black;
        config->black = config->white;
        config->white = tmp;
    }
}

static int config_load_player(struct Config* config,
                              struct PlayerConf* c,
                              const char* p) {
    if (!strcmp(p, "human")) {
        c->type = W_HUMAN;
    } else if (!strncmp(p, "socket:", strlen("socket:"))) {
        c->type = W_GTP_SOCKET;
        c->gtpCmd = p + 7;
    } else {
        c->type = W_GTP_LOCAL;
        if (!(c->gtpCmd = config_find_engine(config, p))) {
            fprintf(stderr, "Error: invalid GTP engine: %s\n", p);
            return 0;
        }
    }
    return 1;
}

static int parse_engine(struct Config* config, json_t* engine) {
    json_t *jname, *jcmd;

    if (       !(jname = json_object_get(engine, "name"))
            || !json_is_string(jname)
            || !(jcmd = json_object_get(engine, "command"))
            || !json_is_string(jcmd)) {
        fprintf(stderr, "Error: config: "
                "engine must have 'name' and 'command' string attributes\n");
        return 0;
    }
    return add_engine(config,
                      json_string_value(jname),
                      json_string_value(jcmd));
}

static int parse_asset_theme(struct AssetParams* params, json_t* jparam) {
    json_t* cur;

    if ((cur = json_object_get(jparam, "texture"))
            && json_is_string(cur)) {
        strncpy(params->texture,
                json_string_value(cur),
                sizeof(params->texture) - 1);
    }
    if ((cur = json_object_get(jparam, "color"))
            && json_is_array(cur)
            && json_array_size(cur) == 3) {
        params->color[0] = json_number_value(json_array_get(cur, 0));
        params->color[1] = json_number_value(json_array_get(cur, 1));
        params->color[2] = json_number_value(json_array_get(cur, 2));
    }
    if ((cur = json_object_get(jparam, "roughness"))
            && json_is_number(cur)) {
        params->roughness = json_number_value(cur);
    }
    if ((cur = json_object_get(jparam, "metalness"))
            && json_is_number(cur)) {
        params->metalness = json_number_value(cur);
    }
    return 1;
}

static int parse_lighting(struct InterfaceTheme* theme, json_t* jlight) {
    json_t* cur;

    if ((cur = json_object_get(jlight, "ibl"))
            && json_is_string(cur)) {
        strncpy(theme->iblPath,
                json_string_value(cur),
                sizeof(theme->iblPath) - 1);
    }
    if ((cur = json_object_get(jlight, "sun_color"))) {
        if (       !json_is_array(cur)
                || json_array_size(cur) != 3) {
            fprintf(stderr, "Error: config: "
                            "'sun_color' must be an array of 3 numbers\n");
            return 0;
        }
        theme->sunColor[0] = json_number_value(json_array_get(cur, 0));
        theme->sunColor[1] = json_number_value(json_array_get(cur, 1));
        theme->sunColor[2] = json_number_value(json_array_get(cur, 2));
    }
    if ((cur = json_object_get(jlight, "sun_direction"))) {
        if (       !json_is_array(cur)
                || json_array_size(cur) != 3) {
            fprintf(stderr, "Error: config: "
                            "'sun_direction' must be an array of 3 numbers\n");
            return 0;
        }
        theme->sunDirection[0] = json_number_value(json_array_get(cur, 0));
        theme->sunDirection[1] = json_number_value(json_array_get(cur, 1));
        theme->sunDirection[2] = json_number_value(json_array_get(cur, 2));
    }
    if ((cur = json_object_get(jlight, "ambient_color"))) {
        if (       !json_is_array(cur)
                || json_array_size(cur) != 3) {
            fprintf(stderr, "Error: config: "
                            "'ambient_color' must be an array of 3 numbers\n");
            return 0;
        }
        theme->ambientColor[0] = json_number_value(json_array_get(cur, 0));
        theme->ambientColor[1] = json_number_value(json_array_get(cur, 1));
        theme->ambientColor[2] = json_number_value(json_array_get(cur, 2));
    }
    if ((cur = json_object_get(jlight, "shadows"))) {
        if (!json_is_boolean(cur)) {
            fprintf(stderr, "Error: config: 'shadow' must be a boolean\n");
            return 0;
        }
        theme->shadow = json_boolean_value(cur);
    }
    if ((cur = json_object_get(jlight, "occlusion"))) {
        if (!json_is_boolean(cur)) {
            fprintf(stderr, "Error: config: 'occlusion' must be a boolean\n");
            return 0;
        }
        theme->occlusion = json_boolean_value(cur);
    }
    return 1;
}

static int parse_theme(struct Config* config, json_t* jtheme) {
    json_t* cur;
    struct InterfaceTheme* theme;

    if (config->numThemes >= W_NUM_THEMES) {
        fprintf(stderr, "Warning: max number of themes reached (%d)\n",
                        W_NUM_THEMES);
        return 1;
    }
    theme = &config->themes[config->numThemes];
    load_default_theme(theme);
    if (!(cur = json_object_get(jtheme, "name"))) {
        fprintf(stderr, "Error: config: missing name in theme\n");
        return 0;
    } else if (!json_is_string(cur)) {
        fprintf(stderr, "Error: config: theme name must be string\n");
        return 0;
    } else {
        strncpy(theme->name, json_string_value(cur), sizeof(theme->name) - 1);
        theme->name[sizeof(theme->name) - 1] = '\0';
    }
    if ((cur = json_object_get(jtheme, "style"))) {
        const char* style;

        if (!(style = json_string_value(cur))) {
            fprintf(stderr, "Error: config: 'style' must be a string\n");
            return 0;
        } else if (!strcmp(style, "pure")) {
            theme->style = W_UI_PURE;
        } else if (!strcmp(style, "nice")) {
            theme->style = W_UI_NICE;

        } else {
            fprintf(stderr, "Error: config: "
                    "theme must be one of 'nice', 'pure'\n");
            return 0;
        }
    }
    if ((cur = json_object_get(jtheme, "fov"))) {
        if (!json_is_number(cur)) {
            fprintf(stderr, "Error: config: 'fov' must be a number\n");
            return 0;
        }
        theme->fov = json_number_value(cur);
    }
    if ((cur = json_object_get(jtheme, "black_stone"))) {
        if (!parse_asset_theme(&theme->bStone, cur)) return 0;
    }
    if ((cur = json_object_get(jtheme, "white_stone"))) {
        if (!parse_asset_theme(&theme->wStone, cur)) return 0;
    }
    if ((cur = json_object_get(jtheme, "board"))) {
        if (!parse_asset_theme(&theme->board, cur)) return 0;
    }
    if ((cur = json_object_get(jtheme, "lighting"))) {
        if (!parse_lighting(theme, cur)) return 0;
    }
    config->numThemes++;
    return 1;
}

static int parse_players(struct Config* config, json_t* jplayers) {
    json_t *cur;

    if ((cur = json_object_get(jplayers, "random"))) {
        if (!json_is_array(cur) || json_array_size(cur) != 2
                || !json_is_string(json_array_get(cur, 0))
                || !json_is_string(json_array_get(cur, 1))) {
            fprintf(stderr, "Error: config: "
                            "'random' must be an array of 2 players\n");
            return 0;
        }
        config->randomPlayer = 1;
        return config_load_player(config,
                                  &config->black,
                                  json_string_value(json_array_get(cur, 0)))
            && config_load_player(config,
                                  &config->white,
                                  json_string_value(json_array_get(cur, 1)));
    }
    if ((cur = json_object_get(jplayers, "white"))) {
        if (!json_is_string(cur)) {
            fprintf(stderr, "Error: config: player 'white' must be string\n");
            return 0;
        }
        if (!config_load_player(config,
                                &config->white,
                                json_string_value(cur))) {
            fprintf(stderr, "Error: config: config_load_player failed\n");
            return 0;
        }
    }
    if ((cur = json_object_get(jplayers, "black"))) {
        if (!json_is_string(cur)) {
            fprintf(stderr, "Error: config: player 'black' must be string\n");
            return 0;
        }
        if (!config_load_player(config,
                                &config->black,
                                json_string_value(cur))) {
            fprintf(stderr, "Error: config: config_load_player failed\n");
            return 0;
        }
    }
    return 1;
}

static int find_theme(struct Config* config, const char* name) {
    unsigned int i = 0;

    for (i = 0; i < config->numThemes; i++) {
        if (!strcmp(config->themes[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static int parse_config(struct Config* config, json_t* root) {
    json_t* cur;

    if ((cur = json_object_get(root, "engines"))) {
        if (!json_is_array(cur)) {
            fprintf(stderr, "Warning: config: 'engines' must be an array\n");
        } else {
            unsigned int i;
            json_t* engine;

            json_array_foreach(cur, i, engine) {
                if (!parse_engine(config, engine)) {
                    return 0;
                }
            }
        }
    }
    if ((cur = json_object_get(root, "themes"))) {
        json_t* theme;
        unsigned int i;

        if (!json_is_array(cur)) {
            fprintf(stderr, "Error: config: 'themes' should be an array\n");
            return 0;
        }
        json_array_foreach(cur, i, theme) {
            if (!parse_theme(config, theme)) {
                return 0;
            }
        }
    }
    if ((cur = json_object_get(root, "default_theme"))) {
        int t;
        if (!json_is_string(cur)) {
            fprintf(stderr, "Error: config: 'default_theme' must be string\n");
            return 0;
        }
        if ((t = find_theme(config, json_string_value(cur))) < 0) {
            fprintf(stderr, "Error: config: invalid theme: %s\n",
                            json_string_value(cur));
            return 0;
        }
        config->curTheme = t;
    }
    if ((cur = json_object_get(root, "players"))) {
        if (!parse_players(config, cur)) {
            return 0;
        }
    }
    return 1;
}

static int get_conf_path(char* path, unsigned int len) {
    const char *confdir, *home;
    char defconfdir[512];
    char name[] = "weiqi", conf[] = "config.json";

    if (!(confdir = getenv("XDG_CONFIG_HOME"))) {
        if (!(home = getenv("HOME"))) {
            confdir = "./";
        } else {
            if (strlen(home) + strlen("/.config") + 1 > sizeof(defconfdir)) {
                fprintf(stderr, "Error: HOME path too long\n");
                return 0;
            }
            strcpy(defconfdir, home);
            strcpy(defconfdir + strlen(home), "/.config");
            confdir = defconfdir;
        }
    }
    if (strlen(confdir) + strlen(name) + strlen(conf) + 3 > len) {
        fprintf(stderr, "Error: conf path too long\n");
        return 0;
    }
    sprintf(path, "%s/%s/%s", confdir, name, conf);
    return 1;
}

int config_load_config(struct Config* config) {
    FILE* f;
    json_error_t error;
    int ok = 0;

    if (!get_conf_path(config->confpath, W_CONF_PATH_SIZE)) return 0;
    if (!(f = fopen(config->confpath, "r"))) {
        fprintf(stderr, "Warning: "
                        "can't open config file: %s, assuming defaults\n",
                        config->confpath);
        return 1;
    }
    if (!(config->file = json_loadf(f, 0, &error))) {
        fprintf(stderr, "Error: couldn't load config file: %s\n", error.text);
    } else {
        ok = parse_config(config, config->file);
    }
    if (config->file && !ok) json_decref(config->file);
    return ok;
}

static void print_help(const char* cmd) {
    printf("%s [OPTIONS]\n"
           "    -s --size <size>: board size (5-25, default 19)\n"
           "    -h --handicap <num>: number of handicaps (default 0)\n"
           "    -b --black <human|gnugo>: black player (default human)\n"
           "    -w --white <human|gnugo>: white player (default gnugo)\n"
           "    -l --load <file>: load a saved game\n"
           "    -i --interface <pure|nice>: interface theme (default nice)\n",
           cmd);
}

int config_parse_args(struct Config* config, unsigned int argc, char** argv) {
    unsigned int i;

    for (i = 1; i < argc; i++) {
        char* arg = argv[i];

        if (!strcmp(arg, "-w") || !strcmp(arg, "--white")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!config_load_player(config, &config->white, argv[i + 1])) {
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-b") || !strcmp(arg, "--black")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!config_load_player(config, &config->black, argv[i + 1])) {
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-r") || !strcmp(arg, "--random")) {
            if (i >= argc - 2) {
                print_help(argv[0]);
                return 0;
            }
            config->randomPlayer = 1;
            if (   !config_load_player(config, &config->black, argv[i + 1])
                || !config_load_player(config, &config->white, argv[i + 2])) {
                return 0;
            }
            i += 2;
        } else if (!strcmp(arg, "-s") || !strcmp(arg, "--size")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            config->boardSize = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "-h") || !strcmp(arg, "--handicap")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            config->handicap = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "-i") || !strcmp(arg, "--interface")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!strcmp(argv[i + 1], "pure")) {
                config->themes[0].style = W_UI_PURE;
            } else if (!strcmp(argv[i + 1], "nice")) {
                config->themes[0].style = W_UI_NICE;
            } else {
                print_help(argv[0]);
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-f") || !strcmp(arg, "--fov")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            config->themes[0].fov = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "--texture")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            strncpy(config->themes[0].board.texture, argv[i + 1],
                    sizeof(config->themes[0].board.texture) - 1);
            i++;
        } else if (!strcmp(arg, "--coordinates")) {
            config->themes[0].coordinates = 1;
        } else if (!strcmp(arg, "--color")) {
            if (i + 3 >= argc) {
                print_help(argv[0]);
                return 0;
            }
            SET_VEC3(config->themes[0].board.color,
                     strtof(argv[i + 1], NULL),
                     strtof(argv[i + 2], NULL),
                     strtof(argv[i + 3], NULL));
            SET_VEC3(config->themes[0].board.color,
                     strtof(argv[i + 1], NULL),
                     strtof(argv[i + 2], NULL),
                     strtof(argv[i + 3], NULL));
            i += 3;
        } else if (!strcmp(arg, "--client")) {
            if (i >= argc - 2) {
                print_help(argv[0]);
                return 0;
            }
            config->mode = WQ_CLIENT;
            config->sockpath = argv[i + 2];
            if (!config_load_player(config, &config->white, argv[i + 1])) {
                return 0;
            }
            i += 2;
        } else {
            print_help(argv[0]);
            return 0;
        }
    }
    if (config->boardSize < 5 || config->boardSize > 25) {
        fprintf(stderr, "Error: invalid board size, we only support 5 to 25\n");
        return 0;
    }
    return 1;
}

void config_free(struct Config* config) {
    unsigned int i;

    json_decref(config->file);
    for (i = 0; i < config->numEngines; i++) {
        free(config->engines[i].name);
        free(config->engines[i].command);
    }
}
