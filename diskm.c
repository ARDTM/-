
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

void print_help(void);
void print_bdev_list(void);
void dev_mon(char *dev);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("bad usage\n");
        return 1;
    }
    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }
    if (strcmp(argv[1], "list") == 0) {
        print_bdev_list();
        return 0;
    }
    dev_mon(argv[1]);
    return 0;
}

void print_help(void) {
    printf("diskm op\nhelp\t\tprint this\nlist\t\tprint list of block devs\n<device>\tmonitoring device\n");
}

void print_bdev_list(void) {
    DIR *dir = opendir("/sys/block");
    struct dirent *de;
    if (!dir) {
        perror("opendir");
        return;
    }
    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
            printf("%s\n", de->d_name);
        }
    }
    closedir(dir);
}

void dev_mon(char *dev) {
    initscr();
    cbreak();
    noecho();

    char path[64];
    sprintf(path, "/tmp/ddaemon/%s_info", dev);

    unsigned long long old_read_sectors = 0, old_read_ms = 0;
    unsigned long long old_write_sectors = 0, old_write_ms = 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        endwin();
        perror("open info file");
        return;
    }
    char bbuf[512] = {0};
    read(fd, bbuf, sizeof(bbuf) - 1);
    close(fd);

    char *str;

    str = strstr(bbuf, "6:");
    if (str) old_read_sectors = strtoull(str + 3, NULL, 10);

    str = strstr(bbuf, "7:");
    if (str) old_read_ms = strtoull(str + 3, NULL, 10);

    str = strstr(bbuf, "8:");
    if (str) old_write_sectors = strtoull(str + 3, NULL, 10);

    str = strstr(bbuf, "9:");
    if (str) old_write_ms = strtoull(str + 3, NULL, 10);

    unsigned long long current_read_sectors = 0;
    unsigned long long current_write_sectors = 0;
    while (1) {
        clear();

        fd = open(path, O_RDONLY);
        if (fd < 0) {
            endwin();
            perror("open info file");
            return;
        }
        char buf[512] = {0};
        read(fd, buf, sizeof(buf) - 1);
        close(fd);

        str = strstr(buf, "1:");
        if (!str) {
            printw("model: unknown\n");
        } else {
            str += 3;
            printw("model:\t");
            while (*str != '\n' && *str != '\0') {
                addch(*str++);
            }
            addch('\n');
        }

        str = strstr(buf, "2:");
        if (!str) {
            printw("vendor: unknown\n");
        } else {
            str += 3;
            printw("vendor:\t");
            while (*str != '\n' && *str != '\0') {
                addch(*str++);
            }
            addch('\n');
        }

        unsigned long long blocks = 0, bsize = 0;
        str = strstr(buf, "3:");
        if (str) blocks = strtoull(str + 3, NULL, 10);

        str = strstr(buf, "4:");
        if (str) bsize = strtoull(str + 3, NULL, 10);

        printw("bsize:\t%llu bytes\n", bsize);
        printw("size:\t%llu MB\n", (blocks * bsize) >> 20);

        str = strstr(buf, "6:");
        if (str) current_read_sectors = strtoull(str + 3, NULL, 10);

        str = strstr(buf, "8:");
        if (str) current_write_sectors = strtoull(str + 3, NULL, 10);

        printw("read:\t%llu MB\n", (current_read_sectors * bsize) >> 20);

        unsigned long long delta_read_bytes = (current_read_sectors - old_read_sectors) * bsize;
        unsigned long long avg_read = delta_read_bytes / 3;
        printw("avg read:\t%llu MB/sec\n", avg_read >> 20);
        printw("write:\t%llu MB\n", (current_write_sectors * bsize) >> 20);

        unsigned long long delta_write_bytes = (current_write_sectors - old_write_sectors) * bsize;
        unsigned long long avg_write = delta_write_bytes / 3;
        printw("avg write:\t%llu MB/sec\n", avg_write >> 20);

        old_read_sectors = current_read_sectors;
        old_write_sectors = current_write_sectors;

        refresh();
        sleep(3);
    }

    getch();
    endwin();
}
