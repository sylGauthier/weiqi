#include "player.h"

int player_init(struct Player* player,
                struct PlayerConf* config,
                struct Interface* ui) {
    switch (config->type) {
        case W_HUMAN:
            if (!player_human_init(player, ui)) {
                fprintf(stderr, "Error: human player init failed\n");
                return 0;
            }
            break;
        case W_GTP_LOCAL:
            if (!player_gtp_pipe_init(player, config->gtpCmd)) {
                fprintf(stderr, "Error: GTP engine init failed\n");
                return 0;
            }
            break;
        case W_GTP_SOCKET:
            if (!player_gtp_socket_init(player, config->gtpCmd)) {
                fprintf(stderr, "Error: GTP socket init failed\n");
                return 0;
            }
            break;
        default:
            fprintf(stderr, "Error: unsupported player backend\n");
            return 0;
    }
    return 1;
}

