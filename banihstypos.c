/* (C) folkert van heusden <mail@vanheusden.com>
   Released under MIT license
*/

#include <ncurses.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#define DICT_DIR "/usr/share/dict"

#define N_STARS 40

int win_w = 80;
int win_h = 24;
int laser_x = 5;
int laser_max_x = 75;
int speed_th = 5;
int laser_move_th = 4;
double start_delay = 0.8;
double delay_mult = 0.79;

void version(void)
{
	printf("banihstypos v" VERSION ", (C) 2007-2023 by folkert@vanheusden.com\n\n");
}

double get_ts(void)
{
	struct timeval ts;

	if (gettimeofday(&ts, NULL) == -1)
	{
		endwin();
		fprintf(stderr, "gettimeofday() failed with error %s\n", strerror(errno));
		exit(1);
	}

	return (double)ts.tv_sec + (double)ts.tv_usec/1000000.0;
}

char * get_word(const char **words, size_t n_words)
{
	return strdup(words[lrand48() % n_words]);
}

void draw_screen(int *stars, int word_x, char *word, int word_cnt, double cur_delay, int points, int n_words)
{
	int loop;
	static int laser_state = 0;
	static int stars_state = 0;
	char *buffer = strdup(word);

	laser_state = !laser_state;
	stars_state = !stars_state;

	if (strlen(buffer) > (win_w - word_x))
	{
		buffer[win_w - word_x] = 0x00;
	}

	werase(stdscr);

	/* show stars */
	wattron(stdscr, COLOR_PAIR(1));
	for(loop=0; loop<N_STARS; loop++)
	{
		int cur_stars_state = stars_state;

		if ((loop & 1) == 1)
			cur_stars_state = !stars_state;

		if (cur_stars_state)
			wattron(stdscr, A_BOLD);

		mvwprintw(stdscr, stars[loop * 2 + 1], stars[loop * 2 + 0], ".");

		wattroff(stdscr, A_BOLD);
	}
	wattroff(stdscr, COLOR_PAIR(1));

	/* draw borders */
	wattron(stdscr, COLOR_PAIR(4));
	wattron(stdscr, A_REVERSE);
	for(loop=0; loop<win_w; loop++)
	{
		mvwprintw(stdscr, 0, loop, " ");
		mvwprintw(stdscr, win_h - 1, loop, " ");
	}
	wattroff(stdscr, A_REVERSE);
	wattroff(stdscr, COLOR_PAIR(4));

	/* show statistics */
	wattron(stdscr, COLOR_PAIR(4));
	wattron(stdscr, A_REVERSE);
	mvwprintw(stdscr, win_h - 1, 0, "Words ok: %d, scroll delay: %.2f, points: %d, # words: %d", word_cnt, cur_delay, points, n_words);
	wattroff(stdscr, A_REVERSE);
	wattroff(stdscr, COLOR_PAIR(4));

	/* print word */
	mvwprintw(stdscr, win_h / 2, word_x, "%s", buffer);

	/* draw laser base */
	wattron(stdscr, COLOR_PAIR(3));
	wattron(stdscr, A_REVERSE);
	mvwprintw(stdscr, 1, laser_x - 1, "   ");
	mvwprintw(stdscr, win_h - 2, laser_x - 1, "   ");
	wattroff(stdscr, A_REVERSE);
	wattroff(stdscr, COLOR_PAIR(3));

	/* show laser */
	wattron(stdscr, COLOR_PAIR(2));
	if (laser_state)
		wattron(stdscr, A_BOLD);
	for(loop=2; loop<(win_h - 2); loop++)
	{
		mvwprintw(stdscr, loop, laser_x, "|");
	}
	wattroff(stdscr, A_BOLD);
	wattroff(stdscr, COLOR_PAIR(2));

	/* banner */
	wattron(stdscr, COLOR_PAIR(4));
	wattron(stdscr, A_REVERSE | A_BOLD);
	mvwprintw(stdscr, 0, 28, "banihstypos v" VERSION ", written by folkert@vanheusden.com");
	wattroff(stdscr, A_REVERSE | A_BOLD);
	wattroff(stdscr, COLOR_PAIR(4));

	wmove(stdscr, win_h - 1, win_w - 1);

	wrefresh(stdscr);
	doupdate();

	free(buffer);
}

void init_ncurses()
{
	initscr();

	cbreak();

	start_color();

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_GREEN, COLOR_BLACK);
}

void init_bg_stars(int *stars)
{
	int loop;

	for(loop=0; loop<N_STARS; loop++)
	{
		stars[loop * 2 + 0] = lrand48() % win_w;
		stars[loop * 2 + 1] = lrand48() % win_h;
	}
}

