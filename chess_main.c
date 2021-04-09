
#include "platform.h"
#include <stdio.h>
#include <pthread.h>

// MARK: -  Chess engine calls

int chess_piece_at(int row, int col);
int chess_user_move(int from, int dest);
void chess_computer_move(void);
void chess_initialize(void);
void chess_callback(void);

// MARK: - Graphic assets

#define SCALE 2
#define HSPACING SCALE*20
#define VSPACING SCALE*24
#define TOPMARGIN 60
#define LEFTMARGIN 50

#define WHITE 3
#define BLACK 0
#define BLACK_OFFSET 10

const int pawn[] = {
    -1,0*SCALE,-10*SCALE,

    -1,2*SCALE,3*SCALE,
    -1,6*SCALE,0*SCALE,
    -1,2*SCALE,-2*SCALE,
    -1,3*SCALE,4*SCALE,
    -1,-3*SCALE,4*SCALE,
    -1,-2*SCALE,-2*SCALE,
    -1,-6*SCALE,0*SCALE,
    -1,-2*SCALE,3*SCALE,

    1
};

const int castle[] = {
    -1,0*SCALE,-10*SCALE,

    -1,2*SCALE,2*SCALE,
    -1,10*SCALE,0*SCALE,
    -1,1*SCALE,-2*SCALE,
    -1,3*SCALE,0*SCALE,
    -1,0*SCALE,2*SCALE,
    -1,-2*SCALE,0*SCALE,
    -1,0*SCALE,2*SCALE,
    -1,2*SCALE,0*SCALE,
    -1,0*SCALE,2*SCALE,
    -1,-2*SCALE,0*SCALE,
    -1,0*SCALE, 2*SCALE,
    -1,2*SCALE,0*SCALE,
    -1,0*SCALE,2*SCALE,
    -1,-3*SCALE,0*SCALE,
    -1,-1*SCALE,-2*SCALE,
    -1,-10*SCALE,0*SCALE,
    -1,-2*SCALE,2*SCALE,

    1
};

const int knight[] = {
    -1,0*SCALE,-10*SCALE,

    -1,2*SCALE,2*SCALE,
    -1,1*SCALE,-2*SCALE,
    -1,11*SCALE,3*SCALE,
    -1,2*SCALE,3*SCALE,
    -1,-1*SCALE,0*SCALE,
    -1,-2*SCALE,4*SCALE,
    -1,-2*SCALE,0*SCALE,
    -1,0*SCALE,-4*SCALE,
    -1,-8*SCALE,4*SCALE,
    -1,-1*SCALE,-2*SCALE,
    -1,-2*SCALE,2*SCALE,

    1
};

const int bishop[] = {
    -1,0*SCALE,-10*SCALE,

    -1,2*SCALE,3*SCALE,
    -1,12*SCALE,-2*SCALE,
    -1,4*SCALE,4*SCALE,
    -1,-4*SCALE,4*SCALE,
    -1,-4*SCALE,-4*SCALE,
    -1,2*SCALE,4*SCALE,
    -1,-10*SCALE,-2*SCALE,
    -1,-2*SCALE,3*SCALE,

    1
};

const int queen[] = {
    -1,0*SCALE,-10*SCALE,

    -1,2*SCALE,3*SCALE,
    -1,10*SCALE,0*SCALE,
    -1,0*SCALE,-2*SCALE,
    -1,10*SCALE,0*SCALE,
    -1,-4*SCALE,2*SCALE,
    -1,4*SCALE,2*SCALE,
    -1,-4*SCALE,2*SCALE,
    -1,4*SCALE,2*SCALE,
    -1,-10*SCALE,0*SCALE,
    -1,0*SCALE,-2*SCALE,
    -1,-10*SCALE,0*SCALE,
    -1,-2*SCALE,3*SCALE,

    1
};

const int king[] = {
    -1,0*SCALE,-10*SCALE,

    -1,2*SCALE,3*SCALE,
    -1,14*SCALE,1*SCALE,
    -1,1*SCALE,-4*SCALE,
    -1,1*SCALE,4*SCALE,
    -1,4*SCALE,1*SCALE,
    -1,-4*SCALE,1*SCALE,
    -1,-1*SCALE,4*SCALE,
    -1,-1*SCALE,-4*SCALE,
    -1,-14*SCALE,1*SCALE,
    -1,-2*SCALE,3*SCALE,

    1
};

// MARK: - Game types used

#define PAWN    1
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6

const int *pieces[8] = {castle, knight, bishop, queen, king, bishop, knight, castle};
const int *piece_type[6] = {pawn, knight, bishop, castle, queen, king};

typedef enum _GAME_STATE {
    GAME_INITIALIZE, GAME_START, COMPUTER_THINK, COMPUTER_MOVED, PLAYER_CHOOSE_FROM, PLAYER_CHOOSE_TO, PLAYER_MOVE, GAME_END
} GAME_STATE;

