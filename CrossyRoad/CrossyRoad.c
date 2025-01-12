#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_CARS 16
#define NUM_ROADS 6
#define ROAD_HEIGHT 2
#define ROAD_GAP 2
#define FROG_CHAR '@'
#define CAR_CHAR 'C'
#define STORK_CHAR 'S'
#define TREE_CHAR 'T'
#define EMPTY_CHAR ' '
#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 20
#define FROG_INITIAL_X (SCREEN_WIDTH / 2)
#define FROG_INITIAL_Y (SCREEN_HEIGHT - 2)
#define LEADERBOARD_FILE "leaderboard.txt"

struct Frog {
    int x, y;
    int is_moving;
    int on_friendly_car;
    int level;
};

struct Car {
    int x, y;
    int speed;
    int dir;
    int road;
    int stops;
    int is_friendly;
};

struct Stork {
    int x, y;
    int speed;
    int move_counter;
};

struct Tree {
    int x, y;
};

struct LeaderboardEntry {
    char name[50];
    int score;
};

void draw_play_area(struct Frog*);
void draw_play_area2(struct Frog*);
void draw_play_area3(struct Frog*);
void draw_frog(struct Frog*);
void draw_cars(struct Car*, int, struct Frog*);
void draw_stork(struct Stork*);
void draw_trees(struct Tree*, int);
void move_cars(struct Car*, int, struct Frog*);
void move_stork(struct Stork*, struct Frog*);
int check_collision(struct Frog*, struct Car*, int);
int check_stork_collision(struct Frog*, struct Stork*);
int check_tree_collision(struct Frog*, struct Tree*, int);
void draw_destination();
void draw_time(time_t, int);
void save_score(const char*, int);
void show_leaderboard();
void initialize_game(struct Frog*, struct Car*, struct Stork*, struct Tree*, int*);
void game_loop(struct Frog*, struct Car*, struct Stork*, struct Tree*, int, time_t);
void process_input(struct Frog*, struct Car*, struct Stork*, int, time_t*);
int check_game_conditions(struct Frog*, struct Car*, struct Stork*, struct Tree*, int, time_t);
void display_game_result(int, time_t);

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    start_color();

    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);

    struct Frog frog;
    struct Car cars[MAX_CARS];
    struct Stork stork;
    struct Tree trees[NUM_ROADS * 2];
    int tree_count = 0;

    initialize_game(&frog, cars, &stork, trees, &tree_count);
    time_t start_time = time(NULL);
    game_loop(&frog, cars, &stork, trees, tree_count, start_time);
    endwin();

    return 0;
}

int load_road_speeds(int* road_speeds) {
    FILE* speed_file = fopen("config.txt", "r");
    if (speed_file == NULL) {
        mvprintw(SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 - 20, "Error loading speeds! Generating random speeds...");
        refresh();
        napms(2000);
        srand((unsigned int)time(NULL));

        for (int i = 0; i < NUM_ROADS; i++) {
            road_speeds[i] = rand() % 2 + 1;
        }
        return -1;
    }

    for (int i = 0; i < NUM_ROADS; i++) {
        if (fscanf(speed_file, "%d", &road_speeds[i]) != 1) {
            mvprintw(SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 - 20, "Error reading speeds! Generating random speeds...");
            refresh();
            napms(2000);
            fclose(speed_file);
            srand((unsigned int)time(NULL));
            for (int j = 0; j < NUM_ROADS; j++) {
                road_speeds[j] = rand() % 2 + 1;
            }
            return -1;
        }
    }
    fclose(speed_file);
    return 0;
}


void initialize_cars(struct Car* cars, int* road_speeds) {
    struct Car predefined_cars[MAX_CARS] = {
            {5, 3, 1, 1, 0, 0, 0},
            {5, 7, 1, 1, 1, 0, 0},
            {25, 7, 1, 1, 1, 1, 0},
            {1, 11, 1, 1, 2, 0, 0},
            {26, 11, 1, 1, 2, 0, 0},
            {20, 11, 1, 1, 2, 0, 1},
            {10, 11, 1, 1, 2, 1, 0},
            {4, 15, 1, 1, 3, 0, 0},
            {15, 15, 1, 1, 3, 0, 0},
            {23, 15, 1, 1, 3, 0, 0},
            {4, 19, 1, 1, 4, 0, 0},
            {17, 19, 1, 1, 4, 0, 0},
            {23, 19, 1, 1, 4, 0, 0},
            {5, 23, 1, 1, 5, 0, 0},
            {19, 23, 1, 1, 5, 0, 0},
            {25, 23, 1, 1, 5, 0, 0}
    };

    for (int i = 0; i < MAX_CARS; i++) {
        cars[i] = predefined_cars[i];
        cars[i].speed = road_speeds[cars[i].road];
        cars[i].dir = (cars[i].road % 2 == 0) ? -1 : 1;
    }
}

