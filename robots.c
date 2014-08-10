/**
 * A text-based Robots Game
 *             _____
 *            |     |
 *            | | | |
 *            |_____|
 *      ____ ___|_|___ ____
 *     ()___)         ()___)
 *     // /|           |\ \\
 *    // / |           | \ \\
 *   (___) |___________| (___)
 *   (___)   (_______)   (___)
 *   (___)     (___)     (___)
 *   (___)      |_|      (___)
 *   (___)  ___/___\___   | |
 *    | |  |           |  | |
 *    | |  |___________| /___\
 *   /___\  |||     ||| //   \\
 *  //   \\ |||     ||| \\   //
 *  \\   // |||     |||  \\ //
 *   \\ // ()__)   (_ ()
 *         ///       \\\
 *        ///         \\\
 *      _///___     ___\\\_
 *     |_______|   |_______|
 */

#include <stdio.h>
#include <string.h>
#include <termios.h>   // for direct input
#include <unistd.h>    // also for direct input
#include <stdlib.h>    // for clearing screen
#include <time.h>      // for a time seed to supply for random ints
#include <sys/ioctl.h> // for getting terminal width

typedef enum { false, true } bool;

void set_direct_input ();
void restore_direct_input ();
void display_help ();
void display_version ();
void draw_start_screen ();
void draw_settings_screen ();
void draw_screen ();
void move_char (int x, int y);
void move_robots ();
void teleport ();
bool wait ();
void set_random_robots ();
void reset ();
void new_level ();
bool check_collision ();
bool all_dead ();
bool game_over ();
int get_int_len(int value);
int random_in_range (unsigned int min, unsigned int max);

// Needed for random number generation
static struct termios oldt, newt;

bool verbose = false;
char char_char = '@';
char dead_char = '#';
char *color_reset  = "\e[0m";
char *color_red    = "\e[0;31m";
char *color_yellow = "\e[0;33m";
char *color_blue   = "\e[0;34m";
int char_x;
int char_y;
char robots_char = '+';
int robots_speed = 1;
int initial_robots_num = 10; 
int robots_num;
int robots[1][3];
char junk_char = '*';
int field_lines = 20;
int field_columns = 20;

int new_level_bonus = 10;
int multiplier = 1;

int level = 1;
int score = 0;

// Controls
int controls = 1;
char up[]          = {'8', 'k'};
char up_right[]    = {'9', 'u'};
char right[]       = {'6', 'l'};
char down_right[]  = {'3', 'n'};
char down[]        = {'2', 'j'};
char down_left[]   = {'1', 'b'};
char left[]        = {'4', 'h'};
char up_left[]     = {'7', 'y'};
char stay[]        = {'5', '.'};


void main (int argc, char** argv)
{
    int i;
    bool override = false;
    for (i = 0; i < argc && !override; i++) {
        if (argv[i][0] == '-')  {
            switch (argv[i][1])  {
                case 'h':
                    display_help ();
                    override = true;
                    break;
                case 'v':
                    verbose = true;
                    break;
            }
        }
    }

    if (!override) {
        set_direct_input ();
        draw_start_screen ();

        robots_num = initial_robots_num;
        bool end = false;
        bool hit = false;
        bool level_end, retry;

        srand (time(NULL)); // Set time as seed for random ints
        teleport ();
        set_random_robots ();
        draw_screen ();
        while (!end) {
            char input = getchar ();
            if (input == up[controls]) {
                move_char (0, -1);
            } else if (input == up_right[controls]) {
                move_char (1, -1);
            } else if (input == right[controls]) {
                move_char (1, 0);
            } else if (input == down_right[controls]) {
                move_char (1, 1);
            } else if (input == down[controls]) {
                move_char (0, 1);
            } else if (input == down_left[controls]) {
                move_char (-1, 1);
            } else if (input == left[controls]) {
                move_char (-1, 0);
            } else if (input == up_left[controls]) {
                move_char (-1, -1);
            } else if (input == stay[controls]) {
                move_char (0,0);
            } else if (input == 't') {
                teleport ();
            } else if (input == 'w') {
                wait();
            }

            move_robots ();

            /* Needs to be before draw_screen (),
               because it also handles collisions.*/
            hit = check_collision ();
            level_end = all_dead ();
            if (level_end && !end) {
                new_level ();
                level_end = false;
            }
            draw_screen ();
            if (hit) {
                retry = game_over ();
                if (retry) {
                    reset ();
                } else {
                    end = true;
                }
            }
        }
        restore_direct_input ();
        system ("clear");
    }
}

