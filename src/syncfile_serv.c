#include "../include//helper.h"
#include "../include/llist.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define TIMEO_SEC 3

const size_t fdata_size = sizeof(struct filedata);

struct node* client_files_m; /* client's files' metadata*/
struct node* server_files_m; /* our files' metadata*/
struct node* client_requires_m; /* metadata of files that client needs*/
struct node* server_requires_m; /* metadata of files that server needs*/
struct node* result; /* to hold result from function*/

int logfd; /* file descriptor of log file*/
char peer_ipv4[MAX_IPV4_LEN]; /* string to hold peer ipv4 addr, max can be
                               xxx.xxx.xxx.xxx*/
size_t client_files_mc; /* client files metadata count*/
size_t client_requires_mc; /* client requires metadata count. number of
                            files whose metadata client needs*/
size_t server_files_mc; /*Number of files server has*/
size_t server_requires_mc; /*Number of files server needs*/

static struct node* pr_list(struct node* head)
{
    struct node* traverse = head;
    int count = 0;
    while (traverse->next) {

        flog("%s\n", traverse->data.filename);
        traverse = traverse->next;
        count++;
    }
    return head;
}

int main(int argc, char* argv[])
{
    // chdir("/home/agastya/Pictures/Wallpapers");
    chdir("/home/agastya/Pictures/Art/");
    set_logfile(LOGFILE);

    // Setting timeout for send and recv
    struct timeval timeout = { .tv_sec = TIMEO_SEC, .tv_usec = 0 };
    if (setsockopt(0, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
        flog("(ERROR) setsockopt failed\n");
    if (setsockopt(1, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0)
        flog("(ERROR) setsockopt failed\n");

    struct sockaddr_in peer_addr;
    socklen_t peer_addrlen = sizeof(peer_addr);

    if (getpeername(0, (struct sockaddr*)&peer_addr, &peer_addrlen) == -1)
        strncpy(peer_ipv4, "Unkown Client", MAX_IPV4_LEN);
    else
        strncpy(peer_ipv4, inet_ntoa(peer_addr.sin_addr), MAX_IPV4_LEN);

    flog("(STATUS) Connected to %s\n", peer_ipv4);

    recv_header_len(0, &client_files_mc);
    flog("(STATUS) %s will send %d headers\n\n", peer_ipv4, client_files_mc);

    // received headers of client's files
    result = recv_headers(0, &client_files_m, client_files_mc);
    if (result) {
        flog("(ERROR) timeout/error while receiving file headers of %s\n",
            result->data.filename);
        goto finish;
    }

    client_requires_mc = compute_differences(
        ".", client_files_m,
        &client_requires_m); // client requires metadata. metadata of files to
                             // send. function prints whats common and whats
                             // not stores length of metadata in
                             // client_requires_mc
    // tell client how many files it needs and sending
    flog("(STATUS) Client needs %ld files\n\n", client_requires_mc);
    send_header_len(
        1, &client_requires_mc); // sending the number of files we plan to send

    // send the files the client needs
    result = send_headers_and_files(1, client_requires_m);
    if (result) {
        flog("(ERROR) Error in sending %s\n", result->data.filename);
        goto finish;
    }
    // not needed anymore
    free_list(&client_files_m);
    free_list(&client_requires_m);

    // the client now has everything that they need, server needs clients file
    server_files_mc = fill_list(&server_files_m, ".");
    flog("(HEADER) Sending headers of %ld files\n\n", server_files_mc);
    send_header_len(1, &server_files_mc);
    send_headers(1, server_files_m);

    // Client now sends headers and files that server needs
    recv_header_len(0, &server_requires_mc);
    flog("(STATUS) Client sending %d files\n", server_requires_mc);
    result = recv_headers_and_files(0, server_requires_mc, &server_requires_m);
    if (result) {
        flog("(ERROR) Error in receiving %s\n", result->data.filename);
        goto finish;
    }

finish:
    if (errno)
        flog("(ERRINFO) %s\n", strerror(errno));

    cleanup();
    dprintf(logfd, "\n");
    flog("(STATUS) Finished dealing with %s\n\n\n", peer_ipv4);
    close(logfd);
}
