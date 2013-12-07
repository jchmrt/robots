/**
 * A ASCII Robots Game
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
void draw_screen ();
void move_char (int x, int y);
void move_robots ();
void teleport ();
void set_random_robots ();
void reset ();
void new_level ();
bool check_collision ();
bool all_dead ();
bool game_over ();
int random_in_range (unsigned int min, unsigned int max);

static struct termios oldt, newt;   // Needed for random number generation
bool verbose = false;
char char_char = '#';
char dead_char = '@';
int char_x;
int char_y;
char robots_char = '+';
int robots_speed = 1;
int initial_robots_num = 10; 
int robots_num;
int robots[1][3];
char junk_char = '*';
int lines = 20;
int columns = 20;

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
        robots_num = initial_robots_num;
        bool end = false;
        bool hit, level_end, retry;
        srand(time(NULL)); // Set time as seed for random ints
        teleport ();
        set_random_robots ();
        draw_screen ();
        while (!end) {
            char input = getchar ();
            switch (input) {
                case '8':
                    move_char (0, -1);
                    break;
                case '9':
                    move_char (1, -1);
                    break;
                case '6':
                    move_char (1, 0);
                    break;
                case '3':
                    move_char (1, 1);
                    break;
                case '2':
                    move_char (0, 1);
                    break;
                case '1':
                    move_char (-1, 1);
                    break;
                case '4':
                    move_char (-1, 0);
                    break;
                case '7':
                    move_char (-1, -1);
                    break;
                case 't':
                    teleport ();
                    break;
                default:
                    break;
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
    }
}

void set_direct_input ()
{

    /* tcgetattr gets the parameters of the current terminal
     * STDIN_FILENO will tell tcgetattr that it should write the settings
     * of stdin to oldt*/
    tcgetattr( STDIN_FILENO, &oldt);
    /*now the settings will be copied*/
    newt = oldt;

    /*ICANON normally takes care that one line at a time will be processed
     *     that means it will return if it sees a "\n" or an EOF or an EOL*/
    newt.c_lflag &= ~(ICANON);          

    /*Those new settings will be set to STDIN
     *     TCSANOW tells tcsetattr to change attributes immediately. */
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
}

void restore_direct_input () {
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

void display_help ()
{
    printf ("help\n");
}

void display_version ()
{
    printf ("version\n");
}

void draw_screen ()
{
    system("clear");
    int i;
    int x;
    for (i = 0; i < (lines + 2); i++) {
        // If the current line is the first or last, print a border
        if (i == 0 || i == ((lines + 2) - 1)) {
            printf ("_");
            for (x = 0; x < columns; x++) {
                printf ("___");
            }
            printf ("_");
            printf ("\n");
        }
        // Else, print normal line
        else {
            printf ("|");
            for (x = 0; x < columns; x++) {
                if (x == (char_x - 1) && i == char_y) {
                    int y;
                    bool char_dead = false;
                    for (y = 0; y < robots_num; y++) {
                        if (x == (robots[y][0] - 1) && i == robots[y][1]) {
                                char_dead = true;
                        }
                    }
                    if (!char_dead) {
                        printf (" %c ", char_char);
                    } else {
                        printf (" \e[0;33m%c\e[0m ", dead_char);
                    }
                } else {
                    int y;
                    bool something_here = false;
                    for (y = 0; y < robots_num; y++) {
                        if (x == (robots[y][0] - 1) && i == robots[y][1]) {
                            // Robot is alive
                            if (robots[y][2] == 0) {
                                printf (" %c ", robots_char);
                                something_here = true;
                            // Robot has become junk
                            } else if (robots[y][2] == 1) {
                                printf (" %c ", junk_char);
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
   if (new_x <= columns && new_x >= 1) {
       char_x = new_x;
   }
   if (new_y <= lines && new_y >= 1)   {
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
            if (robots[i][0] == robots[x][0] && robots[i][1] == robots[x][1]) {
                robots[i][0] = 0;
                robots[i][1] = 0;
                // Indicate that robots are junk
                robots[i][2] = 1;
                // Leave one of the two robots in place as junk
                robots[x][2] = 1;
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

    int columns = w.ws_col; // w.ws_row for lines

    char *game_over_str = "Game Over!";
    int indent = (strlen(game_over_str) - columns) / 2;

    system("clear");
    printf ("%*s%s\n", indent, "", game_over_str);
    printf ("%*s%s\n", (int)((strlen ("Retry?[y/n]") - columns) / 2), "", "Retry?[y/n]");
    bool answer = false, output;
    while (!answer) {
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
