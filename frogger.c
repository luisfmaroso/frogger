#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <conio.h>

#define SCREEN_WIDTH 61
#define SCREEN_HEIGHT 26
#define CONSOLE_WIDTH 62
#define CONSOLE_HEIGHT 27
#define TARGET_FPS 30
#define MS_PER_FRAME (1000 / TARGET_FPS)
#define TURTLE_CYCLE 140
// Cars stretch to the length given by their lane data; the nose and tail are
// fixed art, so anything shorter than the minimum has no body left to draw.
#define CAR_MIN_WIDTH 6
#define CAR_MAX_WIDTH 16
// The playfield: 5 road lanes and 5 river lanes, each carrying a few objects
#define LANE_COUNT 10
#define MAX_LANE_OBJECTS 4

// Objects wrap around once they pass these bounds. The range is wider than the
// screen so they slide in and out of view instead of popping in at the edge.
#define WRAP_MIN (-5)
#define WRAP_MAX 30

#define FROG_START_X 14
#define FROG_START_Y 24
#define FROG_MAX_X 27

#define START_LIVES 3
#define START_SCORE 1000.0f
#define SCORE_PER_LEVEL 500.0f
#define SCORE_DECAY 0.02f
#define LEVELS_TO_WIN 5

#define PHASES_FILE "phases.bin"
#define SAVE_FILE "save.txt"
// Stamped at the head of the level file. A mismatch means the file came from an
// older build, so it is discarded and rewritten rather than misread.
#define PHASES_MAGIC "FROG0001"
#define PHASES_MAGIC_LEN 8

// One lane of level data, exactly as it is stored in the level file
typedef struct game_data {
    char object_type;
    int size;
    int spacing;
    float speed;
    int count;
    int initial_row;
    int initial_column;
} GAME_DATA;

// A row of the playfield and the objects travelling along it
typedef struct lane {
    char  type;                 // 'C' car, 'T' log, 'R' turtle
    int   row;                  // top console row of the lane
    int   size;                 // object length, in game units
    int   spacing;              // gap between objects, in game units
    float speed;                // game units per frame; negative moves left
    int   count;                // objects sharing the lane
    int   start_x;              // column the first object is laid down at
    float x[MAX_LANE_OBJECTS];  // live positions
    int   anim;                 // turtle dive counter; ignored by cars and logs
} LANE;

typedef struct frog {
    float x;
    int   y;
} FROG;

typedef struct game_state {
    int   lives;
    int   difficulty;
    float score;
} GAME_STATE;

void hide_cursor() {
    HANDLE myconsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO CURSOR;
    GetConsoleCursorInfo(myconsole, &CURSOR);
    CURSOR.bVisible = FALSE;
    SetConsoleCursorInfo(myconsole, &CURSOR);
}

void reset_cursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD Position = {0, 0};
    SetConsoleCursorPosition(hOut, Position);
}

void set_color(int color_code) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color_code);
}

// Fills in one lane's design, clearing its live position and animation state
void set_lane(LANE *lane, char type, int row, int size, int spacing, float speed, int count, int start_x) {
    memset(lane, 0, sizeof(LANE));
    lane->type    = type;
    lane->row     = row;
    lane->size    = size;
    lane->spacing = spacing;
    lane->speed   = speed;
    lane->count   = count;
    lane->start_x = start_x;
}

// The built-in layout, used whenever there is no usable level file.
// Road lanes sit on rows 14-22, river lanes on rows 2-10, both bottom-up.
void init_default_data(LANE *lanes, FROG *frog) {
    //                    type  row  size  spacing   speed  count  start
    set_lane(&lanes[0],   'C',  14,    4,      15,  0.15f,     2,     0);
    set_lane(&lanes[1],   'C',  16,    4,      10, -0.25f,     2,     2);
    set_lane(&lanes[2],   'C',  18,    4,      20,  0.12f,     2,     5);
    set_lane(&lanes[3],   'C',  20,    4,      12, -0.30f,     2,     2);
    set_lane(&lanes[4],   'C',  22,    4,      18,  0.20f,     2,     2);
    set_lane(&lanes[5],   'R',  10,    4,      10,  0.12f,     3,     0);
    set_lane(&lanes[6],   'T',   8,    5,       6, -0.15f,     3,     2);
    set_lane(&lanes[7],   'R',   6,    4,       8,  0.17f,     3,     0);
    set_lane(&lanes[8],   'T',   4,    4,       6, -0.20f,     4,     2);
    set_lane(&lanes[9],   'R',   2,    4,      10,  0.22f,     3,     0);

    frog->x = FROG_START_X;
    frog->y = FROG_START_Y;
}

