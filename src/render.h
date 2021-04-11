static void render_stone(struct Interface* ui, enum WeiqiColor color,
                         unsigned char row, unsigned char col) {
    Mat4 model;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    float yScale = ui->theme->stoneYScale;
    struct Asset3D* stone;

    load_id4(model);
    load_id3(invNormal);
    stone = color == W_WHITE ? &ui->wStone : &ui->bStone;

    model[3][0] = ui->theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme->boardThickness / 2.
                    + yScale * ui->theme->stoneRadius;
    model[3][2] =  -ui->theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    model[1][1] = yScale;

    material_use(stone->mat);
    material_set_matrices(stone->mat, model, invNormal);
    vertex_array_render(stone->va);
}

static void render_pointer(struct Interface* ui) {
    Mat4 model;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    unsigned char col = ui->cursorPos[0], row = ui->cursorPos[1];

    load_id4(model);
    load_id3(invNormal);

    model[3][0] = ui->theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme->boardThickness / 2.;
    model[3][2] = -ui->theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    model[2][2] = 1.;

    material_use(ui->pointer.mat);
    material_set_matrices(ui->pointer.mat, model, invNormal);
    vertex_array_render(ui->pointer.va);
}

static void render_lmvpointer(struct Interface* ui) {
    Mat4 model;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    unsigned char col, row;

    if (!ui->weiqi->history.last || ui->weiqi->history.last->action != W_PLAY) {
        return;
    }
    col = ui->weiqi->history.last->col;
    row = ui->weiqi->history.last->row;

    load_id4(model);
    load_id3(invNormal);

    model[3][0] = ui->theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme->boardThickness / 2.
                  + ui->theme->stoneYScale * ui->theme->stoneRadius;
    model[3][2] = -ui->theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    model[2][2] = 1.;

    material_use(ui->lmvpointer.mat);
    material_set_matrices(ui->lmvpointer.mat, model, invNormal);
    vertex_array_render(ui->lmvpointer.va);
}

static void render_board(struct Interface* ui) {
    Mat4 model;
    Mat3 invNormal, tmp;
    unsigned char row, col, s;
    Vec3 axis = {1, 0, 0};

    load_rot4(model, axis, -M_PI / 2);
    mat4to3(tmp, MAT_CONST_CAST(model));
    invert3m(invNormal, MAT_CONST_CAST(tmp));
    transpose3m(invNormal);

    material_use(ui->board.mat);
    material_set_matrices(ui->board.mat, model, invNormal);
    vertex_array_render(ui->board.va);

    s = ui->weiqi->boardSize;
    for (row = 0; row < s; row++) {
        for (col = 0; col < s; col++) {
            if (ui->weiqi->board[row * s + col]) {
                render_stone(ui, ui->weiqi->board[row * s + col], row, col);
            }
        }
    }
    render_lmvpointer(ui);
    render_pointer(ui);
}