void set_direct_input ()
{

    /* tcgetattr gets the parameters of the current terminal *
    STDIN_FILENO will tell tcgetattr that it should write the settings
    * of stdin to oldt*/
    tcgetattr( STDIN_FILENO, &oldt);
    /*now the settings will be copied*/
    newt = oldt;

    /*ICANON normally takes care that one line at a time will be
    processed * that means it will return if it sees a "\n" or an EOF
    or an EOL*/
    newt.c_lflag &= ~(ICANON);          

    /*Those new settings will be set to STDIN * TCSANOW tells
    tcsetattr to change attributes immediately. */
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
}

void restore_direct_input ()
{
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

void display_help ()
{
    printf ("help\n");
}

/**
 * Draws begin screen and gets preferences
 */
void draw_start_screen ()
{
    bool play = false;
    while (!play) {
        char text_filename[] = "start_screen";
        FILE *text_file = fopen(text_filename, "r");
        if (text_file == NULL) {
            fprintf(stderr, "Can't open title file %s!\n",
                    text_filename);
            exit(1);
        }

        struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);

        int term_columns = w.ws_col; // w.ws_row for lines

        char line[256];

        system("clear");
        while (fgets(line, sizeof(line), text_file)) {
            /* note that fgets don't strip the terminating \n,
            checking its * presence would allow to handle lines longer
            that sizeof(line) */
            int indent = (strlen(line) - term_columns) / 2;
            printf ("%*s%s", indent, "", line);
        }
        bool answer = false;
        char input;
        while (!answer) {
            printf ("\r%*s", (term_columns / 2), "");
            input = getchar ();
            if (input == 'p') {
                play = true;
                answer = true;
            } else if (input == 's') {
                draw_settings_screen ();
                answer = true;
            }
        }
    }
}

/**
 * Shows settings screen and
 * changes settings on user input
 */
void draw_settings_screen ()
{
    char text_filename[] = "settings_screen";
    FILE *text_file = fopen(text_filename, "r");
    if (text_file == NULL) {
        fprintf(stderr, "Can't open title file %s!\n",
                text_filename);
        exit(1);
    }

    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    int term_columns = w.ws_col; // w.ws_row for lines

    char line[256];

    system("clear");
    while (fgets(line, sizeof(line), text_file)) {
        /* note that fgets don't strip the terminating \n, checking
        its * presence would allow to handle lines longer that
        sizeof(line) */
        int indent = (strlen(line) - term_columns) / 2;
        printf ("%*s%s", indent, "", line);
    }
    bool answer = false;
    char input;
    while (!answer) {
        printf ("\r%*s", (term_columns / 2), "");
        input = getchar ();
        if (input == '1') {
            controls = 0;
            answer = true;
        } else if (input == '2') {
            controls = 1;
            answer = true;
        } else if (input == 'b') {
            answer = true;
        }
    }
}

