//
// TODO description
//
// Public domain.
//

#define _DEFAULT_SOURCE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>

#define _BSD_SOURCE

//int WIDTH  = 256;
//int HEIGHT = 32;
int WIDTH  = 400;
int HEIGHT = 60;

// -1 means center of the screen
int X = -1;
int Y = -1;

int SECONDS_DELAY = 2;

char _FONT[256];
char _BG[9];
char _FG[9];

char ICON_TEXT[256] = "☀";

// The delay between refreshing mixer values
unsigned long REFRESH_SPEED = 50000;

char *LOCK_FILE = "/tmp/dzbri";

void get_brightness(float *bri);

void print_usage(void)
{
    remove(LOCK_FILE);
    puts("Usage: dzbri [options]");
    puts("Most options are the same as dzen2\n");
    puts("\t-h|--help\tdisplay this message and exit (0)");
    puts("\t-x X POS\tmove to the X coordinate on your screen, -1 will center (default: -1)");
    puts("\t-y Y POS\tmove to the Y coordinate on your screen, -1 will center (default: -1)");
    puts("\t-w WIDTH\tset the width (default: 256)");
    puts("\t-h HEIGHT\tset the height (default: 32)");
    puts("\t-d|--delay DELAY  time it takes to exit when volume doesn't change, in seconds (default: 2)");
    puts("\t-bg\t\tset the background color");
    puts("\t-fg\t\tset the foreground color");
    puts("\t-fn\t\tset the font face; same format dzen2 would accept");
    puts("\t-s|--speed\tmicroseconds to poll alsa for the volume");
    puts("\t\t\thigher amounts are slower, lower amounts (<20000) will begin to cause high CPU usage");
    puts("\t\t\tsee `man 3 usleep`");
    puts("\t\t\tdefaults to 50000\n");
    puts("Note that dzbri does NOT background itself.");
    puts("It DOES, however, allow only ONE instance of itself to be running at a time by creating /tmp/dzbri as a lock.");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    // command line arguments {{{
    for(int i = 1; i < argc; i++)
        if(strcmp(argv[i], "-v") == 0 ||
           strcmp(argv[i], "--version") == 0)
        {
            puts("dzbri: 1.0");
            puts("Nick Allevato, inspired by bruenig's dvol");
            remove(LOCK_FILE);
            exit(EXIT_SUCCESS);
        }

        else if(strcmp(argv[i], "-bg") == 0)
            strcpy(_BG, argv[++i]);

        else if(strcmp(argv[i], "-fg") == 0)
            strcpy(_FG, argv[++i]);

        else if(strcmp(argv[i], "-fn") == 0)
            strcpy(_FONT, argv[++i]);

        else if(strcmp(argv[i], "-x") == 0)
            X = atoi(argv[++i]);

        else if(strcmp(argv[i], "-y") == 0)
            Y = atoi(argv[++i]);

        else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--delay") == 0)
            SECONDS_DELAY = atoi(argv[++i]);

        else if(strcmp(argv[i], "-w") == 0)
            WIDTH = atoi(argv[++i]);

        else if(strcmp(argv[i], "-h") == 0)
            HEIGHT = atoi(argv[++i]);

        else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--speed") == 0)
            REFRESH_SPEED = atoi(argv[++i]);

        else if(strcmp(argv[i], "--help") == 0)
            print_usage();
    // }}}

    // Create a file in /tmp/ to see if this was already running
    if(access(LOCK_FILE, F_OK) == -1)
    {
        FILE *f = fopen(LOCK_FILE, "w");
        fprintf(f, "%d", getpid());
        fclose(f);
    }
    else
        return 0;

    // get X and Y
    Display *xdisp = XOpenDisplay(NULL);
    Screen *screen = DefaultScreenOfDisplay(xdisp);

    // screw division, bit shift to the right
    if(X == -1)
        X = (screen->width >> 1) - (WIDTH >> 1);

    if(Y == -1)
        Y = (screen->height >> 1) - HEIGHT;

    // Calculate the progress bar width and height from a base percentage.
    // I calculated these by coming up with a solid foundation for dimensions, then
    // divided to get a percentage for preservation of proportions.
    long pbar_height = lround(HEIGHT * 0.34);
    long pbar_max_width = lround(WIDTH * 0.53);
    long ltext_x = lround(WIDTH * 0.078);
    long rtext_x = lround(WIDTH * 0.74);

    char *command = malloc(sizeof(char) * 256);

    char BG[24];
    char FG[24];
    char FONT[270];

    // Do color and font checks
    if(strcmp(_BG, "") != 0)
        sprintf(BG, "-bg '%s'", _BG);
    else
        strcpy(BG, "");

    if(strcmp(_FG, "") != 0)
        sprintf(FG, "-fg '%s'", _FG);
    else
        strcpy(FG, "");

    if(strcmp(_FONT, "") != 0)
        sprintf(FONT, "-fn '%s'", _FONT);
    else
        strcpy(FONT, "");

    sprintf(command, "dzen2 -ta l -x %d -y %d -w %d -h %d %s %s %s", X, Y, WIDTH, HEIGHT,
            BG, FG, FONT);

    FILE *stream = popen(command, "w");

    free(command);

    float prev_bri = -1;
    int prev_switch_value;

    // The time we should stop
    time_t current_time = (unsigned)time(NULL);
    time_t stop_time = current_time + SECONDS_DELAY;

    while(current_time <= stop_time)
    {
        current_time = (unsigned)time(NULL);

        float bri;

        get_brightness(&bri);

        if(prev_bri != bri)
        {
            if (bri > 0.5){
                strcpy(ICON_TEXT, "☀");
            } else {
                strcpy(ICON_TEXT, "☼");
            }
            char *string = malloc(sizeof(char) * 512);
            sprintf(string, "^pa(+%ldX)%s^pa()  ^r(%ldx%ld) ^pa(+%ldX)%3.0f%%^pa()\n",
                    ltext_x, ICON_TEXT,
                    lround(pbar_max_width * bri), pbar_height, rtext_x, bri*100);
            fprintf(stream, string);
            fflush(stream);
            free(string);

            prev_bri = bri;

            stop_time = current_time + SECONDS_DELAY;
        }

        usleep(REFRESH_SPEED);
    }

    fflush(NULL);
    pclose(stream);
    remove(LOCK_FILE);
    return 0;
}

void get_brightness(float *bri)
{
    FILE * fp;
    char brightness[256];

    // Open the command for getting brightness
    fp = popen("/bin/xbacklight" ,"r");
    if (fp == NULL){
      printf("Failed to run /bin/xbacklight\n");
      exit(1);
    }

    // Read the output
    fgets(brightness, sizeof(brightness)-1, fp);


    *bri = atof(brightness) / 100;
}
