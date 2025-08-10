#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

void stop(int signo){
	unlink("/var/run/disk_daemon.pid");
	exit(0);
}

void make_pid(){
	int fd = open("/var/run/disk_daemon.pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	char pid[8] = {0};
	snprintf(pid, sizeof(pid), "%d", getpid());
	write(fd, pid, sizeof(pid));
	close(fd);
}

void daemonize(void){
	mkdir("/tmp/ddaemon", 0755);  // или 0700

	pid_t pid, sid;

	pid = fork();
	if(pid<0)  
		exit(EXIT_FAILURE);
	if(pid>0)
		exit(EXIT_SUCCESS);

	umask(0);

	sid = setsid();
	if(sid < 0)
		exit(EXIT_FAILURE);
	if((chdir("/")) < 0)
		exit(EXIT_FAILURE);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

}

void parse_model(int dest_fd, int src_fd);
void parse_vendor(int dest_fd, int src_fd);
void parse_size(int dest_fd, int src_fd);
void parse_stat(int dest_fd, int src_fd);
void parse_removable(int dest_fd, int src_fd);
void parse_lb_size(int dest_fd, int src_fd);



void parse_sys(char *name){
	int dest_fd;

	char path[64];
	snprintf(path, sizeof(path), "/sys/block/%s/", name);

	char dest_file[32];
	snprintf(dest_file, sizeof(dest_file), "/tmp/ddaemon/%s_info", name);
	dest_fd = open(dest_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);	

	chdir(path);
	
	int stat_fd = open("stat", O_RDONLY );
	int size_fd = open("size", O_RDONLY);
	int lb_size_fd = open("queue/logical_block_size", O_RDONLY);
	int removable_fd = open("removable", O_RDONLY);

	strcat(path, "device/");
	chdir(path);
	
	int model_fd = open("model", O_RDONLY);
	int vendor_fd = open("vendor", O_RDONLY);

	parse_model(dest_fd, model_fd);	
	parse_vendor(dest_fd, vendor_fd);
	parse_size(dest_fd, size_fd);
	parse_lb_size(dest_fd, lb_size_fd);
	parse_removable(dest_fd, removable_fd);
	parse_stat(dest_fd, stat_fd);

	write(dest_fd, "\n", 2);

	close(dest_fd);
	close(stat_fd);
	close(size_fd);
	close(lb_size_fd);
	close(removable_fd);
	close(model_fd);	
	close(vendor_fd);
}

int main(void){
	daemonize();
	
	make_pid();

	signal(SIGTERM, stop);
	signal(SIGINT, stop);

	DIR *dir;
	struct dirent *direntry;
	while(1){
		dir = opendir("/sys/block");
		if(dir){
			while((direntry = readdir(dir)) != NULL){
				if(  strcmp(direntry->d_name, ".") && strcmp(direntry->d_name, "..")){
					parse_sys(direntry->d_name);
				}
			}
			closedir(dir);
		}
	sleep(3);
	}
}

void parse_stat(int dest_fd, int src_fd){\

	char buf[128] = {0};
	read(src_fd, buf, sizeof(buf)-1);

	unsigned long long stats[17] = {0};
	sscanf(buf, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &stats[0], &stats[1], &stats[2], &stats[3], &stats[4], &stats[5], &stats[6], &stats[7], &stats[8], &stats[9], &stats[10], &stats[11], &stats[12], &stats[13], &stats[14], &stats[15], &stats[16]);
	
	int len = snprintf(buf, sizeof(buf), "6: %llu\n7: %llu\n8: %llu\n9: %llu\n", stats[2], stats[3], stats[6], stats[7]);

	write(dest_fd, buf, len);
}

void parse_model(int dest_fd, int src_fd){
	char buf[32] = {0};
	read(src_fd, buf, sizeof(buf)-1);
	write(dest_fd, "1: ", 3);
	write(dest_fd, buf, strlen(buf));
}

void parse_vendor(int dest_fd, int src_fd){
        char buf[32] = {0};
        read(src_fd, buf, sizeof(buf)-1);

	write(dest_fd, "2: ", 3);
        write(dest_fd, buf, strlen(buf));
}

void parse_size(int dest_fd, int src_fd){
	char buf[16] = {0};
	read(src_fd, buf, sizeof(buf)-1);
	
	write(dest_fd, "3: ", 3);
	write(dest_fd, buf, strlen(buf));
}

void parse_lb_size(int dest_fd, int src_fd){
        char buf[8] = {0};
        read(src_fd, buf, sizeof(buf)-1);

	write(dest_fd, "4: ", 3);
        write(dest_fd, buf, strlen(buf));
}

void parse_removable(int dest_fd, int src_fd){
        char buf[16] = {0};
        read(src_fd, buf, sizeof(buf)-1);

	write(dest_fd, "5: ", 3);
        write(dest_fd, buf, strlen(buf));
}