void game_loop(const char **words, size_t n_words)
{
	struct pollfd fds[] = { { 0, POLLIN, 0 } };
	char *word = get_word(words, n_words);
	double delay = start_delay;
	int word_cnt = 0;
	int word_x = win_w - 1;
	double start_ts = get_ts();
	int points = 0;
	int pointsadd = 2;
	int stars[N_STARS * 2];
	char nok = 0;

	init_bg_stars(stars);

	init_ncurses();

	for(;;)
	{
		double cur_delay = delay - (get_ts() - start_ts);

		draw_screen(stars, word_x, word, word_cnt, delay, points, n_words);

		if (cur_delay > 0 && poll(fds, 1, cur_delay * 1000) == -1)
		{
			endwin();
			fprintf(stderr, "poll failed: %s\n", strerror(errno));
			exit(0);
		}

		if (start_ts + delay < get_ts())
		{
			word_x--;

			if (word_x == laser_x)
			{
				flash();
				beep();

				memcpy(&word[0], &word[1], strlen(word));

				points -= (pointsadd + 1);

				word_x = laser_x + 1;

				nok = 1;
			}

			start_ts = get_ts();
		}

		if (fds[0].revents)
		{
			int c = getch();

			if (c == 3)	/* exit program */
			{
				endwin();

				version();

				exit(0);
			}

			if (c == word[0])
			{
				points += pointsadd;

				memcpy(&word[0], &word[1], strlen(word));
			}
			else
			{
				points -= (pointsadd - 1);
				beep();
			}
		}

		if (strlen(word) == 0)
		{
			free(word);

			word_x = win_w - 1;
			word = get_word(words, n_words);

			if (!nok)
				word_cnt++;
			nok = 0;
			if ((word_cnt % speed_th) == 0)
			{
				delay *= delay_mult;
				pointsadd++;
			}
			else if ((word_cnt % laser_move_th) == 0)
			{
				if (laser_x < laser_max_x)
					laser_x++;
			}
		}
	}
}

void add_file(const char ***filenames, size_t *n, const char *name)
{
	*filenames = (const char **)realloc(*filenames, (*n + 1) * sizeof(char *));
	(*filenames)[*n] = name;
	(*n)++;
}

void find_words_file(const char ***filenames, size_t *n)
{
	struct dirent *de;
	DIR *dir = opendir(DICT_DIR);

	if (!dir)
	{
		fprintf(stderr, "Default directory which is searched for words lists (" DICT_DIR ") cannot be opened: %s\n", strerror(errno));
		exit(1);
	}

	while((de = readdir(dir)) != NULL)
	{
		if (strcmp(de -> d_name, ".") != 0 &&
		    strcmp(de -> d_name, "..") != 0)
		{
			char filename_buffer[4096];
			snprintf(filename_buffer, sizeof(filename_buffer), DICT_DIR "/%s", de -> d_name);

			add_file(filenames, n, strdup(filename_buffer));
		}
	}
}

void load_words_file(const char *file, char lowercase_only, const char ***words, size_t *n_words)
{
	FILE *fh = fopen(file, "r");
	if (!fh)
	{
		fprintf(stderr, "Cannot open words file %s for read access: %s\n", file, strerror(errno));
		exit(1);
	}

	printf("Loading file %s...\n", file);

	while(!feof(fh))
	{
		char read_buffer[4096], *start = read_buffer, *space, *lf;

		if (!fgets(read_buffer, sizeof(read_buffer), fh))
			break;

		read_buffer[win_w] = 0x00;

		*words = realloc(*words, sizeof(char *) * (*n_words + 1));
		if (!*words)
		{
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}

		while(*start == ' ') start++;

		space = strchr(start, ' ');
		if (space) *space = 0x00;

		lf = strchr(start, '\n');
		if (lf) *lf = 0x00;

		if (lowercase_only)
		{
			int loop, len = strlen(start);

			for(loop=0; loop<len; loop++)
				start[loop] = tolower(start[loop]);
		}

		if (start[0] != 0x00)
		{
			(*words)[*n_words] = strdup(start);
			(*n_words)++;
		}
	}

	fclose(fh);
}

void help(void)
{
	version();

	printf("-f file	select file with words to use\n");
	printf("-l	only lowercase words\n");
	printf("-h	this help\n");
	printf("-V	show version\n");
}

int main(int argc, char *argv[])
{
	const char **words = NULL;
	size_t n_words = 0;
	const char **words_files = NULL;
	size_t n_files = 0;
	char lowercase_only = 0;
	int c = -1;
	size_t loop = 0;

	while((c = getopt(argc, argv, "hVf:l")) != -1)
	{
		switch(c)
		{
			case 'h':
				help();
				return 0;

			case 'V':
				version();
				return 0;

			case 'f':
				add_file(&words_files, &n_files, strdup(optarg));
				break;

			case 'l':
				lowercase_only = 1;
				break;

			default:
				help();
				return 1;
		}
	}

	if (n_files == 0)
		find_words_file(&words_files, &n_files);

	if (n_files == 0)
	{
		fprintf(stderr, "No file with words found. Please select one with the -f commandline parameter.\n");
		return 1;
	}

	srand48(get_ts() + (get_ts() * 1000000) + getpid());

	for(loop=0; loop<n_files; loop++)
		load_words_file(words_files[loop], lowercase_only, &words, &n_words);

	game_loop(words, n_words);

	return 0;
}