// Lays a lane's objects out from its starting column, evenly spaced
void reset_lane_positions(LANE *lane) {
    int i;
    for(i = 0; i < lane->count; i++)
        lane->x[i] = lane->start_x + i * (lane->size + lane->spacing);
}

// Writes the current layout to the level file: a magic stamp, one record per
// lane, then a final record holding the frog's starting square.
int write_phase_file(LANE *lanes, FROG *frog) {
    GAME_DATA data;
    FILE *file;
    int j;

    file = fopen(PHASES_FILE, "wb");
    if(!file) return -1;
    fwrite(PHASES_MAGIC, 1, PHASES_MAGIC_LEN, file);

    for(j = 0; j <= LANE_COUNT; j++) {
        // Clear the struct padding too, so the file is byte-for-byte reproducible
        memset(&data, 0, sizeof(GAME_DATA));
        if(j < LANE_COUNT) {
            data.object_type    = lanes[j].type;
            data.size           = lanes[j].size;
            data.spacing        = lanes[j].spacing;
            data.speed          = lanes[j].speed;
            data.count          = lanes[j].count;
            data.initial_row    = lanes[j].row;
            data.initial_column = lanes[j].start_x;
        } else {
            data.object_type    = 'S';
            data.size           = 1;
            data.count          = 1;
            data.initial_row    = frog->y;
            data.initial_column = (int)frog->x;
        }
        fwrite(&data, sizeof(GAME_DATA), 1, file);
    }
    fclose(file);
    return 0;
}

// Rejects records that would put an object off the playfield or overflow a lane
int lane_is_valid(GAME_DATA *data) {
    return (data->object_type == 'C' || data->object_type == 'T' || data->object_type == 'R')
        && data->size > 0 && data->spacing >= 0
        && data->count >= 1 && data->count <= MAX_LANE_OBJECTS
        && data->initial_row >= 0 && data->initial_row < SCREEN_HEIGHT - 1;
}

// Falls back to the built-in layout, leaving a fresh level file behind
int load_defaults(LANE *lanes, FROG *frog) {
    init_default_data(lanes, frog);
    write_phase_file(lanes, frog);
    return 1;
}

// mode 1: load the lane layout   mode 2: load a saved game   mode 3: save the game
int file_manager(int mode, LANE *lanes, FROG *frog, GAME_STATE *state) {
    GAME_DATA data;
    char magic[PHASES_MAGIC_LEN];
    FILE *file;
    int j, frog_y;
    float frog_x;

    switch(mode) {
        case 1: // Load lane layout
            file = fopen(PHASES_FILE, "rb");
            if(!file) return load_defaults(lanes, frog);

            if(fread(magic, 1, PHASES_MAGIC_LEN, file) != PHASES_MAGIC_LEN
               || memcmp(magic, PHASES_MAGIC, PHASES_MAGIC_LEN) != 0) {
                fclose(file);
                return load_defaults(lanes, frog);
            }
            for(j = 0; j < LANE_COUNT; j++) {
                // A short, stale or nonsensical file is dropped whole rather than
                // half-applied, which would leave the playfield in a broken state
                if(fread(&data, sizeof(GAME_DATA), 1, file) != 1 || !lane_is_valid(&data)) {
                    fclose(file);
                    return load_defaults(lanes, frog);
                }
                lanes[j].type    = data.object_type;
                lanes[j].row     = data.initial_row;
                lanes[j].size    = data.size;
                lanes[j].spacing = data.spacing;
                lanes[j].speed   = data.speed;
                lanes[j].count   = data.count;
                lanes[j].start_x = data.initial_column;
            }
            // Trailing frog record; its absence is not worth discarding the level over
            if(fread(&data, sizeof(GAME_DATA), 1, file) == 1) {
                frog->x = data.initial_column;
                frog->y = data.initial_row;
            } else {
                frog->x = FROG_START_X;
                frog->y = FROG_START_Y;
            }
            fclose(file);
            break;

        case 2: // Load Game
            file = fopen(SAVE_FILE, "r");
            if(!file) return -1;
            if(fscanf(file, "%d %d %f", &state->lives, &state->difficulty, &state->score) != 3) {
                fclose(file);
                return -1;
            }
            for(j = 0; j < LANE_COUNT; j++) {
                if(fscanf(file, " %c %d %d %d %f %d %d",
                          &lanes[j].type, &lanes[j].row, &lanes[j].size, &lanes[j].spacing,
                          &lanes[j].speed, &lanes[j].count, &lanes[j].start_x) != 7) {
                    fclose(file);
                    return -1;
                }
            }
            if(fscanf(file, " %f %d", &frog_x, &frog_y) == 2) {
                frog->x = frog_x;
                frog->y = frog_y;
            }
            fclose(file);
            break;

        case 3: // Save Game
            file = fopen(SAVE_FILE, "w");
            if(!file) return -1;
            // Run state on the first line, then one line per lane, then the frog
            fprintf(file, "%d %d %f\n", state->lives, state->difficulty, state->score);
            for(j = 0; j < LANE_COUNT; j++) {
                fprintf(file, "%c %d %d %d %f %d %d\n",
                        lanes[j].type, lanes[j].row, lanes[j].size, lanes[j].spacing,
                        lanes[j].speed, lanes[j].count, lanes[j].start_x);
            }
            fprintf(file, "%f %d\n", frog->x, frog->y);
            fclose(file);
            break;
    }
    return 0;
}

