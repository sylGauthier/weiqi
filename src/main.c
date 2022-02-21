#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "game.h"

int main(int argc, char** argv) {
    int ok = 1, confinit = 0, wqinit = 0, uiinit = 0;
    struct Config config;
    struct Weiqi weiqi;
    struct UI ui;

    if (       config_load_defaults(&config)
            && (confinit = config_load_config(&config))
            && (uiinit = ui_start(&ui, &weiqi, &config))) {
        if (argc > 1 && !config_parse_args(&config, argc, argv)) {
            fprintf(stderr, "Error: can't parse args\n");
            ok = 0;
            goto exit;
        } else if (argc == 1) {
            ui_config_menu(&ui);
            switch (ui.status) {
                case W_UI_IDLE:
                    break;
                case W_UI_QUIT:
                    goto exit;
                default:
                    ok = 0;
                    goto exit;
            }
        }
        if (!(wqinit = weiqi_init(&weiqi,
                                  config.boardSize,
                                  config.handicap))) {
            fprintf(stderr, "Error: can't init game state\n");
            ok = 0;
            goto exit;
        }
        if (config.mode == WQ_SERVER) {
            struct GameServer srv;

            if (!game_server_init(&srv, &ui, &weiqi, &config)) {
                fprintf(stderr, "Error: failed to start game server\n");
                ok = 0;
                goto exit;
            } else if (ui_game_start(&ui) != W_NO_ERROR) {
                fprintf(stderr, "Error: failed to start interface\n");
                ok = 0;
                goto exit;
            }
            ok = game_server_run(&srv);
            game_server_free(&srv);
        } else if (config.mode == WQ_CLIENT) {
            struct GameClient client;

            if (!game_client_init(&client, &ui, &weiqi, &config)) {
                fprintf(stderr, "Error: failed to start game client\n");
                ok = 0;
                goto exit;
            } else if (ui_game_start(&ui) != W_NO_ERROR) {
                fprintf(stderr, "Error: failed to start interface\n");
                ok = 0;
                goto exit;
            }
            ok = game_client_run(&client);
            game_client_free(&client);
        } else {
            fprintf(stderr, "Error: unknown game mode\n");
            ok = 0;
        }
    } else {
        ok = 0;
    }
exit:
    if (uiinit) ui_stop(&ui);
    if (wqinit) weiqi_free(&weiqi);
    if (confinit) config_free(&config);
    return !ok;
}
