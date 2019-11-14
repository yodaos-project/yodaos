/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <wordexp.h>
#include <getopt.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <glib.h>

#include "src/shared/io.h"
#include "src/shared/util.h"
#include "src/shared/queue.h"
#include "src/shared/shell.h"

#define CMD_LENGTH	48
#define print_text(color, fmt, args...) \
		printf(color fmt COLOR_OFF "\n", ## args)
#define print_menu(cmd, args, desc) \
		printf(COLOR_HIGHLIGHT "%s %-*s " COLOR_OFF "%s\n", \
			cmd, (int)(CMD_LENGTH - strlen(cmd)), args, desc)
#define print_submenu(cmd, desc) \
		printf(COLOR_BLUE "%s %-*s " COLOR_OFF "%s\n", \
			cmd, (int)(CMD_LENGTH - strlen(cmd)), "", desc)

static GMainLoop *main_loop;

static struct {
	struct io *input;

	bool saved_prompt;
	bt_shell_prompt_input_func saved_func;
	void *saved_user_data;

	const struct bt_shell_menu *menu;
	const struct bt_shell_menu *main;
	struct queue *submenus;
} data;

static void shell_print_menu(void);

static void cmd_version(int argc, char *argv[])
{
	bt_shell_printf("Version %s\n", VERSION);
}

static void cmd_quit(int argc, char *argv[])
{
	g_main_loop_quit(main_loop);
}

static void cmd_help(int argc, char *argv[])
{
	shell_print_menu();
}

static const struct bt_shell_menu *find_menu(const char *name)
{
	const struct queue_entry *entry;

	for (entry = queue_get_entries(data.submenus); entry;
						entry = entry->next) {
		struct bt_shell_menu *menu = entry->data;

		if (!strcmp(menu->name, name))
			return menu;
	}

	return NULL;
}

static char *menu_generator(const char *text, int state)
{
	static unsigned int index, len;
	static struct queue_entry *entry;

	if (!state) {
		index = 0;
		len = strlen(text);
		entry = (void *) queue_get_entries(data.submenus);
	}

	for (; entry; entry = entry->next) {
		struct bt_shell_menu *menu = entry->data;

		index++;

		if (!strncmp(menu->name, text, len)) {
			entry = entry->next;
			return strdup(menu->name);
		}
	}

	return NULL;
}

static void cmd_menu(int argc, char *argv[])
{
	const struct bt_shell_menu *menu;

	if (argc < 2 || !strlen(argv[1])) {
		bt_shell_printf("Missing name argument\n");
		return;
	}

	menu = find_menu(argv[1]);
	if (!menu) {
		bt_shell_printf("Unable find menu with name: %s\n", argv[1]);
		return;
	}

	bt_shell_set_menu(menu);

	shell_print_menu();
}

static bool cmd_menu_exists(const struct bt_shell_menu *menu)
{
	/* Skip menu command if not on main menu or if there are no
	 * submenus.
	 */
	if (menu != data.main || queue_isempty(data.submenus))
		return false;

	return true;
}

static void cmd_back(int argc, char *argv[])
{
	if (data.menu == data.main) {
		bt_shell_printf("Already on main menu\n");
		return;
	}

	bt_shell_set_menu(data.main);

	shell_print_menu();
}

static bool cmd_back_exists(const struct bt_shell_menu *menu)
{
	/* Skip back command if on main menu */
	if (menu == data.main)
		return false;

	return true;
}

static const struct bt_shell_menu_entry default_menu[] = {
	{ "back",         NULL,       cmd_back, "Return to main menu", NULL,
							NULL, cmd_back_exists },
	{ "menu",         "<name>",   cmd_menu, "Select submenu",
							menu_generator, NULL,
							cmd_menu_exists},
	{ "version",      NULL,       cmd_version, "Display version" },
	{ "quit",         NULL,       cmd_quit, "Quit program" },
	{ "exit",         NULL,       cmd_quit, "Quit program" },
	{ "help",         NULL,       cmd_help,
					"Display help about this program" },
	{ }
};

static void shell_print_menu(void)
{
	const struct bt_shell_menu_entry *entry;
	const struct queue_entry *submenu;

	if (!data.menu)
		return;

	print_text(COLOR_HIGHLIGHT, "Menu %s:", data.menu->name);
	print_text(COLOR_HIGHLIGHT, "Available commands:");
	print_text(COLOR_HIGHLIGHT, "-------------------");

	if (data.menu == data.main) {
		for (submenu = queue_get_entries(data.submenus); submenu;
						submenu = submenu->next) {
			struct bt_shell_menu *menu = submenu->data;

			print_submenu(menu->name, menu->desc ? menu->desc :
								"Submenu");
		}
	}

	for (entry = data.menu->entries; entry->cmd; entry++) {
		print_menu(entry->cmd, entry->arg ? : "", entry->desc ? : "");
	}

	for (entry = default_menu; entry->cmd; entry++) {
		if (entry->exists && !entry->exists(data.menu))
			continue;

		print_menu(entry->cmd, entry->arg ? : "", entry->desc ? : "");
	}
}

static int parse_args(char *arg, wordexp_t *w, char *del, int flags)
{
	char *str;

	str = g_strdelimit(arg, del, '"');

	if (wordexp(str, w, flags)) {
		g_free(str);
		return -EINVAL;
	}

	/* If argument ends with ,,, set we_offs bypass strict checks */
	if (w->we_wordc && g_str_has_suffix(w->we_wordv[w->we_wordc -1], "..."))
		w->we_offs = 1;

	g_free(str);

	return 0;
}

static int cmd_exec(const struct bt_shell_menu_entry *entry,
					int argc, char *argv[])
{
	wordexp_t w;
	size_t len;
	char *man, *opt;
	int flags = WRDE_NOCMD;

	if (!entry->arg || entry->arg[0] == '\0') {
		if (argc > 1) {
			print_text(COLOR_HIGHLIGHT, "Too many arguments");
			return -EINVAL;
		}
		goto exec;
	}

	/* Find last mandatory arguments */
	man = strrchr(entry->arg, '>');
	if (!man) {
		opt = strdup(entry->arg);
		goto optional;
	}

	len = man - entry->arg;
	man = strndup(entry->arg, len + 1);

	if (parse_args(man, &w, "<>", flags) < 0) {
		print_text(COLOR_HIGHLIGHT,
				"Unable to parse mandatory command arguments");
		return -EINVAL;
	}

	/* Check if there are enough arguments */
	if ((unsigned) argc - 1 < w.we_wordc && !w.we_offs) {
		print_text(COLOR_HIGHLIGHT, "Missing %s argument",
						w.we_wordv[argc - 1]);
		goto fail;
	}

	flags |= WRDE_APPEND;
	opt = strdup(entry->arg + len + 1);

optional:
	if (parse_args(opt, &w, "[]", flags) < 0) {
		print_text(COLOR_HIGHLIGHT,
				"Unable to parse optional command arguments");
		return -EINVAL;
	}

	/* Check if there are too many arguments */
	if ((unsigned) argc - 1 > w.we_wordc && !w.we_offs) {
		print_text(COLOR_HIGHLIGHT, "Too many arguments: %d > %zu",
					argc - 1, w.we_wordc);
		goto fail;
	}

	wordfree(&w);

exec:
	if (entry->func)
		entry->func(argc, argv);

	return 0;

fail:
	wordfree(&w);
	return -EINVAL;
}

static int menu_exec(const struct bt_shell_menu_entry *entry,
					int argc, char *argv[])
{
	for (; entry->cmd; entry++) {
		if (strcmp(argv[0], entry->cmd))
			continue;

		/* Skip menu command if not on main menu */
		if (data.menu != data.main && !strcmp(entry->cmd, "menu"))
			continue;

		/* Skip back command if on main menu */
		if (data.menu == data.main && !strcmp(entry->cmd, "back"))
			continue;

		return cmd_exec(entry, argc, argv);
	}

	return -ENOENT;
}

static void shell_exec(int argc, char *argv[])
{
	if (!data.menu || !argv[0])
		return;

	if (menu_exec(default_menu, argc, argv) == -ENOENT) {
		if (menu_exec(data.menu->entries, argc, argv) == -ENOENT)
			print_text(COLOR_HIGHLIGHT, "Invalid command");
	}
}

void bt_shell_printf(const char *fmt, ...)
{
	va_list args;
	bool save_input;
	char *saved_line;
	int saved_point;

	save_input = !RL_ISSTATE(RL_STATE_DONE);

	if (save_input) {
		saved_point = rl_point;
		saved_line = rl_copy_text(0, rl_end);
		if (!data.saved_prompt) {
			rl_save_prompt();
			rl_replace_line("", 0);
			rl_redisplay();
		}
	}

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	if (save_input) {
		if (!data.saved_prompt)
			rl_restore_prompt();
		rl_replace_line(saved_line, 0);
		rl_point = saved_point;
		rl_forced_update_display();
		free(saved_line);
	}
}

void bt_shell_hexdump(const unsigned char *buf, size_t len)
{
	static const char hexdigits[] = "0123456789abcdef";
	char str[68];
	size_t i;

	if (!len)
		return;

	str[0] = ' ';

	for (i = 0; i < len; i++) {
		str[((i % 16) * 3) + 1] = ' ';
		str[((i % 16) * 3) + 2] = hexdigits[buf[i] >> 4];
		str[((i % 16) * 3) + 3] = hexdigits[buf[i] & 0xf];
		str[(i % 16) + 51] = isprint(buf[i]) ? buf[i] : '.';

		if ((i + 1) % 16 == 0) {
			str[49] = ' ';
			str[50] = ' ';
			str[67] = '\0';
			bt_shell_printf("%s\n", str);
			str[0] = ' ';
		}
	}

	if (i % 16 > 0) {
		size_t j;
		for (j = (i % 16); j < 16; j++) {
			str[(j * 3) + 1] = ' ';
			str[(j * 3) + 2] = ' ';
			str[(j * 3) + 3] = ' ';
			str[j + 51] = ' ';
		}
		str[49] = ' ';
		str[50] = ' ';
		str[67] = '\0';
		bt_shell_printf("%s\n", str);
	}
}

void bt_shell_prompt_input(const char *label, const char *msg,
			bt_shell_prompt_input_func func, void *user_data)
{
	/* Normal use should not prompt for user input to the value a second
	 * time before it releases the prompt, but we take a safe action. */
	if (data.saved_prompt)
		return;

	rl_save_prompt();
	rl_message(COLOR_RED "[%s]" COLOR_OFF " %s ", label, msg);

	data.saved_prompt = true;
	data.saved_func = func;
	data.saved_user_data = user_data;
}

int bt_shell_release_prompt(const char *input)
{
	bt_shell_prompt_input_func func;
	void *user_data;

	if (!data.saved_prompt)
		return -1;

	data.saved_prompt = false;

	rl_restore_prompt();

	func = data.saved_func;
	user_data = data.saved_user_data;

	data.saved_func = NULL;
	data.saved_user_data = NULL;

	func(input, user_data);

	return 0;
}

static void rl_handler(char *input)
{
	wordexp_t w;

	if (!input) {
		rl_insert_text("quit");
		rl_redisplay();
		rl_crlf();
		g_main_loop_quit(main_loop);
		return;
	}

	if (!strlen(input))
		goto done;

	if (!bt_shell_release_prompt(input))
		goto done;

	if (history_search(input, -1))
		add_history(input);

	if (wordexp(input, &w, WRDE_NOCMD))
		goto done;

	if (w.we_wordc == 0) {
		wordfree(&w);
		goto done;
	}

	shell_exec(w.we_wordc, w.we_wordv);
	wordfree(&w);
done:
	free(input);
}

static char *find_cmd(const char *text,
			const struct bt_shell_menu_entry *entry, int *index)
{
	const struct bt_shell_menu_entry *tmp;
	int len;

	len = strlen(text);

	while ((tmp = &entry[*index])) {
		(*index)++;

		if (!tmp->cmd)
			break;

		if (tmp->exists && !tmp->exists(data.menu))
			continue;

		if (!strncmp(tmp->cmd, text, len))
			return strdup(tmp->cmd);
	}

	return NULL;
}

static char *cmd_generator(const char *text, int state)
{
	static int index;
	static bool default_menu_enabled;
	char *cmd;

	if (!state) {
		index = 0;
		default_menu_enabled = true;
	}

	if (default_menu_enabled) {
		cmd = find_cmd(text, default_menu, &index);
		if (cmd) {
			return cmd;
		} else {
			index = 0;
			default_menu_enabled = false;
		}
	}

	return find_cmd(text, data.menu->entries, &index);
}

static char **menu_completion(const struct bt_shell_menu_entry *entry,
				const char *text, char *input_cmd)
{
	char **matches = NULL;

	for (; entry->cmd; entry++) {
		if (strcmp(entry->cmd, input_cmd))
			continue;

		if (!entry->gen) {
			if (text[0] == '\0')
				bt_shell_printf("Usage: %s %s\n", entry->cmd,
						entry->arg ? entry->arg : "");
			break;
		}

		rl_completion_display_matches_hook = entry->disp;
		matches = rl_completion_matches(text, entry->gen);
		break;
	}

	return matches;
}

static char **shell_completion(const char *text, int start, int end)
{
	char **matches = NULL;

	if (!data.menu)
		return NULL;

	if (start > 0) {
		wordexp_t w;

		if (wordexp(rl_line_buffer, &w, WRDE_NOCMD))
			return NULL;

		matches = menu_completion(default_menu, text, w.we_wordv[0]);
		if (!matches)
			matches = menu_completion(data.menu->entries, text,
							w.we_wordv[0]);

		wordfree(&w);
	} else {
		rl_completion_display_matches_hook = NULL;
		matches = rl_completion_matches(text, cmd_generator);
	}

	if (!matches)
		rl_attempted_completion_over = 1;

	return matches;
}

static bool io_hup(struct io *io, void *user_data)
{
	g_main_loop_quit(main_loop);

	return false;
}

static bool signal_read(struct io *io, void *user_data)
{
	static bool terminated = false;
	struct signalfd_siginfo si;
	ssize_t result;
	int fd;

	fd = io_get_fd(io);

	result = read(fd, &si, sizeof(si));
	if (result != sizeof(si))
		return false;

	switch (si.ssi_signo) {
	case SIGINT:
		if (data.input) {
			rl_replace_line("", 0);
			rl_crlf();
			rl_on_new_line();
			rl_redisplay();
			break;
		}

		/*
		 * If input was not yet setup up that means signal was received
		 * while daemon was not yet running. Since user is not able
		 * to terminate client by CTRL-D or typing exit treat this as
		 * exit and fall through.
		 */

		/* fall through */
	case SIGTERM:
		if (!terminated) {
			rl_replace_line("", 0);
			rl_crlf();
			g_main_loop_quit(main_loop);
		}

		terminated = true;
		break;
	}

	return false;
}

static struct io *setup_signalfd(void)
{
	struct io *io;
	sigset_t mask;
	int fd;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		perror("Failed to set signal mask");
		return 0;
	}

	fd = signalfd(-1, &mask, 0);
	if (fd < 0) {
		perror("Failed to create signal descriptor");
		return 0;
	}

	io = io_new(fd);

	io_set_close_on_destroy(io, true);
	io_set_read_handler(io, signal_read, NULL, NULL);
	io_set_disconnect_handler(io, io_hup, NULL, NULL);

	return io;
}

