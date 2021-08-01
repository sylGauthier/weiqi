#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "game.h"

int main(int argc, char** argv) {
    int ok = 1, confinit = 0, wqinit = 0, uiinit = 0;
    struct Config config;
    struct Weiqi weiqi;
    struct Interface ui;

    if (       config_load_defaults(&config)
            && (confinit = config_load_config(&config))
            && config_parse_args(&config, argc, argv)
            && (wqinit = weiqi_init(&weiqi,
                                    config.boardSize,
                                    config.handicap))) {
        if (config.mode == WQ_SERVER) {
            struct GameServer srv;

            if (!game_server_init(&srv, &ui, &weiqi, &config)) {
                fprintf(stderr, "Error: failed to start game server\n");
                ok = 0;
                goto exit;
            } else if (!(uiinit = interface_init(&ui, &weiqi, &config.theme))) {
                fprintf(stderr, "Error: failed to start interface\n");
                ok = 0;
                goto exit;
            }
            ok = game_server_run(&srv);
            game_server_free(&srv);
        } else if (config.mode == WQ_CLIENT) {
            struct GameClient client;

            /* client_init will query critical informations such as board size
             * from the server so it must be called *before* interface_init
             */
            if (!game_client_init(&client, &ui, &weiqi, &config)) {
                fprintf(stderr, "Error: failed to start game client\n");
                ok = 0;
                goto exit;
            } else if (!(uiinit = interface_init(&ui, &weiqi, &config.theme))) {
                fprintf(stderr, "Error: failed to start interface\n");
                ok = 0;
                goto exit;
            }
            ok = game_client_run(&client);
            game_client_free(&client);
        } else {
            fprintf(stderr, "Error: unkown game mode\n");
            ok = 0;
        }
    } else {
        ok = 0;
    }
exit:
    if (uiinit) interface_free(&ui);
    if (wqinit) weiqi_free(&weiqi);
    if (confinit) config_free(&config);
    return !ok;
}
