#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

FILE *data_file; // arq
FILE *capture_file; // arq2
FILE *save_file; // arq3

typedef struct game_data {
    char object_type;
    int size;
    int spacing;
    float speed;
    int initial_row;
    int initial_column;
} GAME_DATA;

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

void init_default_data(char *objects, int *sizes, int *spacings, float *speeds, int *y_coords, int *x_coords) {
    // Row 0: Cars Right
    objects[0] = 'C'; sizes[0] = 2; spacings[0] = 15; speeds[0] = 0.15f; y_coords[0] = 14; x_coords[0] = 0;
    // Row 1: Cars Left
    objects[1] = 'C'; sizes[1] = 2; spacings[1] = 10; speeds[1] = 0.25f; y_coords[1] = 16; x_coords[1] = 2;
    // Row 2: Cars Right
    objects[2] = 'C'; sizes[2] = 3; spacings[2] = 20; speeds[2] = 0.12f; y_coords[2] = 18; x_coords[2] = 5;
    // Row 3: Cars Left
    objects[3] = 'C'; sizes[3] = 2; spacings[3] = 12; speeds[3] = 0.3f; y_coords[3] = 20; x_coords[3] = 2;
    // Row 4: Cars Right
    objects[4] = 'C'; sizes[4] = 2; spacings[4] = 18; speeds[4] = 0.2f; y_coords[4] = 22; x_coords[4] = 2;
    
    // River Order: Turtle, Log, Turtle, Log, Turtle
    
    // Row 5 (y=10): Turtle (Right, 4 units)
    objects[5] = 'R'; sizes[5] = 4; spacings[5] = 10; speeds[5] = 0.12f; y_coords[5] = 10; x_coords[5] = 0;
    
    // Row 6 (y=8): Log (Left, 5 units)
    objects[6] = 'T'; sizes[6] = 5; spacings[6] = 6; speeds[6] = 0.15f; y_coords[6] = 8; x_coords[6] = 2;
    
    // Row 7 (y=6): Turtle (Right, changed from 3 to 4 units)
    objects[7] = 'R'; sizes[7] = 4; spacings[7] = 8;  speeds[7] = 0.17f; y_coords[7] = 6; x_coords[7] = 0;
    
    // Row 8 (y=4): Log (Left, 4 units)
    objects[8] = 'T'; sizes[8] = 4; spacings[8] = 6; speeds[8] = 0.2f; y_coords[8] = 4; x_coords[8] = 2;
    
    // Row 9 (y=2): Turtle (Right, changed from 3 to 4 units)
    objects[9] = 'R'; sizes[9] = 4; spacings[9] = 10; speeds[9] = 0.22f; y_coords[9] = 2; x_coords[9] = 0;
    
    // Frog
    objects[10] = 'S'; sizes[10] = 1; spacings[10] = 0; speeds[10] = 0.0f; y_coords[10] = 24; x_coords[10] = 14;
}