void initialize_stork(struct Stork* stork) {
    *stork = (struct Stork){ 18, 1, 1, 0 };
}

void initialize_trees(struct Tree* trees, int* tree_count) {
    *tree_count = 0;
    struct Tree predefined_trees[] = {
            {5, 6},
            {28, 6},
            {15, 10},
            {27, 10},
            {23, 14},
            {4, 14},
            {9, 18},
            {25, 18},
    };

    for (int i = 0; i < sizeof(predefined_trees) / sizeof(predefined_trees[0]); i++) {
        trees[*tree_count] = predefined_trees[i];
        (*tree_count)++;
    }
}

void initialize_game(struct Frog* frog, struct Car* cars, struct Stork* stork, struct Tree* trees, int* tree_count) {
    *frog = (struct Frog){ FROG_INITIAL_X, FROG_INITIAL_Y, 0 };
    int road_speeds[NUM_ROADS] = { 0 };
    frog->level = 1;
    load_road_speeds(road_speeds);


    initialize_cars(cars, road_speeds);
    initialize_stork(stork);
    initialize_trees(trees, tree_count);
}

void change_car_speeds(struct Car* cars, int count) {
    for (int i = 0; i < count; i++) {
        if (cars[i].road == 0) {
            int new_speed = abs(3 - cars[i].speed);
            cars[i].speed = new_speed;
        }
    }
}

void level(int level, struct Frog* frog, struct Car* cars, struct Tree* trees, struct Stork* stork, time_t start_time, int tree_count) {
    if (level == 1) {
        draw_play_area(frog);
        draw_time(start_time, frog->level);
    }
    else if (level == 2) {
        draw_play_area2(frog);
        draw_time(start_time, frog->level);
    }
    if (level == 3) {
        draw_play_area3(frog);
        draw_time(start_time, frog->level);
    }
    draw_cars(cars, MAX_CARS, frog);
    draw_frog(frog);
    draw_trees(trees, tree_count);
    draw_stork(stork);
    draw_destination();
    move_cars(cars, MAX_CARS, frog);
    move_stork(stork, frog);
}
int speed(int speed_change_timer, struct Car* cars) {
    speed_change_timer++;
    if (speed_change_timer >= 9) {
        change_car_speeds(cars, MAX_CARS);
        speed_change_timer = 0;
    }
    return speed_change_timer;
}

void game_loop(struct Frog* frog, struct Car* cars, struct Stork* stork, struct Tree* trees, int tree_count, time_t start_time) {
    int speed_timer = 0;
    while (1) {
        clear();
        if (frog->level == 1) {
            level(1, frog, cars, trees, stork, start_time, tree_count);
            speed_timer = speed(speed_timer, cars);
        }
        else if (frog->level == 2) {
            level(2, frog, cars, trees, stork, start_time, tree_count);
            speed_timer = speed(speed_timer, cars);
        }
        else if (frog->level == 3) {
            level(3, frog, cars, trees, stork, start_time, tree_count);
            speed_timer = speed(speed_timer, cars);
        }

        int result = check_game_conditions(frog, cars, stork, trees, tree_count, start_time);
        if (result != 0 && result != 2) {
            display_game_result(result, start_time);
            break;
        }
        int input = getch();
        process_input(frog, cars, stork, input, &start_time);
        refresh();
        napms(80);
    }
}

void save_game_state(struct Frog* frog, struct Car* cars, struct Stork* stork, const char* filename, time_t start_time) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        mvprintw(SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 - 10, "Error saving game state!");
        refresh();
        napms(2000);
        return;
    }

    fprintf(file, "%d %d %d %d\n", frog->x, frog->y, frog->level, frog->is_moving);

    for (int i = 0; i < MAX_CARS; i++) {
        fprintf(file, "%d %d %d %d %d %d %d\n", cars[i].x, cars[i].y, cars[i].speed, cars[i].dir, cars[i].road, cars[i].stops, cars[i].is_friendly);
    }
    fprintf(file, "%d %d %d %d\n", stork->x, stork->y, stork->speed, stork->move_counter);
    time_t current_time = time(NULL);
    int elapsed_time = (int)(current_time - start_time);
    fprintf(file, "%d\n", elapsed_time);

    fclose(file);
}