void draw_screen ()
{
    system("clear");

    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    int term_columns = w.ws_col; // w.ws_row for lines

    int indent = ((((field_columns * 3) + 2) - term_columns) / 2);

    int i;
    int x;
    for (i = 0; i < (field_lines + 2); i++) {
        // Indent the line, to center it
        printf ("%*s", indent, "");
        // If the current line is the first or last, print a border
        if (i == 0 || i == ((field_lines + 2) - 1)) {
            printf ("_");
            for (x = 0; x < field_columns; x++) {
                printf ("___");
            }
            printf ("_");
            printf ("\n");
        }
        // Else, print normal line
        else {
            printf ("|");
            for (x = 0; x < field_columns; x++) {
                if (x == (char_x - 1) && i == char_y) {
                    int y;
                    bool char_dead = false;
                    for (y = 0; y < robots_num; y++) {
                        if (x == (robots[y][0] - 1) && i == robots[y][1]) {
                                char_dead = true;
                        }
                    }
                    if (!char_dead) {
                        printf (" %s%c%s ", 
                                color_blue, char_char, color_reset);
                    } else {
                        printf (" %s%c%s ", 
                                color_yellow, dead_char, color_reset);
                    }
                } else {
                    int y;
                    bool something_here = false;
                    for (y = 0; y < robots_num; y++) {
                        if (x == (robots[y][0] - 1) && i == robots[y][1]) {
                            // Robot is alive
                            if (robots[y][2] == 0) {
                                printf (" %s%c%s ",
                                        color_red, robots_char, color_reset);
                                something_here = true;
                            // Robot has become junk
                            } else if (robots[y][2] == 1) {
                                printf (" %s%c%s ", 
                                        color_yellow, junk_char, color_reset);
                                something_here = true;
                            }
                        }
                    }
                    if (!something_here) {
                        printf ("   ");
                    }
                }
            }
            printf ("|");
            printf ("\n");
        }
    }
    printf ("Level: %.d\n", level);
    if (verbose) {
        printf ("Char: %.d, %.d\n", char_x, char_y);
        printf ("Robot: %.d, %.d\n", robots[0][0], robots[0][1]);
    }
}

/**
 * Move the char by x, y
 * x, if positive moves to the right
 * y, if positive moves to the left
 */
void move_char (int x, int y)
{
   int new_x = char_x + x;
   int new_y = char_y + y;
   if (new_x <= field_columns && new_x >= 1) {
       char_x = new_x;
   }
   if (new_y <= field_lines && new_y >= 1)   {
       char_y = new_y;
   }
}

void move_robots ()
{
    int new_x, new_y, old_x, old_y;
    int i;
    bool change_x, change_y;
    for (i = 0; i < robots_num; i++) {
        if (robots[i][2] == 0) {
            old_x = robots[i][0];
            old_y = robots[i][1];

            change_x = false;
            change_y = false;

            // Movement on X-axis
            if (old_x < char_x) {
                new_x = old_x + robots_speed;
                change_x = true;
            } else if (old_x > char_x) {
                new_x = old_x - robots_speed;
                change_x = true;
            }

            // Movement on Y-axis
            if (old_y < char_y) {
                new_y = old_y + robots_speed;
                change_y = true;
            } else if (old_y > char_y) {
                new_y = old_y - robots_speed;
                change_y = true;
            }

            if (change_x) {
                robots[i][0] = new_x;
            }
            if (change_y) {
                robots[i][1] = new_y;
            }
        }
    }
}

void teleport ()
{
    char_x = random_in_range (1, 20);
    char_y = random_in_range (1, 20);
}

/**
 * Makes the player wait till either the level has been completed or
 * or he is killed
 */
bool wait ()
{
    bool hit = false;
    bool level_end = false;
    multiplier = 4
    while (!hit && !level_end) {
        move_robots ();
        hit = check_collision ();
        level_end = all_dead ();
        draw_screen ();
    }
}

void set_random_robots ()
{
    int i, x;
    bool checked; 
    for (i = 0; i < robots_num; i++) {
        checked = false;
        robots[i][2] = 0; // Robot is still alive
        while (!checked) {
            robots[i][0] = random_in_range (1, 20);
            robots[i][1] = random_in_range (1, 20);
            checked = true;
            if (robots[i][0] == char_x && robots[i][1] == char_y) {
                checked = false;
            }
            for (x = 0; x < i; x++) {
                if ((robots[x][0] == robots[i][0] &&
                        robots[x][1] == robots[i][1])) {
                    checked = false;
                }
            }
        }
    }
}