// --- VISUALS ---

void game_over() {
    system("cls");
    set_color(12);
    printf("\n\n\n\n");
    printf("    ##### ##### #   # #####   ##### #   # ##### #####\n");
    printf("    #     #   # ## ## #       #   # #   # #     #   #\n");
    printf("    # ### ##### # # # ####    #   #  # #  ####  #####\n");
    printf("    #   # #   # #   # #       #   #  # #  #     #  # \n");
    printf("    ##### #   # #   # #####   #####   #   ##### #   #\n");
    printf("\n\n");
    Sleep(2000);
}

void level_complete() {
    system("cls");
    set_color(10);
    printf("\n\n\n");
    printf("         #### #### ####   #  #   #  # ####\n");
    printf("         #  # #  # #  #   # #    ## # #  #\n");
    printf("         #### #### #  #    #     # # ####\n");
    printf("         #    #  # #  #   # #    #   #  # \n");
    printf("         #    #  # ####   #  #   #   #  # \n\n\n");
    Sleep(1500);
}

void instructions() {
    system("cls");
    set_color(15);
    puts("INSTRUCTIONS:");
    puts("Use Arrow Keys to move.");
    puts("Press Q to Save and Quit.");
    printf("\n");
    system("pause");
}

void menu() {
    system("cls");
    set_color(10);
    printf("\n\n");
    printf("      _______________________________________________\n");
    printf("     ||   ##### ##### ##### ##### ##### ##### #####   ||\n");
    printf("     ||   #     #   # #   # #     #     #     #   #   ||\n");
    printf("     ||   ####  ##### #   # # ### # ### ####  #####   ||\n");
    printf("     ||   #     #  #  #   # #   # #   # #     #  #    ||\n");
    printf("     ||   #     #   # ##### ##### ##### ##### #   #   ||\n");
    printf("     ||_______________________________________________||\n");
    set_color(11);
    printf("\n\n");
    printf("           1 - Play \n\n");
    printf("           2 - Load Game\n\n");
    printf("           3 - Instructions\n\n");
    set_color(12);
    printf("           4 - Exit \n\n\n");
    set_color(15);
}

void victory_screen() {
    system("cls");
    set_color(14);
    printf("\n\n\n");
    printf("         #   #  #  ##### #### ##### # ##### ##\n");
    printf("         #   #  #    #   #  # #   # # #   # ##\n");
    printf("          # #   #    #   #  # ##### # ##### ##\n");
    printf("          # #   #    #   #  # #  #  # #   # \n");
    printf("           #    #    #   #### #   # # #   # ##\n");
    Sleep(3000);
}