int load_game_state(struct Frog* frog, struct Car* cars, struct Stork* stork, const char* filename, time_t* start_time) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        mvprintw(SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 - 10, "Error loading game state!");
        refresh();
        napms(2000);
        return -1;
    }

    fscanf_s(file, "%d %d %d %d", &frog->x, &frog->y, &frog->level, &frog->is_moving);
    for (int i = 0; i < MAX_CARS; i++) {
        fscanf_s(file, "%d %d %d %d %d %d %d", &cars[i].x, &cars[i].y, &cars[i].speed, &cars[i].dir, &cars[i].road, &cars[i].stops, &cars[i].is_friendly);
    }
    fscanf_s(file, "%d %d %d %d", &stork->x, &stork->y, &stork->speed, &stork->move_counter);

    int elapsed_time;
    fscanf(file, "%d", &elapsed_time);

    elapsed_time -= 2;
    *start_time = time(NULL) - elapsed_time;

    fclose(file);
    return 0;
}

void process_input(struct Frog* frog, struct Car* cars, struct Stork* stork, int input, time_t* start_time) {
    switch (input) {
    case KEY_UP:
        if (frog->y > 1) frog->y--;
        break;
    case KEY_DOWN:
        if (frog->y < SCREEN_HEIGHT - 2 && frog->level == 1 ||
            frog->y < SCREEN_HEIGHT + 2 && frog->level == 2 ||
            frog->y < SCREEN_HEIGHT + 6 && frog->level == 3)
            frog->y++;
        break;
    case KEY_LEFT:
        if (frog->x > 1) frog->x--;
        break;
    case KEY_RIGHT:
        if (frog->x < SCREEN_WIDTH - 2) frog->x++;
        break;

    case '1':
        save_game_state(frog, cars, stork, "game_state.txt", *start_time);
        break;
    case '2':
        if (load_game_state(frog, cars, stork, "game_state.txt", start_time) == 0) {
            mvprintw(SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 - 6, "Game loaded!");
            refresh();
            napms(2000);
        }
        break;

    default: break;
    }
}

int check_game_conditions(struct Frog* frog, struct Car* cars, struct Stork* stork, struct Tree* trees, int tree_count, time_t start_time) {
    if (check_collision(frog, cars, MAX_CARS)) {
        return -1;
    }
    if (check_stork_collision(frog, stork)) {
        return -2;
    }
    if (check_tree_collision(frog, trees, tree_count)) {
        return -3;
    }
    if (frog->y == 1) {
        if (frog->level == 3)
            return 1;
        else if (frog->y == 1) {
            frog->level++;
            frog->x = FROG_INITIAL_X;
            frog->y = FROG_INITIAL_Y + 4 * (frog->level - 1);
            initialize_trees(trees, &tree_count);
            initialize_stork(stork);
            return 2;
        }
    }
    return 0;
}

void display_game_result(int result, time_t start_time) {
    time_t end_time = time(NULL);
    int elapsed_time = (int)difftime(end_time, start_time);
    if (result == 1) {
        attron(COLOR_PAIR(1));
        mvprintw(SCREEN_HEIGHT / 2, 16, "YOU WON!");
        mvprintw(SCREEN_HEIGHT / 2 + 1, 11, "Time: %d seconds", elapsed_time);
        refresh();
        napms(2000);
        attroff(COLOR_PAIR(1));
        char name[50];
        echo();
        attron(COLOR_PAIR(1));
        mvprintw(SCREEN_HEIGHT + 3, 11, "Enter your name: ");
        getstr(name);
        attroff(COLOR_PAIR(1));
        noecho();
        save_score(name, elapsed_time);
    }
    else {
        mvprintw(SCREEN_HEIGHT + 1, 16, "GAME OVER!");
        mvprintw(SCREEN_HEIGHT + 2, 11, "Cause: %s",
            (result == -1) ? "Car collision" :
            (result == -2) ? "Caught by stork" :
            "Blocked by tree");
        refresh();
        napms(2000);
    }

    show_leaderboard();
}