int file_manager(int mode, char *objects, int *sizes, int *spacings, float *speeds, int *y_coords, int *x_coords, int *lives, int *difficulty_level, float *score) {
    GAME_DATA data;
    int j;
    char info_buffer[255];

    switch(mode) {
        case 0: // Capture Screen (unused but kept for structure)
            capture_file = fopen("screen_capture.txt","w");
            if(!capture_file) return -1;
            break;
        case 1: // Load Phase Data
            data_file = fopen("phases.bin","rb");
            if(!data_file) {
                init_default_data(objects, sizes, spacings, speeds, y_coords, x_coords);
                return 1;
            }
            for(j = 0; j < 11; j++) {
                fread(&data, sizeof(GAME_DATA), 1, data_file);
                objects[j] = data.object_type;
                sizes[j] = data.size;
                spacings[j] = data.spacing;
                y_coords[j] = data.initial_row;
                x_coords[j] = data.initial_column;
                speeds[j] = data.speed * 0.5f;
            }
            fclose(data_file);
            break;
        case 2: // Load Game
            save_file = fopen("save.txt","r");
            if(!save_file) return -1;
            for(j = 0; j < 11; j++) {
                objects[j] = getc(save_file);
                fgets(info_buffer, 255, save_file);
                char *token = strtok(info_buffer, " "); if(token) sizes[j] = atoi(token);
                token = strtok(NULL, " "); if(token) spacings[j] = atoi(token);
                token = strtok(NULL, " "); if(token) x_coords[j] = atoi(token);
                token = strtok(NULL, " "); if(token) y_coords[j] = atoi(token);
                token = strtok(NULL, " "); if(token) speeds[j] = atof(token);
                token = strtok(NULL, " "); if(token) *lives = atoi(token);
                token = strtok(NULL, " "); if(token) *difficulty_level = atoi(token);
                token = strtok(NULL, " "); if(token) *score = atof(token);
            }
            fclose(save_file);
            break;
        case 3: // Save Game
            save_file = fopen("save.txt","w");
            if(!save_file) return -1;
            for(j = 0; j < 11; j++) {
                fprintf(save_file, "%c %d %d %d %d %4.1f %d %d %f\n", 
                        objects[j], sizes[j], spacings[j], x_coords[j], y_coords[j], speeds[j], *lives, *difficulty_level, *score);
            }
            fclose(save_file);
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

// Renders a game object (Turtle, Log, Car, Frog) onto the map matrix
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
    // Car graphics (Right, Left)
    char car_right[2][8] = {{"_|=\\__"}, {"/o___o_\\"}};
    char car_left[2][8] = {{"__/=|_"}, {"/_o___o|"}};

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
            if(speed > 0) { // Right moving car
                for(i = 0; i < 2; i++) {
                    for(j = 0; j < 8; j++) {
                        if(screenX + j >= 0 && screenX + j < SCREEN_WIDTH - 1 && screenY + i >= 0 && screenY + i < SCREEN_HEIGHT)
                            game_map[screenY+i][screenX+j] = car_right[i][j];
                    }
                }
            } else { // Left moving car
                for(i = 0; i < 2; i++) {
                    for(j = 0; j < 8; j++) {
                        if(screenX + j >= 0 && screenX + j < SCREEN_WIDTH - 1 && screenY + i >= 0 && screenY + i < SCREEN_HEIGHT)
                            game_map[screenY+i][screenX+j] = car_left[i][j];
                    }
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
    // Game object properties
    int y_coords[11], sizes[11], spacings[11], x_coords[11];
    float x_positions[27], speeds[11];
    char objects[11];
    // Turtle animation counters
    int turtle_anim_1 = 0, turtle_anim_2 = 80;
    int turtle_anim_3 = 42; 
    // Game state
    int lives = 3, difficulty_level = 0;
    float score = 1000;
    
    char menu_choice;
    do {
        menu();
        menu_choice = getch();
        if(menu_choice == '1') {
            init_default_data(objects, sizes, spacings, speeds, y_coords, x_coords);
            break;
        }
        else if(menu_choice == '2') {
            if(file_manager(2, objects, sizes, spacings, speeds, y_coords, x_coords, &lives, &difficulty_level, &score) == 0) break;
        }
        else if(menu_choice == '3') instructions();
        else if(menu_choice == '4') return 0;
    } while(1);

    // Initialize object x_positions based on data
    x_positions[0] = x_coords[0]; x_positions[1] = x_coords[0] + spacings[0] + sizes[0];
    x_positions[2] = x_coords[1]; x_positions[3] = x_coords[1] + spacings[1] + sizes[1];
    x_positions[4] = x_coords[2]; x_positions[5] = x_coords[2] + spacings[2] + sizes[2];
    x_positions[6] = x_coords[3]; x_positions[7] = x_coords[3] + spacings[3] + sizes[3];
    x_positions[8] = x_coords[4]; x_positions[9] = x_coords[4] + spacings[4] + sizes[4];
    
    x_positions[10] = x_coords[5]; x_positions[11] = x_positions[10] + sizes[5] + spacings[5]; x_positions[12] = x_positions[11] + sizes[5] + spacings[5];
    x_positions[13] = x_coords[6]; x_positions[14] = x_positions[13] + sizes[6] + spacings[6]; x_positions[15] = x_positions[14] + sizes[6] + spacings[6];
    x_positions[16] = x_coords[7]; x_positions[17] = x_positions[16] + sizes[7] + spacings[7]; x_positions[18] = x_positions[17] + sizes[7] + spacings[7];
    x_positions[19] = x_coords[8]; x_positions[20] = x_positions[19] + sizes[8] + spacings[8]; x_positions[21] = x_positions[20] + sizes[8] + spacings[8]; x_positions[22] = x_positions[21] + sizes[8] + spacings[8];
    x_positions[23] = x_coords[9]; x_positions[24] = x_positions[23] + sizes[9] + spacings[9]; x_positions[25] = x_positions[24] + sizes[9] + spacings[9];

    x_positions[26] = 14; // Frog X position
    y_coords[10] = 24; // Frog Y position (row 24)

    int game_running = 1;
    clock_t lastTime = clock();

    while(lives > 0 && game_running) {
        clock_t currentTime = clock();
        double delta = (double)(currentTime - lastTime);
        
        // Framerate limit
        if (delta < MS_PER_FRAME) {
            Sleep(MS_PER_FRAME - delta);
            continue;
        }
        lastTime = currentTime;

        clear_map(game_map);

        float difficulty_factor = 1.0f + (difficulty_level * 0.1f);
        
        // Car Movement (Road)
        x_positions[0] += speeds[0] * difficulty_factor; if(x_positions[0] > 30) x_positions[0] = -5;
        x_positions[1] += speeds[0] * difficulty_factor; if(x_positions[1] > 30) x_positions[1] = -5;
        x_positions[4] += speeds[2] * difficulty_factor; if(x_positions[4] > 30) x_positions[4] = -5;
        x_positions[5] += speeds[2] * difficulty_factor; if(x_positions[5] > 30) x_positions[5] = -5;
        x_positions[8] += speeds[4] * difficulty_factor; if(x_positions[8] > 30) x_positions[8] = -5;
        x_positions[9] += speeds[4] * difficulty_factor; if(x_positions[9] > 30) x_positions[9] = -5;

        x_positions[2] -= speeds[1] * difficulty_factor; if(x_positions[2] < -5) x_positions[2] = 30;
        x_positions[3] -= speeds[1] * difficulty_factor; if(x_positions[3] < -5) x_positions[3] = 30;
        x_positions[6] -= speeds[3] * difficulty_factor; if(x_positions[6] < -5) x_positions[6] = 30;
        x_positions[7] -= speeds[3] * difficulty_factor; if(x_positions[7] < -5) x_positions[7] = 30;

        // Turtle Animation Cycle Update
        turtle_anim_1++; if(turtle_anim_1 > TURTLE_CYCLE) turtle_anim_1 = 0;
        turtle_anim_2--; if(turtle_anim_2 < 0) turtle_anim_2 = TURTLE_CYCLE;
        turtle_anim_3++; if(turtle_anim_3 > TURTLE_CYCLE) turtle_anim_3 = 0;

        // River Object Movement (Logs and Turtles)
        for(int k=10; k<=12; k++) { x_positions[k] += speeds[5] * difficulty_factor; if(x_positions[k] > 30) x_positions[k] = -5; }
        for(int k=13; k<=15; k++) { x_positions[k] -= speeds[6] * difficulty_factor; if(x_positions[k] < -5) x_positions[k] = 30; }
        for(int k=16; k<=18; k++) { x_positions[k] += speeds[7] * difficulty_factor; if(x_positions[k] > 30) x_positions[k] = -5; }
        for(int k=19; k<=22; k++) { x_positions[k] -= speeds[8] * difficulty_factor; if(x_positions[k] < -5) x_positions[k] = 30; }
        for(int k=23; k<=25; k++) { x_positions[k] += speeds[9] * difficulty_factor; if(x_positions[k] > 30) x_positions[k] = -5; }

        // Render Cars
        render_object(game_map, (int)x_positions[0], y_coords[0], sizes[0]*2, objects[0], 1, 1);
        render_object(game_map, (int)x_positions[1], y_coords[0], sizes[0]*2, objects[0], 1, 1);
        render_object(game_map, (int)x_positions[4], y_coords[2], sizes[2]*2, objects[2], 1, 1);
        render_object(game_map, (int)x_positions[5], y_coords[2], sizes[2]*2, objects[2], 1, 1);
        render_object(game_map, (int)x_positions[8], y_coords[4], sizes[4]*2, objects[4], 1, 1);
        render_object(game_map, (int)x_positions[9], y_coords[4], sizes[4]*2, objects[4], 1, 1);
        render_object(game_map, (int)x_positions[2], y_coords[1], sizes[1]*2, objects[1], 1, -1);
        render_object(game_map, (int)x_positions[3], y_coords[1], sizes[1]*2, objects[1], 1, -1);
        render_object(game_map, (int)x_positions[6], y_coords[3], sizes[3]*2, objects[3], 1, -1);
        render_object(game_map, (int)x_positions[7], y_coords[3], sizes[3]*2, objects[3], 1, -1);
        
        // Render Logs and Turtles
        for(int k=10; k<=12; k++) render_object(game_map, (int)x_positions[k], y_coords[5], sizes[5], objects[5], turtle_anim_1, 1); 
        for(int k=13; k<=15; k++) render_object(game_map, (int)x_positions[k], y_coords[6], sizes[6], objects[6], 1, -1); 
        for(int k=16; k<=18; k++) render_object(game_map, (int)x_positions[k], y_coords[7], sizes[7], objects[7], turtle_anim_3, 1); 
        for(int k=19; k<=22; k++) render_object(game_map, (int)x_positions[k], y_coords[8], sizes[8], objects[8], 1, -1); 
        for(int k=23; k<=25; k++) render_object(game_map, (int)x_positions[k], y_coords[9], sizes[9], objects[9], turtle_anim_1, 1); 

        // Input Handling
        if(_kbhit()) {
            char key = getch();
            if(key == -32) { // Arrow key input
                key = getch();
                if(key == 72 && y_coords[10] > 0) y_coords[10] -= 2; // Up
                if(key == 80 && y_coords[10] < 24) y_coords[10] += 2; // Down
                if(key == 75 && x_positions[26] > 0) x_positions[26] -= 1; // Left
                if(key == 77 && x_positions[26] < 27) x_positions[26] += 1; // Right
            }
            if(key == 'q' || key == 'Q') {
                file_manager(3, objects, sizes, spacings, speeds, y_coords, x_coords, &lives, &difficulty_level, &score);
                game_running = 0;
            }
        }

        // Frog carried by logs/turtles (River only)
        if(y_coords[10] <= 11 && y_coords[10] >= 2) {
             if(y_coords[10] >= 10) x_positions[26] += speeds[5] * difficulty_factor; // Row 10 (Turtle right)
             else if(y_coords[10] >= 8) x_positions[26] -= speeds[6] * difficulty_factor; // Row 8 (Log left)
             else if(y_coords[10] >= 6) x_positions[26] += speeds[7] * difficulty_factor; // Row 6 (Turtle right)
             else if(y_coords[10] >= 4) x_positions[26] -= speeds[8] * difficulty_factor; // Row 4 (Log left)
             else if(y_coords[10] >= 2) x_positions[26] += speeds[9] * difficulty_factor; // Row 2 (Turtle right)
        }
        
        // Keep frog within bounds
        if(x_positions[26] < 0) x_positions[26] = 0;
        if(x_positions[26] > 27) x_positions[26] = 27;

        // Collision Check
        if(check_collision(game_map, (int)x_positions[26], y_coords[10])) {
            lives--;
            x_positions[26] = 14;
            y_coords[10] = 24;
            set_color(12);
            reset_cursor();
            printf("SPLASH! - You Died!"); 
            Sleep(500);
        }
        
        // Render Frog (after collision check, so it sits on the objects)
        render_object(game_map, (int)x_positions[26], y_coords[10], sizes[10], objects[10], turtle_anim_2, 0);

        // Win Condition Check
        int win = check_win(game_map, (int)x_positions[26], y_coords[10]);
        if(win == 1) {
            difficulty_level++;
            score += 500;
            level_complete();
            x_positions[26] = 14;
            y_coords[10] = 24;
            if(difficulty_level >= 5) { victory_screen(); game_running = 0; }
        } else if (win == -1) {
            lives--; x_positions[26] = 14; y_coords[10] = 24;
        }

        // Final Rendering and Score Update
        render_map(game_map, score, lives, difficulty_level);
        score -= 0.02f; 
        if(score < 0) score = 0;
    }

    if(lives <= 0) game_over();
    return 0;
}