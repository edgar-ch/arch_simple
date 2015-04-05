#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <utime.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <ctype.h>
#include <fcntl.h>
#include "arch.h"

int main(int argc, char **argv)
{
	int c;
	char *name, *arch_name;
	int mod = -1, arch_name_taken = 0;

	opterr = 0;
	while ((c = getopt(argc, argv, "i:f:e:")) != -1) {
		switch (c) {
		case 'i':
			name = optarg;
			mod = CREATE;
			break;
		case 'e':
			name = optarg;
			mod = EXTRACT;
			break;
		case 'f':
			arch_name = optarg;
			arch_name_taken = 1;
			break;
		case '?':
			if (optopt == 'i' || optopt == 'e')
				fprintf(
					stderr,
					"Option -%c requires an argument.\n",
					optopt);
			else if (isprint (optopt))
				fprintf(
					stderr,
					"Unknown option `-%c'.\n",
					optopt);
			else
				fprintf(
					stderr,
					"Unknown option character `\\x%x'.\n",
					optopt);
			return 1;
		default:
			abort();
		}
	}
	if (!arch_name_taken) {
		arch_name = (char *) malloc(
			sizeof(char) * strlen("ururu.arch") + 1);
		strcpy(arch_name, "ururu.arch");
	}
	if (mod == CREATE)
		create_arch(arch_name, name);
	if (mod == EXTRACT)
		extract_arch(name);
	if (!arch_name_taken)
		free(arch_name);
	return 0;
}

void create_arch(char *arch_name, char *name)
{
	int arch_fd, len;
	struct stat fstat;
	char *s = NULL;
	struct arch_header *head;

	arch_fd = creat(arch_name, 0777);
	if (arch_fd == -1) {
		perror("create_arch");
		return;
	}

	s = (char *) malloc(sizeof(char) * PATH_MAX);

	stat(name, &fstat);
	if (S_ISDIR(fstat.st_mode)) {
		len = strlen(name);
		strcpy(s, name);
		if (s[len-1] != '/')
			strcat(s, "/");
		add_to_arch(s, arch_fd);
	} else
		write_file_to_arch(name, arch_fd);

	head = (struct arch_header *) calloc(1, sizeof(struct arch_header));
	len = write(arch_fd, head, sizeof(struct arch_header));
	if (len == -1) {
		perror("arch_fd");
		free(s);
		free(head);
		return;
	}
	close(arch_fd);
	free(s);
	free(head);
}

void add_to_arch(char *path, int arch_fd)
{
	DIR *dir;
	struct dirent *dp;
	char path_next[PATH_MAX], file_name[PATH_MAX];

	write_dir_header(path, arch_fd);
	dir = opendir(path);
	if (dir == NULL) {

		perror("Couldn't open");
		return;
	}
	do {
		errno = 0;
		dp = readdir(dir);
		if (dp != NULL) {
			if (strcmp(dp->d_name, ".") == 0 ||
				strcmp(dp->d_name, "..") == 0)
				continue;
			if (dp->d_type == DT_DIR) {
				snprintf(path_next,
					sizeof(path_next),
					"%s%s/",
					path,
					dp->d_name);
				add_to_arch(path_next, arch_fd);
			} else {
				snprintf(file_name,
					sizeof(file_name),
					"%s%s",
					path,
					dp->d_name);
				write_file_to_arch(file_name, arch_fd);
			}
		}
	} while (dp != NULL);

	if (errno != 0) {
		perror("error reading directory");
		return;
	}

	closedir(dir);
}

void write_dir_header(char *path, int arch_fd)
{
	struct arch_header *head;
	int len;
	struct stat fstat;

	head = (struct arch_header *) calloc(
		1, sizeof(struct arch_header));
	strcpy(head->name, path);
	head->type = DIR_T;
	stat(path, &fstat);
	head->ctime = fstat.st_mtime;
	len = write(arch_fd, head, sizeof(struct arch_header));
	if (len == -1)
		perror("write_dir_header");
	free(head);
}

void write_file_to_arch(char *path, int arch_fd)
{
	void *buf;
	struct arch_header *head;
	struct stat fstat;
	ssize_t len, len_read;
	int fd;

	head = (struct arch_header *) calloc(1, sizeof(struct arch_header));

	stat(path, &fstat);
	strcpy(head->name, path);
	head->type = FILE_T;
	head->size = fstat.st_size;
	head->ctime = fstat.st_mtime;
	len = write(arch_fd, head, sizeof(struct arch_header));
	if (len == -1) {
		perror("arch_fd");
		free(head);
		return;
	}
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("fd");
		free(head);
		return;
	}

	buf = malloc(BUF_SIZE);
	errno = 0;
	while ((len_read = read(fd, buf, BUF_SIZE)) > 0) {
		len = write(arch_fd, buf, len_read);
		if (len_read != len) {
			printf("Error with write to dest !\n");
			free(head);
			free(buf);
			return;
		}
	}

	free(head);
	free(buf);
}
void extract_arch(char *name)
{
	int arch_fd;
	int len_read;
	struct arch_header *head;

	arch_fd = open(name, O_RDONLY);
	if (arch_fd == -1) {
		perror("arch_fd");
		return;
	}

	head = (struct arch_header *) malloc(sizeof(struct arch_header));
	while ((len_read = read(arch_fd,
		head, sizeof(struct arch_header))) > 0) {
		if (len_read <= 0) {
			perror("read");
			return;
		}
		if ((strlen(head->name) == 0) && (head->size == 0))
			break;
		extract_file_from_arch(head, arch_fd);
	}
	free(head);
}

void extract_file_from_arch(struct arch_header *entry, int arch_fd)
{
	int len_read, len, file_len;
	void *buf;
	int fd;
	struct utimbuf times;

	if (entry->type == FILE_T) {
		fd = creat(entry->name, 0666);
		if (fd == -1) {
			perror("fd");
			return;
		}
		buf = malloc(BUF_SIZE);
		errno = 0;
		file_len = 0;
		while (1) {
			if ((file_len < entry->size) &&
					(entry->size-file_len > BUF_SIZE)) {
				len_read = read(arch_fd, buf, BUF_SIZE);
				file_len += BUF_SIZE;
			} else if ((file_len < entry->size) &&
					(entry->size-file_len < BUF_SIZE)) {
				len_read = read(arch_fd,
					buf, entry->size-file_len);
				file_len += len_read;
			} else if (file_len == entry->size)
				break;

			len = write(fd, buf, len_read);
			if (len_read != len) {
				printf("Error with write to dest !\n");
				free(buf);
				return;
			}
		}
		times.actime = time(NULL);
		times.modtime = entry->ctime;
		utime(entry->name, &times);
		free(buf);
		close(fd);
	} else if (entry->type == DIR_T) {
		mkdir(entry->name, 0666);
		times.actime = time(NULL);
		times.modtime = entry->ctime;
		utime(entry->name, &times);
	}
}