static void rl_init(void)
{
	setlinebuf(stdout);
	rl_attempted_completion_function = shell_completion;

	rl_erase_empty_line = 1;
	rl_callback_handler_install(NULL, rl_handler);
}

static const struct option main_options[] = {
	{ "version",	no_argument, 0, 'v' },
	{ "help",	no_argument, 0, 'h' },
};

static void usage(int argc, char **argv, const struct bt_shell_opt *opt)
{
	unsigned int i;

	printf("%s ver %s\n", argv[0], VERSION);
	printf("Usage:\n"
		"\t%s [options]\n", argv[0]);

	printf("Options:\n");

	for (i = 0; opt && opt->options[i].name; i++)
		printf("\t--%s \t%s\n", opt->options[i].name, opt->help[i]);

	printf("\t--version \tDisplay version\n"
		"\t--help \t\tDisplay help\n");
}

void bt_shell_init(int argc, char **argv, const struct bt_shell_opt *opt)
{
	int c, index = 0;
	struct option options[256];
	char optstr[256];
	size_t offset;

	offset = sizeof(main_options) / sizeof(struct option);

	memcpy(options, main_options, sizeof(struct option) * offset);

	if (opt) {
		memcpy(options + offset, opt->options,
				sizeof(struct option) * opt->optno);
		snprintf(optstr, sizeof(optstr), "+hv%s", opt->optstr);
	} else
		snprintf(optstr, sizeof(optstr), "+hv");

	while ((c = getopt_long(argc, argv, optstr, options, &index)) != -1) {
		switch (c) {
		case 'v':
			printf("%s: %s\n", argv[0], VERSION);
			exit(EXIT_SUCCESS);
			return;
		case 'h':
			usage(argc, argv, opt);
			exit(EXIT_SUCCESS);
			return;
		default:
			if (c != opt->options[index - offset].val) {
				usage(argc, argv, opt);
				exit(EXIT_SUCCESS);
				return;
			}

			*opt->optarg[index - offset] = optarg;
		}
	}

	main_loop = g_main_loop_new(NULL, FALSE);

	rl_init();
}

