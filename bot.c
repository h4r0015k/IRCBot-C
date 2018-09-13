#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

struct server_details {
    char server[80];
    char port[10];
    char pass[30];
    char nick[30];
};

struct server_details *load_config();
void pull_config(FILE*, struct server_details*);
void ident(char*, char*, int);
void send_pong(char*, int);
void process_priv_msg(char*, char*, int);
void get_nick(char*, char*);
void get_msg(char*, char*);
void send_msg(char*, char*, int);

int main(int argc, char *argv[])
{
    int status, sock, c;
    struct addrinfo hints;
    struct addrinfo *res, *r;
    struct server_details *ser;
    char data[1000];
    char *tmp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    ser = load_config();
    
    if((status = getaddrinfo(ser->server, ser->port, &hints, &res)) < 0) {
	fprintf(stderr,"Error: %s", gai_strerror(status));
	exit(EXIT_FAILURE);
    }

    for(r = res; r != NULL; r = r->ai_next) {
	
	if((sock = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) < 0)
	    continue;

	if((c = connect(sock, r->ai_addr, r->ai_addrlen)) < 0)
	    continue;
    }

    if(c < 0) {
	if(sock >= 0)
	    close(sock);

	perror("Connect");
	exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    ident(ser->nick, ser->pass, sock);
    free(ser);

    while(recv(sock, data, sizeof(data), 0) > 0) {
	printf("%s\n",data);

	if(strncmp(data,"PING",4) == 0)
	    send_pong(data,sock);
	else if((tmp = strstr(data, "PRIVMSG")) != NULL && *(tmp + 7) == ' ') 
	    process_priv_msg((tmp + 7), data, sock);

	memset(data, '\0', sizeof(data));  
    }

    close(sock);
    exit(EXIT_SUCCESS);
}

struct server_details *load_config()
{
    struct server_details *ser = malloc(sizeof(struct server_details));
    FILE *f;

    if((f = fopen("config.txt","r")) == NULL) {
	perror("Config File");
	exit(EXIT_FAILURE);
    }

    pull_config(f, ser);
    return ser;
}

void pull_config(FILE *f, struct server_details *ser) 
{
    char data[50];
    char key[10];
    char value[10];

    while(fgets(data, sizeof(data), f) != NULL) {
	sscanf(data,"%s = %s",key,value);
	
	if(!strcmp(key,"server"))
	    strcpy(ser->server, value);
	else if(!strcmp(key,"port"))
	    strcpy(ser->port, value);
	else if(!strcmp(key, "nick"))
	    strcpy(ser->nick, value);
	else if(!strcmp(key, "pass"))
	    strcpy(ser->pass, value);
    }
}

void ident(char *nick, char *pass, int s) 
{    
    char user[50];
    char nk[50];
    char p[80];

    snprintf(user, sizeof(user), "USER %s 0 0: %s\r\n", nick, nick);
    send(s, user, strlen(user), 0);

    snprintf(nk, sizeof(nk), "NICK %s\r\n", nick);
    send(s, nk, strlen(nk), 0);

    if(strlen(pass) > 0) {
	snprintf(p, sizeof(p), "identify %s %s\r\n", nick, pass);
	send_msg("NICKSERV", p, s);
    }
}

void process_priv_msg(char *str, char *data, int s)
{
    char nick[30];
    char msg[500];

    get_nick(data, nick);
    get_msg(++data, msg);
    send_msg(nick, msg, s);

    memset(nick, '\0', sizeof(nick));
    memset(msg, '\0', sizeof(msg));
}

void get_nick(char *data, char *nick) 
{
    char c;
    ++data;

    while((c = *(data++)) != '!')
	*(nick++) = c;
}

void get_msg(char *data, char *msg)
{
    char *c = strchr(data, ':');
    ++c;

    strcpy(msg, c);
}

void send_msg(char *nick, char *msg, int s)
{
    char n_msg[500];

    snprintf(n_msg, sizeof(n_msg), "PRIVMSG %s :%s\r\n", nick, msg);
    send(s, n_msg, strlen(n_msg), 0);
}

void send_pong(char *data, int s)
{    
    *(strchr(data, 'I')) = 'O';

    send(s, data, strlen(data), 0); 
}