void clear_map(char game_map[SCREEN_HEIGHT][SCREEN_WIDTH]) {
    int i;
    // Top two rows: Win zones and dividers
    for (i = 0; i < 2; i++)
        strcpy(game_map[i], "      |**|      |**|      |**|      |**|      |**|          ");
    
    // River area
    for (i = 2; i < 12; i++)
        strcpy(game_map[i], "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    
    // Road/Safe area
    for (i = 12; i < SCREEN_HEIGHT; i++)
        strcpy(game_map[i], "                                                            ");
}

// Draws a car `cols` columns wide into two row buffers, which are left unterminated.
// The nose, tail and cabin are fixed art and the body between them stretches, so a
// car of any length keeps the same silhouette. Both rows are always exactly `cols`
// wide: a short row would leave NUL padding that draws as blank but still collides.
void build_car(char *top, char *bottom, int cols, int facing_right) {
    int i;
    for(i = 0; i < cols; i++) {
        top[i] = ' ';
        bottom[i] = ' ';
    }

    if(facing_right) {
        memcpy(top, " _|=\\", 5);
        for(i = 5; i < cols - 1; i++) top[i] = '_';
        memcpy(bottom, "/o", 2);
        for(i = 2; i < cols - 3; i++) bottom[i] = '_';
        memcpy(bottom + cols - 3, "o_\\", 3);
    } else {
        for(i = 1; i < cols - 5; i++) top[i] = '_';
        memcpy(top + cols - 5, "/=|_", 4);
        memcpy(bottom, "/_o", 3);
        for(i = 3; i < cols - 2; i++) bottom[i] = '_';
        memcpy(bottom + cols - 2, "o|", 2);
    }
}

// Renders a game object (Turtle, Log, Car, Frog) onto the map matrix.
// `size` is the object's length in game units; each unit is 2 console columns.
void render_object(char game_map[SCREEN_HEIGHT][SCREEN_WIDTH], int x, int y, int size, char type, int anim_frame, float speed) {
    int i, j;
    // Turtle graphics (Up, Warning, Down/Underwater)
    char turtle_up[2][2] = {"/\\", "\\/"};
    char turtle_warning[2][2] = {"\\/", "/\\"};
    char turtle_down[2][2] = {"~~", "~~"};
    // Log graphics
    char wood[2][2] = {"##", "##"};
    // Frog graphics
    char frog[2][2] = {"@@", "()"};
    // Car graphics, built on demand because their length comes from the lane data
    char car[2][CAR_MAX_WIDTH];
    int car_cols;

    // Console rendering uses 2 columns per game unit (x)
    int screenX = 2 * x;
    int screenY = y;

    switch(type) {
        case 'R': // Turtles
            while(size > 0) {
                for(i = 0; i < 2; i++) {
                    for(j = 0; j < 2; j++) {
                        if(screenX + j >= 0 && screenX + j < SCREEN_WIDTH - 1 && screenY + i >= 0 && screenY + i < SCREEN_HEIGHT) {
                            // Extended Cycle Logic (Total 140 frames)
                            // 0-80: Surface (80 frames)
                            // 81-90: Warning (10 frames)
                            // 91-125: Dive/Underwater (35 frames)
                            // 126-140: Resurfacing (15 frames)
                            if(anim_frame >= 0 && anim_frame <= 80) game_map[screenY+i][screenX+j] = turtle_up[i][j];
                            else if(anim_frame > 80 && anim_frame <= 90) game_map[screenY+i][screenX+j] = turtle_warning[i][j];
                            else if(anim_frame > 90 && anim_frame <= 125) game_map[screenY+i][screenX+j] = turtle_down[i][j];
                            else game_map[screenY+i][screenX+j] = turtle_warning[i][j];
                        }
                    }
                }
                screenX += 2;
                size--;
            }
            break;
        case 'T': // Logs
            while(size > 0) {
                for(i = 0; i < 2; i++) {
                    for(j = 0; j < 2; j++) {
                        if(screenX + j >= 0 && screenX + j < SCREEN_WIDTH - 1 && screenY + i >= 0 && screenY + i < SCREEN_HEIGHT)
                             game_map[screenY+i][screenX+j] = wood[i][j];
                    }
                }
                screenX += 2;
                size--;
            }
            break;
        case 'C': // Cars
            car_cols = 2 * size;
            if(car_cols < CAR_MIN_WIDTH) car_cols = CAR_MIN_WIDTH;
            if(car_cols > CAR_MAX_WIDTH) car_cols = CAR_MAX_WIDTH;
            build_car(car[0], car[1], car_cols, speed > 0);

            for(i = 0; i < 2; i++) {
                for(j = 0; j < car_cols; j++) {
                    if(screenX + j >= 0 && screenX + j < SCREEN_WIDTH - 1 && screenY + i >= 0 && screenY + i < SCREEN_HEIGHT)
                        game_map[screenY+i][screenX+j] = car[i][j];
                }
            }
            break;
        case 'S': // Frog
            for(i = 0; i < 2; i++) {
                for(j = 0; j < 2; j++) {
                    if(screenX + j >= 0 && screenX + j < SCREEN_WIDTH - 1 && screenY + i >= 0 && screenY + i < SCREEN_HEIGHT)
                        game_map[screenY+i][screenX+j] = frog[i][j];
                }
            }
            break;
    }
}

int check_collision(char game_map[SCREEN_HEIGHT][SCREEN_WIDTH], int x, int y) {
    int screenX = 2 * x;
    int screenY = y;

    if(screenX < 0 || screenX >= SCREEN_WIDTH || screenY < 0 || screenY >= SCREEN_HEIGHT) return 0;

    // Road Collision Check (Rows 13-23)
    if(screenY > 12 && screenY < 24) {
        char c1 = game_map[screenY][screenX];
        char c2 = (screenX+1 < SCREEN_WIDTH) ? game_map[screenY][screenX+1] : ' ';
        // Unwritten cells hold '\0' and draw as blank, so treat them as empty road
        if(c1 == '\0') c1 = ' ';
        if(c2 == '\0') c2 = ' ';
        // Collision if anything (car part) is drawn in the frog's spot
        if(c1 != ' ' || c2 != ' ') return 1;
    }

    // Water Collision Check (Rows 2-11)
    if(screenY >= 2 && screenY <= 11) {
        char c = game_map[screenY][screenX];
        // If on water ('~'), die. Must be on Log (#) or Turtle (/) or (\)
        if(c == '~') return 1;
    }

    return 0;
}

int check_win(char game_map[SCREEN_HEIGHT][SCREEN_WIDTH], int x, int y) {
    int screenX = 2 * x;
    int screenY = y;
    if(screenY < 2) {
        // Check if landed on a safe lily pad (*)
        if(game_map[screenY][screenX] == '*' || (screenX+1 < SCREEN_WIDTH && game_map[screenY][screenX+1] == '*'))
            return 1; // Win
        return -1; // Died (landed on grass/water in win area)
    }
    return 0; // Not at the top
}

void render_map(char game_map[SCREEN_HEIGHT][SCREEN_WIDTH], float score, int lives, int difficulty_level) {
    reset_cursor();
    
    set_color(15);
    printf(" Lives: %d   Score: %04d   Difficulty: %d \n", lives, (int)score, difficulty_level + 1);
    
    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        for(int j = 0; j < SCREEN_WIDTH; j++) {
            char c = game_map[i][j];
            if(c == '\0') c = ' ';
            
            // Color logic for the map
            if(i >= 12) { // Road/Safe Zone
                if(c == '_' || c == '|' || c == 'o' || c == '=' || c == '/' || c == '\\') set_color(12); // Car parts (red)
                else if(c == '@' || c == '(' || c == ')') set_color(10); // Frog (green)
                else set_color(7); // Road/Space (grey)
            } else { // River/Win Zone
                if(c == '~') set_color(3); // Water (cyan)
                else if(c == '#') set_color(6); // Log (brown/yellow)
                else if(c == '/' || c == '\\') set_color(10); // Turtle (green)
                else if(c == '@' || c == '(' || c == ')') set_color(10); // Frog (green)
                else if(c == '*') set_color(14); // Lilypad (yellow)
                else set_color(7); // Grass/Dividers (grey)
            }
            putchar(c);
        }
        if (i < SCREEN_HEIGHT - 1) putchar('\n'); // Avoid scroll on last line
    }
}

