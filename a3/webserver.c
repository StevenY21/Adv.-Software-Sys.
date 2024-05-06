#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


#define BUFFER_SIZE 2056
/*sending final response*/
void send_response(int new_socket, const char *response, size_t response_len) {
    ssize_t sent_bytes;
    size_t total_sent = 0;

    while (total_sent < response_len) {
        sent_bytes = send(new_socket, response + total_sent, response_len - total_sent, 0);
        if (sent_bytes == -1) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }
        total_sent += sent_bytes;
    }
}
/*for running cgi scripts, should work with any*/
void execute_cgi_script(const char *script_name, int new_socket) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE]= "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    char command[BUFFER_SIZE]= "./";
    strcat(command, script_name);
    FILE *output = popen(command, "r");
    if (output == NULL) {
        const char *response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<h1>404 Not Found</h1>";
        send_response(new_socket, response, strlen(response));
    } else {
        int count = 1;
        while (fgets(buffer, BUFFER_SIZE-1, output) != NULL) {
            strcat(response,buffer);
        }
        send_response(new_socket, response, strlen(response));
    }
    pclose(output);
}
/*for running my-histogram directory*/
void exec_histogram_script(const char *requestCmd, int new_socket) {
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    strcpy(input, requestCmd);
    char response[BUFFER_SIZE]= "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    char command[BUFFER_SIZE]= "./";

    strcat(command, "my-histogram.cgi ");
    char* token = strtok(input, "%20");
    char *arg = strtok(NULL, "%20");
    strcat(command, arg);
    FILE *output = popen(command, "r");
    if (output == NULL || strtok(NULL, "%20") == NULL) {
        const char *response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<h1>404 Not Found</h1>";
        send_response(new_socket, response, strlen(response));
    } else {
        int count = 1;
        while (fgets(buffer, BUFFER_SIZE-1, output) != NULL) {
            strcat(response, buffer);
        }
        send_response(new_socket, response, strlen(response));
    }
    pclose(output);
}
int main(int argc, char *argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    //setting up socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    int port = atoi(argv[1]);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //loop listening for requests
    while (1) {
        if (listen(server_fd, 3) < 0) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        valread = read(new_socket, buffer, BUFFER_SIZE);
        if (valread < 0) {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
        //regex for matching for request
        //currently only accepts get request
        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regmatch_t matches[2];
        // found match
        if (regexec(&regex, buffer, 2, matches, 0) == 0) {
            buffer[matches[1].rm_eo] = '\0';
            const char *file_name = buffer + matches[1].rm_so;
            int valid_type = 0;
            const char *file_type = NULL;

            if (strcmp(file_name, "favicon.ico") != 0) {
                //printf("file name: %s\n", file_name);
                if (strstr(file_name, ".html") != NULL) {
                    valid_type = 1;
                    file_type = "text/html";
                } else if (strstr(file_name, ".jpeg") != NULL || strstr(file_name, ".jpg") != NULL) {
                    valid_type = 1;
                    file_type = "image/jpeg";
                } else if (strstr(file_name, ".gif") != NULL) {
                    valid_type = 1;
                    file_type = "image/gif";
                } else if (strstr(file_name, ".cgi") != NULL) {
                    valid_type = 1;
                    file_type = ".cgi";
                } else if (strstr(file_name, "my-histogram%20") != NULL) {
                    valid_type = 1;
                    file_type = "my-histogram";
                }
                //printf("file type %s \n", file_type);
                if (valid_type) { // checking for each kind of file type in request
                    if (strcmp(file_type, ".cgi") == 0) {
                        //printf("%s \n", "exec cgi script");
                        execute_cgi_script(file_name, new_socket);
                    }else if (strcmp(file_type, "my-histogram") == 0){
                        //printf("%s \n", "exec histogram script");
                        exec_histogram_script(file_name, new_socket);
                    } else {
                        //printf("responding with file\n");
                        char header[BUFFER_SIZE];
                        snprintf(header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", file_type);
                        //printf("opening file\n");
                        int file_fd = open(file_name, O_RDONLY);
                        if (file_fd == -1) {
                            const char *response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<h1>404 Not Found</h1>";
                            send_response(new_socket, response, strlen(response));
                        } else {
                            //printf("getting fstat\n");
                            struct stat file_stat;
                            fstat(file_fd, &file_stat);
                            off_t file_size = file_stat.st_size;
                            // generates response by opening file and reading through the file data
                            char *response = (char *)malloc((BUFFER_SIZE + file_size) * sizeof(char));
                            if (response == NULL) {
                                perror("malloc failed");
                                exit(EXIT_FAILURE);
                            }
                            strcpy(response, header);
                            size_t response_len = strlen(header);
                            ssize_t bytes_read;
                            //printf("getting file content\n");
                            while ((bytes_read = read(file_fd, response + response_len, BUFFER_SIZE)) > 0) {
                                response_len += bytes_read;
                            }
                            send_response(new_socket, response, response_len);
                            free(response);
                        }
                    }
                } else {
                    const char *response = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\n\n<h1>400 Bad Request</h1>";
                    send_response(new_socket, response, strlen(response));
                }
            }
        } else {
                const char *response = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\n\n<h1>400 Bad Request</h1>";
                send_response(new_socket, response, strlen(response));
        }

        //printf("Response sent\n");
        close(new_socket);
    }

    close(server_fd);

    return 0;
}