// MARK: - Forwards

void update_board(void);

// MARK: - Game vars

GAME_STATE game_state;
int game_colour;
int game_board[8][8] = {};
int game_from_x, game_from_y;
int game_to_x, game_to_y;
int draw_color = DEFAULT_COLOR;
char temp[256];

// MARK: - drawing helpers

void draw_rect(int row, int col) {
    float x = LEFTMARGIN + col * HSPACING - 10;
    float y = TOPMARGIN + row * VSPACING + 5;

    // Temporary solution
    int points[5*2];
    int points_count = 0;

    points[points_count++] = x;
    points[points_count++] = y;

    points[points_count++] = HSPACING;
    points[points_count++] = 0;

    points[points_count++] = 0;
    points[points_count++] = VSPACING;

    points[points_count++] = -HSPACING;
    points[points_count++] = 0;

    points[points_count++] = 0;
    points[points_count++] = -VSPACING;
    
    platform_draw_continous_points(&points[0], points_count, HIGHLIGHT_COLOR);
}

void draw_marker(int row, int col) {
    float x = LEFTMARGIN + col * HSPACING - 10;
    float y = TOPMARGIN + row * VSPACING + 5;

    // Temporary solution
    int points[4*2];
    int points_count = 0;

    points[points_count++] = x;
    points[points_count++] = y;
    points[points_count++] = HSPACING;
    points[points_count++] = 0;
    points[points_count++] = 0;
    points[points_count++] = -2;
    points[points_count++] = -HSPACING;
    points[points_count++] = 0;

    platform_draw_continous_points(&points[0], points_count,HIGHLIGHT_COLOR);
}

void draw_lines(const int *lines, int row, int col, int color) {
    float x = LEFTMARGIN + col * HSPACING;
    float y = TOPMARGIN + row * VSPACING;

    // Temporary solution
    int points[100];
    int points_count = 0;
    
    if (game_colour == BLACK) x += 10*SCALE;

    points[points_count++] = x;
    points[points_count++] = y;

    for (;;) {
        int f = *lines;

        if (f == 1 || f == 2) {
            platform_draw_continous_points(&points[0], points_count, color);
            return;
        }

        lines++;
        int xOff = *lines;

        lines++;
        int yOff = *lines;

        lines++;

        points[points_count++] = yOff;
        points[points_count++] = xOff;
    }
}

// MARK: - Board drawing

void draw_piece(const int *piece, int row, int col, int color) {
    draw_lines((int *)piece+game_colour, row, col, color);
}

void draw_board_piece(int row, int col) {
    int index = game_board[row][col];
    if (index == 0) {
        // Empty board cell
        return;
    }
    
    game_colour = WHITE;
    draw_color = DEFAULT_COLOR;
    
    if (index > 9) {
        index -= BLACK_OFFSET;
        game_colour = BLACK;
        draw_color = HIGHLIGHT_COLOR;
    }
    
    draw_piece(piece_type[index-1], row, col, draw_color);
}

void draw_board() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            draw_board_piece(row, col);
        }
    }
}

void update_board() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            game_board[row][col] = chess_piece_at(row, col);
            draw_board_piece(row, col);
        }
    }
}

// MARK: - Game state helpers

void choose_from_move() {
    if (platform_input_is_up()) {
        if (game_from_y > 0) {
            game_from_y--;
            platform_input_wait();
        }
    }
    if (platform_input_is_down()) {
        if (game_from_y < 7) {
            game_from_y++;
            platform_input_wait();
        }
    }
    if (platform_input_is_left()) {
        if (game_from_x > 0) {
            game_from_x--;
            platform_input_wait();
        }
    }
    if (platform_input_is_right()) {
        if (game_from_x < 7) {
            game_from_x++;
            platform_input_wait();
        }
    }

    if (platform_button_is_pressed(BUTTON_ONE)) {
        game_to_x = game_from_x;
        game_to_y = game_from_y;

        game_state = PLAYER_CHOOSE_TO;
        platform_input_wait();
    }
}

void choose_to_move() {
    if (platform_input_is_up()) {
        if (game_to_y > 0) {
            game_to_y--;
            platform_input_wait();
        }
    }
    if (platform_input_is_down()) {
        if (game_to_y < 8) {
            game_to_y++;
            platform_input_wait();
        }
    }
    if (platform_input_is_left()) {
        if (game_to_x > 0) {
            game_to_x--;
            platform_input_wait();
        }
    }
    if (platform_input_is_right()) {
        if (game_to_x < 8) {
            game_to_x++;
            platform_input_wait();
        }
    }
    
    if (platform_button_is_pressed(BUTTON_ONE)) {
        game_state = PLAYER_MOVE;
        platform_input_wait();
    }
}

void draw_from_move() {
    draw_rect(game_from_y, game_from_x);
}

void draw_choosen_from_move() {
    draw_marker(game_from_y,game_from_x);
}