int main() {
    // Disable output buffering
    setbuf(stdout, NULL);

    system("TITLE frogger");
    // Set console size based on adjusted configuration
    char modeCmd[50];
    sprintf(modeCmd, "mode con:cols=%d lines=%d", CONSOLE_WIDTH, CONSOLE_HEIGHT);
    system(modeCmd);
    hide_cursor();

    char game_map[SCREEN_HEIGHT][SCREEN_WIDTH];
    LANE lanes[LANE_COUNT];
    FROG frog;
    GAME_STATE state = {START_LIVES, 0, START_SCORE};
    int i, k;

    char menu_choice;
    do {
        menu();
        menu_choice = getch();
        if(menu_choice == '1') {
            file_manager(1, lanes, &frog, &state);
            break;
        }
        else if(menu_choice == '2') {
            if(file_manager(2, lanes, &frog, &state) == 0) break;
        }
        else if(menu_choice == '3') instructions();
        else if(menu_choice == '4') return 0;
    } while(1);

    // Lay out each lane and stagger the turtle dive cycles, so that neighbouring
    // lanes never submerge at the same moment
    for(i = 0; i < LANE_COUNT; i++) {
        reset_lane_positions(&lanes[i]);
        lanes[i].anim = (i * TURTLE_CYCLE) / LANE_COUNT;
    }
    frog.x = FROG_START_X;
    frog.y = FROG_START_Y;

    int game_running = 1;
    clock_t lastTime = clock();

    while(state.lives > 0 && game_running) {
        clock_t currentTime = clock();
        double delta = (double)(currentTime - lastTime);

        // Framerate limit
        if (delta < MS_PER_FRAME) {
            Sleep(MS_PER_FRAME - delta);
            continue;
        }
        lastTime = currentTime;

        clear_map(game_map);

        float difficulty_factor = 1.0f + (state.difficulty * 0.1f);

        // Advance and draw every lane. An object that runs off one end of the
        // wrap range comes back on at the other.
        for(i = 0; i < LANE_COUNT; i++) {
            LANE *lane = &lanes[i];

            lane->anim++;
            if(lane->anim > TURTLE_CYCLE) lane->anim = 0;

            for(k = 0; k < lane->count; k++) {
                lane->x[k] += lane->speed * difficulty_factor;
                // Only wrap at the edge the object is heading towards, so one that
                // starts beyond the far edge slides into view instead of jumping
                if(lane->speed > 0) {
                    if(lane->x[k] > WRAP_MAX) lane->x[k] = WRAP_MIN;
                } else {
                    if(lane->x[k] < WRAP_MIN) lane->x[k] = WRAP_MAX;
                }

                render_object(game_map, (int)lane->x[k], lane->row,
                              lane->size, lane->type, lane->anim, lane->speed);
            }
        }

        // Input Handling
        if(_kbhit()) {
            char key = getch();
            if(key == -32) { // Arrow key input
                key = getch();
                if(key == 72 && frog.y > 0) frog.y -= 2;            // Up
                if(key == 80 && frog.y < FROG_START_Y) frog.y += 2; // Down
                if(key == 75 && frog.x > 0) frog.x -= 1;            // Left
                if(key == 77 && frog.x < FROG_MAX_X) frog.x += 1;   // Right
            }
            if(key == 'q' || key == 'Q') {
                file_manager(3, lanes, &frog, &state);
                game_running = 0;
            }
        }

        // A frog standing in the river drifts along with whatever lane it is on
        for(i = 0; i < LANE_COUNT; i++) {
            if(lanes[i].type != 'C' && lanes[i].row == frog.y)
                frog.x += lanes[i].speed * difficulty_factor;
        }

        // Keep frog within bounds
        if(frog.x < 0) frog.x = 0;
        if(frog.x > FROG_MAX_X) frog.x = FROG_MAX_X;

        // Collision Check
        if(check_collision(game_map, (int)frog.x, frog.y)) {
            state.lives--;
            frog.x = FROG_START_X;
            frog.y = FROG_START_Y;
            set_color(12);
            reset_cursor();
            printf("SPLASH! - You Died!");
            Sleep(500);
        }

        // Render Frog (after collision check, so it sits on the objects)
        render_object(game_map, (int)frog.x, frog.y, 1, 'S', 0, 0);

        // Win Condition Check
        int win = check_win(game_map, (int)frog.x, frog.y);
        if(win == 1) {
            state.difficulty++;
            state.score += SCORE_PER_LEVEL;
            level_complete();
            frog.x = FROG_START_X;
            frog.y = FROG_START_Y;
            if(state.difficulty >= LEVELS_TO_WIN) { victory_screen(); game_running = 0; }
        } else if (win == -1) {
            state.lives--;
            frog.x = FROG_START_X;
            frog.y = FROG_START_Y;
        }

        // Final Rendering and Score Update
        render_map(game_map, state.score, state.lives, state.difficulty);
        state.score -= SCORE_DECAY;
        if(state.score < 0) state.score = 0;
    }

    if(state.lives <= 0) game_over();
    return 0;
}