static void rl_cleanup(void)
{
	rl_message("");
	rl_callback_handler_remove();
}

void bt_shell_run(void)
{
	struct io *signal;

	signal = setup_signalfd();

	g_main_loop_run(main_loop);

	bt_shell_release_prompt("");
	bt_shell_detach();

	io_destroy(signal);

	g_main_loop_unref(main_loop);
	main_loop = NULL;

	rl_cleanup();
}

bool bt_shell_set_menu(const struct bt_shell_menu *menu)
{
	if (!menu)
		return false;

	data.menu = menu;

	if (!data.main)
		data.main = menu;

	return true;
}

bool bt_shell_add_submenu(const struct bt_shell_menu *menu)
{
	if (!menu)
		return false;

	if (!data.submenus)
		data.submenus = queue_new();

	queue_push_tail(data.submenus, (void *) menu);

	return true;
}

void bt_shell_set_prompt(const char *string)
{
	if (!main_loop)
		return;

	rl_set_prompt(string);
	printf("\r");
	rl_on_new_line();
	rl_redisplay();
}

static bool input_read(struct io *io, void *user_data)
{
	rl_callback_read_char();
	return true;
}

bool bt_shell_attach(int fd)
{
	struct io *io;

	/* TODO: Allow more than one input? */
	if (data.input)
		return false;

	io = io_new(fd);

	io_set_read_handler(io, input_read, NULL, NULL);
	io_set_disconnect_handler(io, io_hup, NULL, NULL);

	data.input = io;

	return true;
}

bool bt_shell_detach(void)
{
	if (!data.input)
		return false;

	io_destroy(data.input);
	data.input = NULL;

	return true;
}