void draw_play_area(struct Frog* frog) {
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            if (i == 0 || i == SCREEN_HEIGHT - 1 || j == 0 || j == SCREEN_WIDTH - 1) {
                mvaddch(i, j, '#');
            }
            else {
                mvaddch(i, j, EMPTY_CHAR);
            }
        }
    }
    for (int i = 0; i < NUM_ROADS; i++) {
        if (i == 4 || i == 5 && frog->level == 1) {
            continue;
        }
        int road_start_y = i * (ROAD_HEIGHT + ROAD_GAP) + 3;
        for (int j = 1; j < SCREEN_WIDTH - 1; j++) {
            mvaddch(road_start_y, j, '-');
            mvaddch(road_start_y + 1, j, '-');
        }
    }
}

void draw_play_area2(struct Frog* frog) {
    for (int i = 0; i < SCREEN_HEIGHT + 4; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            if (i == 0 || i == SCREEN_HEIGHT + 3 || j == 0 || j == SCREEN_WIDTH - 1) {
                mvaddch(i, j, '#');
            }
            else {
                mvaddch(i, j, EMPTY_CHAR);
            }
        }
    }
    for (int i = 0; i < NUM_ROADS; i++) {
        if (i == 5 && frog->level == 2) {
            continue;
        }
        int road_start_y = i * (ROAD_HEIGHT + ROAD_GAP) + 3;
        for (int j = 1; j < SCREEN_WIDTH - 1; j++) {
            mvaddch(road_start_y, j, '-');
            mvaddch(road_start_y + 1, j, '-');
        }
    }
}

void draw_play_area3(struct Frog* frog) {
    for (int i = 0; i < SCREEN_HEIGHT + 8; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            if (i == 0 || i == SCREEN_HEIGHT + 7 || j == 0 || j == SCREEN_WIDTH - 1) {
                mvaddch(i, j, '#');
            }
            else {
                mvaddch(i, j, EMPTY_CHAR);
            }
        }
    }
    for (int i = 0; i < NUM_ROADS; i++) {
        int road_start_y = i * (ROAD_HEIGHT + ROAD_GAP) + 3;
        for (int j = 1; j < SCREEN_WIDTH - 1; j++) {
            mvaddch(road_start_y, j, '-');
            mvaddch(road_start_y + 1, j, '-');
        }
    }
}
void draw_frog(struct Frog* frog) {
    mvaddch(frog->y, frog->x, FROG_CHAR);
}

void draw_cars(struct Car* cars, int count, struct Frog* frog) {
    for (int i = 0; i < count; i++) {
        if ((cars[i].road == 5 || cars[i].road == 4) && frog->level == 1) {
            continue;
        }
        if (cars[i].road == 5 && frog->level == 2) {
            continue;
        }
        if (cars[i].is_friendly) {
            attron(COLOR_PAIR(2));
        }
        else if (cars[i].stops == 1) {
            attron(COLOR_PAIR(3));
        }
        else {
            attron(COLOR_PAIR(5));
        }
        mvaddch(cars[i].y, cars[i].x, '[');
        mvaddch(cars[i].y, cars[i].x + 1, '=');
        mvaddch(cars[i].y, cars[i].x + 2, '=');
        mvaddch(cars[i].y, cars[i].x + 3, ']');
        mvaddch(cars[i].y + 1, cars[i].x, 'O');
        mvaddch(cars[i].y + 1, cars[i].x + 3, 'O');

        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));
        attroff(COLOR_PAIR(3));
        attroff(COLOR_PAIR(4));
        attroff(COLOR_PAIR(5));
        attroff(COLOR_PAIR(6));
    }
}

void draw_stork(struct Stork* stork) {
    attron(COLOR_PAIR(2));
    mvaddch(stork->y, stork->x, STORK_CHAR);
    attroff(COLOR_PAIR(2));
}

void draw_trees(struct Tree* trees, int count) {
    for (int i = 0; i < count; i++) {
        attron(COLOR_PAIR(3));
        mvprintw(trees[i].y - 1, trees[i].x, "( ");
        mvaddch(trees[i].y - 1, trees[i].x + 2, ')');
        attroff(COLOR_PAIR(3));
        attron(COLOR_PAIR(4));
        mvprintw(trees[i].y, trees[i].x, " |");
        attroff(COLOR_PAIR(4));
    }
}