void draw_to_move() {
    draw_rect(game_to_y, game_to_x);
}

void wait_for_begin() {
    if (platform_button_is_pressed(BUTTON_ONE)) {
        game_state = GAME_START;
        platform_input_wait();
    }
}

void build_from_position() {
	#define HORIZ "ABCDEFGH"
    char h = HORIZ[game_from_x];

    sprintf(temp, "FROM %c%d TO", h, 8-game_from_y);
}

void build_from_to_position() {
	if (game_from_x == 0 && game_from_y == 0) {
		sprintf(temp, "");
		return;
	}
	
	#define HORIZ "ABCDEFGH"
    char h1 = HORIZ[game_from_x];
    char h2 = HORIZ[game_to_x];

    sprintf(temp, "YOUR MOVE %c%d TO %c%d", h1, 8-game_from_y, h2, 8-game_from_y);
}

// MARK: - Chess

void* threadFunction(void* args) {
	printf("Start thinking in thread\n");
	chess_computer_move();
	game_state = COMPUTER_MOVED;
	printf("End of thinking in thread\n");
    
    return 0;
}

void computer_move() {
	pthread_t id;
    int ret;

    // creating thread
    ret = pthread_create(&id, NULL, &threadFunction,NULL);
    if (ret==0) {
    	printf("Thinking thread created successfully.\n");
    }
    else {
        printf("Thread not created.\n");
    }

    game_state = COMPUTER_THINK;
}

void user_move() {
    int from = game_from_y*8+game_from_x;
    int to = game_to_y*8+game_to_x;

    chess_user_move(from, to);
}

void init_board() {
    game_board[0][0] = ROOK;
    game_board[0][1] = KNIGHT;
    game_board[0][2] = BISHOP;
    game_board[0][3] = QUEEN;
    game_board[0][4] = KING;
    game_board[0][5] = BISHOP;
    game_board[0][6] = KNIGHT;
    game_board[0][7] = ROOK;
    
    for (int i = 0; i < 8; i++) {
        game_board[1][i] = PAWN;
        game_board[6][i] = BLACK_OFFSET+PAWN;
    }

    game_board[7][0] = BLACK_OFFSET+ROOK;
    game_board[7][1] = BLACK_OFFSET+KNIGHT;
    game_board[7][2] = BLACK_OFFSET+BISHOP;
    game_board[7][3] = BLACK_OFFSET+QUEEN;
    game_board[7][4] = BLACK_OFFSET+KING;
    game_board[7][5] = BLACK_OFFSET+BISHOP;
    game_board[7][6] = BLACK_OFFSET+KNIGHT;
    game_board[7][7] = BLACK_OFFSET+ROOK;
    
    chess_initialize();
    
    game_state = GAME_INITIALIZE;

    update_board();
}

// MARK: - User message

void print_info_top(char* msg) {
    platform_msg(msg, -100, 120, DEFAULT_TEXT_SMALL_SIZE, DEFAULT_COLOR);
}

void print_msg(char* msg) {
    platform_msg(msg, -80, 0, DEFAULT_TEXT_SIZE, DEFAULT_COLOR);
}

// MARK: - Game loop

boolean game_win() {
    return false;
}

void game_start(void) {
    init_board();
}

void game_stop(void) {
}

boolean game_frame(void) {
    platform_frame();
    
    if (platform_button_is_pressed(BUTTON_FOUR)) {
		// Press all 4 button to stop is implemented in system
        //return false;
    }

    draw_board();
    
    switch (game_state) {
        case GAME_INITIALIZE:
            wait_for_begin();
            print_msg("PRESS BUTTON TO START");
            break;
        case GAME_START:
            computer_move();
            break;
        case COMPUTER_THINK:
        	build_from_to_position();
            print_info_top(temp);

        	print_msg("THINKING...");
        	break;
        case COMPUTER_MOVED:
            game_from_x = 0; game_from_y = 0;
            game_to_x = 0; game_to_y = 0;

            update_board();
            
            if (game_win()) {
                game_state = GAME_END;
            }
            else {
                game_state = PLAYER_CHOOSE_FROM;
            }
            break;
        case PLAYER_CHOOSE_FROM:
            print_info_top("YOUR MOVE");
            
            choose_from_move();
            draw_from_move();
            break;
        case PLAYER_CHOOSE_TO:
        	build_from_position();
            print_info_top(temp);

            choose_to_move();
            draw_choosen_from_move();
            draw_to_move();
            break;
        case PLAYER_MOVE:
            user_move();
            update_board();

            if (game_win()) {
                game_state = GAME_END;
            }
            else {
                computer_move();
            }
            break;
        case GAME_END:
            print_msg("GAME ENDED");
            break;
    }
    
    return true;
}

#ifdef PITREX
int main() {
#else
int chess_main() {
#endif
    platform_init("chess");
    
    game_start();

    for (;;) {
        if (!game_frame()) {
            game_stop();
            return 0;
        }
    }
}