void reset ()
{
    robots_num = initial_robots_num;
    level = 1;
    score = 0;
    teleport ();
    set_random_robots ();
    draw_screen ();
}

/**
 * Initializes a new level
 */
void new_level ()
{
    draw_screen ();
    /* Show the player how he has won
     * and then continue. */
    sleep (1); // 1 second
    level++;
    score += new_level_bonus * level * multiplier;

    multiplier = 1;

    robots_num += 5;
    teleport ();
    set_random_robots ();
}

/**
 * Solves collisions of robots
 * and returns true if the player is hit
 */
bool check_collision ()
{
    int i, x;
    for (i = 0; i < robots_num; i++) {
        for (x = 0; x < i; x++) {
            if (robots[i][0] == robots[x][0] 
             && robots[i][1] == robots[x][1]) {
                robots[i][0] = 0;
                robots[i][1] = 0;
                // Indicate that robots are junk
                robots[i][2] = 1;
                // Leave one of the two robots in place as junk
                robots[x][2] = 1;
                
                // Update the score
                score += level;
                }
            }
        if (robots[i][0] == char_x &&
            robots[i][1] == char_y) {
            return true;
        }
    }
    return false;
}

/**
 * Returns true if all robots are dead
 */
bool all_dead ()
{
    int i;
    bool dead = true; // true to begin with false if one isnt
    for (i = 0; i < robots_num; i++) {
        // if robot is dead
        if (robots[i][2] == 0) {
            dead = false;
        }
    }
    return dead;
}

/**
 * Shows game over screen
 * returns true if player wants to retry
 */
bool game_over ()
{
    // Show the user how he died before continuing
    sleep (1);

    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    int term_columns = w.ws_col; // w.ws_row for lines

    char *game_over_str = "Game Over!";
    char *level_str = "You made it to level ";
    char *score_str = "Your score was ";
    char *retry_str = "Retry?[y/n]";

    int level_number_length = get_int_len (level);
    int score_number_length = get_int_len (score);

    int game_over_indent = (strlen(game_over_str) - term_columns) / 2;
    int level_indent = ((strlen(level_str) + level_number_length)
                        - term_columns) / 2;
    int score_indent = ((strlen (level_str) + score_number_length)
                        - term_columns) / 2;
    int retry_indent = (strlen(retry_str) - term_columns) / 2;

    system("clear");

    printf ("%*s%s\n", game_over_indent, "", game_over_str);
    printf ("%*s%s%.d\n", level_indent, "", level_str, level);
    if (score == 0) {
        char *score_zero_str = "Your score was 0";
        printf("%*s%s\n", (strlen(score_zero_str) - term_columns) / 2,
                "", score_zero_str);
    } else {
        printf ("%*s%s%.d\n", score_indent, "", score_str, score);
    }
    printf ("%*s%s\n", retry_indent, "", retry_str);

    bool answer = false, output;
    while (!answer) {
        printf ("\r%*s", (term_columns / 2), "");
        char input = getchar ();
        if (input == 'y') {
            output = true;
            answer = true;
        } else if (input == 'n') {
            output = false;
            answer = true;
        }
    }
    return output;
}

/**
 * Returns length of a int
 * when printed in decimals
 */
int get_int_len (int value){
    int l=1;
    while(value>9){ l++; value/=10; }
    return l;
}

int random_in_range (unsigned int min, unsigned int max)
{
    int base_random = rand (); /* in [0, RAND_MAX] */
    if (RAND_MAX == base_random) return random_in_range (min, max);
    /* now guaranteed to be in [0, RAND_MAX) */
    int range       = max - min,
        remainder   = RAND_MAX % range,
        bucket      = RAND_MAX / range;
    /* There are range buckets, plus one smaller interval
     *      within remainder of RAND_MAX */
    if (base_random < RAND_MAX - remainder) {
        return min + base_random/bucket;
    } else {
        return random_in_range (min, max);
    }
}