void move_car(struct Car* car, struct Frog* frog) {
    if (car->stops == 1) {
        if (car->dir == 1) {
            if ((car->x + 5 >= frog->x) && (car->x + 4 <= frog->x) && (frog->y == car->y || frog->y == car->y + 1)) {
                return;
            }
        }
        else if (car->dir == -1) {
            if ((car->x - 1 <= frog->x) && (car->x >= frog->x) && (frog->y == car->y || frog->y == car->y + 1)) {
                return;
            }
        }
    }
    car->x += car->dir * car->speed;
    if (car->x < 0) {
        car->x = SCREEN_WIDTH - 4;
    }
    else if (car->x > SCREEN_WIDTH - 4) {
        car->x = 0;
    }
}

void move_frog_on_car(struct Car* car, struct Frog* frog, int* frog_new_x) {
    if (car->is_friendly == 1) {
        if (frog->y == car->y || frog->y == car->y + 1) {
            if (frog->x >= car->x + 1 && frog->x <= car->x + 4) {
                *frog_new_x += car->dir * car->speed;
                frog->on_friendly_car = 1;
                if (*frog_new_x < 0) *frog_new_x = 0;
                if (*frog_new_x > SCREEN_WIDTH - 2) *frog_new_x = SCREEN_WIDTH - 2;
            }
        }
    }
}

void move_cars(struct Car* cars, int count, struct Frog* frog) {
    int frog_new_x = frog->x;
    for (int i = 0; i < count; i++) {
        move_car(&cars[i], frog);
        move_frog_on_car(&cars[i], frog, &frog_new_x);
    }
    if (frog->on_friendly_car == 1 && frog->x != cars[0].x) {
        frog->on_friendly_car = 0;
    }
    frog->x = frog_new_x;
}

void move_stork(struct Stork* stork, struct Frog* frog) {
    stork->move_counter++;
    if (stork->move_counter >= 8) {
        stork->move_counter = 0;
        stork->move_counter = 0;
        if (frog->x < stork->x) stork->x -= stork->speed;
        else if (frog->x > stork->x) stork->x += stork->speed;
        if (frog->y < stork->y) stork->y -= stork->speed;
        else if (frog->y > stork->y) stork->y += stork->speed;
    }
}

int check_collision(struct Frog* frog, struct Car* cars, int count) {
    for (int i = 0; i < count; i++) {
        if (cars[i].is_friendly == 0) {
            if (frog->y == cars[i].y || frog->y == cars[i].y + 1) {
                if (frog->x >= cars[i].x && frog->x <= cars[i].x + 3) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int check_stork_collision(struct Frog* frog, struct Stork* stork) {
    if (frog->x == stork->x && frog->y == stork->y) {
        return 1;
    }
    return 0;
}

int check_tree_collision(struct Frog* frog, struct Tree* trees, int count) {
    for (int i = 0; i < count; i++) {
        if ((frog->x == trees[i].x || frog->x == trees[i].x + 2) && frog->y == trees[i].y - 1 || frog->x == trees[i].x + 1 && frog->y == trees[i].y) {
            return 1;
        }
        if (frog->x == trees[i].x && frog->y == trees[i].y) {
            return 1;
        }
    }
    return 0;
}

void draw_time(time_t start_time, int current_level) {
    time_t current_time = time(NULL);
    int elapsed_time = (int)difftime(current_time, start_time);
    mvprintw(0, 43, "Time: %d s", elapsed_time);
    mvprintw(1, 43, "Level: %d", current_level);
}

void draw_destination() {
    attron(COLOR_PAIR(1));
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        mvaddch(1, i, '=');
    }
    mvprintw(1, 18, "GOAL");
    attroff(COLOR_PAIR(1));
}

void save_score(const char* name, int score) {
    FILE* file = fopen(LEADERBOARD_FILE, "a");
    if (file) {
        fprintf(file, "%s %d\n", name, score);
        fclose(file);
    }
}

void show_leaderboard() {
    FILE* file = fopen(LEADERBOARD_FILE, "r");
    struct LeaderboardEntry results[100];
    int count = 0;
    while (fscanf_s(file, "%49s %d", results[count].name, (unsigned)_countof(results[count].name), &results[count].score) == 2) {
        count++;
        if (count >= 100) break;
    }
    fclose(file);

    if (count == 0) {
        printf("No results.\n");
        return;
    }
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (results[i].score > results[j].score) {
                struct LeaderboardEntry temp = results[i];
                results[i] = results[j];
                results[j] = temp;
            }
        }
    }
    printf("\nLEADERBOARD:\n");
    for (int i = 0; i < count; i++) {
        printf("%s - %d seconds\n", results[i].name, results[i].score);
    }
}