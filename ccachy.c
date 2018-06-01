
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <stdarg.h>
void cflog(FILE* f,char* color,char* format, ...) {
	va_list args;
	va_start(args,format);
	
	fprintf(f,"\033[%sm",color);
	vfprintf(f,format,args);
	fprintf(f,"\033[0m");
}
void flog(FILE* f,char* format, ...) {
	va_list args;
	va_start(args,format);
	
	fprintf(f,"\033[93m");
	vfprintf(f,format,args);
	fprintf(f,"\033[0m");
}

typedef void(*perline)(char*,int len,void*);

int indexof_char(char* s,int a,int b,char c) {
	int i;
	for (i=a; s[i] && a < b; ++i) {
		if (s[i] == c)
			return i;
	}
	return -1;
}

void file_fread_chunks_callback(FILE* f,int bufsize,perline callback,void* p) {
	
	char* s = (char*)malloc(bufsize+1);
	
	int rsize = fread(s,1,bufsize,f);
	while (rsize == bufsize) {
		callback(s,rsize,p);
		rsize = fread(s,1,bufsize,f);
	}
	if (0 <= rsize && rsize < bufsize) {
		callback(s,rsize,p);
	}
	
	free(s);
}

void file_read_chunks_callback(int fd,int bufsize,perline callback,void* p) {
	char* s = (char*)malloc(bufsize);

	int rsize = read(fd,s,bufsize);
	#ifdef DEBUG
	cflog(stderr,"91","[frcc %i, %i, %x, %x]\n",fd,bufsize,callback,p);
	cflog(stderr,"91","[rsize %i]\n",rsize);
	#endif
	
	while (rsize == bufsize) {
		callback(s,rsize,p);
		rsize = read(fd,s,bufsize);
	}
	if (0 <= rsize && rsize < bufsize) {
		callback(s,rsize,p);
	}
	#ifdef DEBUG
	else if (rsize < 0) {
		strerror_r(errno,s,bufsize);
		cflog(stderr,"31","bad read, errno=%i (%s)\n",errno,s);
	}
	#endif
	
	free(s);
}

void _file_read_new_string_callback(char* s,int len,void* p) {
	
	int nsize;
	char* nbuf;
	struct what {
		int len;
		int size;
		char* buf;
	}* ob;
	ob = (struct what*)p;
	
	#ifdef DEBUG
	char ehh[5000]; strncpy(ehh,s,len); ehh[len] = 0;
	flog(stderr,"ob {%i, %i, %s}\n",ob->len,ob->size,ehh);
	#endif
	
	if (ob->len + len > ob->size) {
		#ifdef DEBUG
		flog(stderr,"expanding\n");
		#endif
		nsize = ob->size*2;
		nbuf = (char*)malloc(nsize+1);
		strncpy(nbuf,ob->buf,ob->len);
		free(ob->buf);
		ob->buf = nbuf;
		ob->size = nsize;
	}
	strncpy(&ob->buf[ob->len],s,len);
	ob->len += len;
}
char* file_read_new_string(int fd,int bufsize) {
	
	struct {
		int len;
		int size;
		char* buf;
	} ob;
	ob.len = 0;
	ob.size = bufsize;
	ob.buf = (char*)malloc(ob.size+1);
	
	file_read_chunks_callback(fd,bufsize,
		_file_read_new_string_callback,
		&ob);
	
	ob.buf[ob.len] = 0;
	return ob.buf;
}

typedef struct {
	int fd;
	int buflen;
	struct sockaddr_un addr;
} socket_data;

