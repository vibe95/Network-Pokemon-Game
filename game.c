/*#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct player_info {
	char *name;
	int hitpoints;
	int powermoves;
} player;

int main(){
	
}

int status(player *one, player *two){
	printf("\nYour hitpoints: %d", one->hitpoints);
	printf("\nYour powermoves: %d", one->powermoves);
	printf("\n\n%s's hitpoints: %d", two->name, two->hitpoints);
}
*/
/*
 * socket demonstrations:
 * This is the server side of an "internet domain" socket connection, for
 * communicating over the network.
 *
 * In this case we are willing to wait either for chatter from the client
 * _or_ for a new connection.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#ifndef PORT
    #define PORT 30100
#endif

#define MAX_NAME 250

struct client {
    int fd;
    struct in_addr ipaddr;
    struct client *next;
    struct player *info;
};

struct player {
    char name[MAX_NAME];
    int status; // stores if in a match (1) or not (0)
    int hitpoints;
    int powermoves;
    int last_played;	//keeps track of last played opponents fd
};

static struct client *addclient(struct client *top, int fd, struct in_addr addr);
static struct client *removeclient(struct client *top, int fd);
static void broadcast(struct client *top, char *s, int size);
int game(struct client *p, struct client *top);
static void match(struct client *top,struct client *p1);
static void game2(struct client *p1,struct client *p2);

int bindandlisten(void);

int main(void) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct client *head = NULL;
    socklen_t len;
    struct sockaddr_in q;
    struct timeval tv;
    fd_set allset;
    fd_set rset;
    int i;

    int listenfd = bindandlisten();
    // initialize allset and add listenfd to the
    // set of file descriptors passed into select
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    // maxfd identifies how far into the set to search
    maxfd = listenfd;
    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;
        /* timeout in seconds (You may not need to use a timeout for
        * your assignment)*/
        tv.tv_sec = 10;
        tv.tv_usec = 0;  /* and microseconds */

        nready = select(maxfd + 1, &rset, NULL, NULL, &tv);
        if (nready == 0) {
            printf("No response from clients in %ld seconds\n", tv.tv_sec);
            continue;
        }

        if (nready == -1) {
            perror("select");
            continue;
        }

        if (FD_ISSET(listenfd, &rset)){
            printf("a new client is connecting\n");
            len = sizeof(q);
            if ((clientfd = accept(listenfd, (struct sockaddr *)&q, &len)) < 0) {
                perror("accept");
                exit(1);
            }
            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("connection from %s\n", inet_ntoa(q.sin_addr));
            head = addclient(head, clientfd, q.sin_addr);
            
        }

        for(i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &rset)) {
                for (p = head; p != NULL; p = p->next) {
                    if (p->fd == i) {
                        int result = game(p, head);
                        if (result == -1) {
                            int tmp_fd = p->fd;
                            head = removeclient(head, p->fd);
                            FD_CLR(tmp_fd, &allset);
                            close(tmp_fd);
                        }
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

int game(struct client *p, struct client *top) {
    char buf[256];
    char outbuf[356];
    int len = read(p->fd, buf, sizeof(buf) - 1);
    struct client *temp_p;
    struct player *i;
    struct player *i_opp;
	struct client *p2;
    struct player *i1;
    struct player *i2;
	i1=p->info;
	for (p2 = top; p2; p2 = p2->next) {
		i2=p2->info;
		if ((i2->status==0) & (i1->last_played != p2->fd) & (p->fd != p->fd)) {
			printf("Creating a match between %s and %s",i1->name,i2->name);
			i2->status=1;
			i1->status=1;
			i1->last_played=p2->fd;
			i2->last_played=p->fd;
			break;}}
    if (len > 0) {
        buf[len] = '\0';
        printf("Received %d bytes: %s", len, buf);
        sprintf(outbuf,"%s says: %s", inet_ntoa(p->ipaddr), buf);
        broadcast(top, outbuf, strlen(outbuf));
        return 0;
    } else if (len == 0) {
        // socket is closed
		i=p->info; 
		char message[350]="**\0";
		sprintf(message,"** %s leaves the arena ** \r\n", i->name);
		broadcast(p,message,sizeof(message)-1);		
        printf("Disconnect from %s\n", inet_ntoa(p->ipaddr))
        for (temp_p = top; temp_p; temp_p = temp_p->next) {
			count++;
			if ((temp_p->fd == i->last_played) & (count >1)){
				i_opp=(temp_p)->info;
				i_opp->status=0; 
				write(temp_p->fd,"Your opponent has left, you won !!:)\r\n", 40);

			break;}}
        return -1;
    } else { // shouldn't happen
        perror("read");
        return -1;
    }
}

 /* bind and listen, abort on error
  * returns FD of listening socket
  */
int bindandlisten(void) {
    struct sockaddr_in r;
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);    
    }
    int yes = 1;
    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
        perror("setsockopt");
    }
    memset(&r, '\0', sizeof(r));
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&r, sizeof r)) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5)) {
        perror("listen");
        exit(1);
    }
    return listenfd;
}

static struct client *addclient(struct client *top, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    struct player *i = malloc(sizeof(struct player));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));
    
    p->fd = fd;
    p->ipaddr = addr;
         
    //Asks client for name
    char letter;
    write(fd, "Please enter your name: ", 25);
    while(1){
	if(read(fd,&letter, MAX_NAME- 2)==-1)
	    perror("Name");
	else{
		if (letter=='\n'){
	    		strncat(i->name,"\0",1);
	    		break;}
		else
	    		strncat(i->name,&letter,1);
	  	}
	}

    p->next = top;
    top = p;
    i->status=0;
    i->last_played=-2;
    p->info=i;	
    //broadcast that new player entered arena
    char message[350]="**\0";
    sprintf(message,"** %s enters the arena ** \r\n", i->name);
    broadcast(p,message,sizeof(message)-1);

    //look for a match
    write(fd,"**Awaiting an opponent**\r\n", 28);
    game(p,top); 
    return top;
}

static struct client *removeclient(struct client *top, int fd) {
    struct client **p;

    for (p = &top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        //broadcast that player has left
		    
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
	
    return top;
}


static void broadcast(struct client *top, char *s, int size) {
    struct client *p;
    for (p = top; p; p = p->next) {
        write(p->fd, s, size);
    }
    /* should probably check write() return value and perhaps remove client */
}

static void match(struct client *top,struct client *p1){
    	/*Need to match to clients not playing, and create a game between them
		   Then need to use select to go through every game(player_info), and order the execution of the game depending on when the press (a,p or s) I think
		   Then change status to 0 when game ends or some leaves....
		   also need to generate initial hit-points (# of powermoves) in match, all other random numbers will*/
}

static void game2(struct client *p1,struct client *p2){
		/* randomly creates valuse for attack and then subtract it from oppenent. saving it in thier hitpoints. the oponnets stauts chanes to 2
			status: 0 not in game, 1 in game waiting for oppenent, 2 your move */

}