void thang(char* buffer,int len,void* p) {
	
	socket_data* sock = (socket_data*)p;
	
	char cbuf[4097];
	strncpy(cbuf,buffer,len);
	cbuf[len] = 0;
	cbuf[4096] = 0;
	char* s;
	
	// Send to socket.
	
	// TODO: ncachy assumes what it receives is cleanly split by '\n', with no
	//   breaks in get, set, or commands.  Maybe I can use that line iterator I
	//   wrote on my first ccachy attempt?  For now, let's just add an option to
	//   set the buffer size to arbitrarily large values.
	//   
	//   Though that would mean that *all* stdin input would have to fit within
	//   said single buffer.  ...  I think that iterator may be my ONLY CHOICE.
	int a = write(sock->fd,buffer,len);
	//write(sock->fd,cbuf,len);
	
	#ifdef DEBUG
	flog(stderr,"[sent... %s]\n",cbuf);
	flog(stderr,"[wrote bytes %i]\n",a);
	#endif
	
	// ncachy's daemon should only respond on accepting a full line, so...
	// 
	// If there's a '\n' somewhere in the send string, or it's the very last read,
	// go ahead and wait to receive something back from the socket.
	//if (indexof_char(cbuf,0,len,'\n') >= 0) {
	if ((indexof_char(cbuf,0,len,'\n') >= 0 &&
				indexof_char(cbuf,0,len,'=') < 0) ||
			len < sock->buflen) {
		// Receive from socket.
		#ifdef DEBUG
		flog(stderr,"[Wait for read, I guess?]\n");
		#endif
		
		// Force an endline when it's the final line and doesn't end in one.
		if (len < sock->buflen && cbuf[len-1] != '\n')
			write(sock->fd,"\n",1);
		
		s = file_read_new_string(sock->fd,4096);
		if (s[0])
			printf("%s",s);
		
		if (s)
			free(s);
	}
	
}

typedef struct {
	char socket[256];
	int args;
} config;
config* init_config(config* c) {
	strcpy(c->socket,"/tmp/jeeze.sock");
	c->args = 0;
	return c;
}

char* arg2(int argc, char** argv, char* arg) {
	int i;
	char* s;
	for (i=0; i < argc-1; ++i) {
		if (strcmp(argv[i],arg) == 0) {
			s = (char*)malloc(strlen(argv[i+1]));
			strcpy(s,argv[i+1]);
			return s;
		}
	}
	return 0;
}

void parse_args(int argc, char** argv, config* conf) {
	char* sockpath;
	int firstarg = 1;
	if (argc > 1 && argv[1][0] == '@') {
		sprintf(conf->socket,"/tmp/%s.sock",&argv[1][1]);
		++firstarg;
	}
	else {
		sockpath = arg2(argc,argv,"--socket");
		if (sockpath) {
			strcpy(conf->socket,sockpath);
			free(sockpath);
		}
	}
	
	if (argc > firstarg)
		conf->args = 1;
}

void usage() {
	
}

int main(int argc, char** argv) {
	
	FILE* f = stdin;
	int conn;
	
	config conf;
	init_config(&conf);
	parse_args(argc,argv,&conf);
	
	socket_data sock;
	sock.addr.sun_family = AF_UNIX;
	strcpy(sock.addr.sun_path,conf.socket);
	// NOTE: buflen use feels... hacky...
	sock.buflen = 4096;
	//sock.buflen = 24000;
	//sock.buflen = 2;
	
	sock.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	
	// NOTE: What do I use, here?  connect?  bind?
	conn = connect(sock.fd,(struct sockaddr*)&sock.addr,sizeof(sock.addr));
	//conn = bind(sock.fd,(struct sockaddr*)&sock,sizeof(sock));
	
	#ifdef DEBUG
	char cbuf[1024];
	flog(stderr,"[res %i]\n",conn);
	if (conn < 0) {
		strerror_r(errno,cbuf,1024);
		cflog(stderr,"31","bad connect, errno=%i (%s)\n",errno,cbuf);
	}
	#endif
	
	// Loop through input read.
	if (conf.args) {
		int i=1;
		if (argv[1][0] == '@')
			++i;
		for (; i < argc; ++i) {
			thang(argv[i],strlen(argv[i]),&sock);
		}
	}
	else
		file_fread_chunks_callback(f,sock.buflen,thang,&sock);
	
	// UNCERTAIN: Do I need this?
	write(sock.fd,"\n",1);
	
	return 0;
}


