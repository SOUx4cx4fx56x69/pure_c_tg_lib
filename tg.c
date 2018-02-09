#include"tg.h"

#define DEBUG


#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdarg.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static char * botToken=0;
#define apiDOMAIN "api.telegram.org"
#define BLOCKSIZE 1024
const char apiURL[] = "https://" apiDOMAIN "/";
static char * RAW_JSON=0;


void tg_clearJSON(void){
	if(RAW_JSON){
		free(RAW_JSON);
		RAW_JSON=0;
	}
}


void
tg_init(const char * token){
	const size_t tokenSize = strlen(token);
	if(botToken){
		free(botToken);
		puts("WARNING: Getting new token");
		botToken=0;
	}
	botToken = (char*)calloc(tokenSize + 4, 1); //token+"bot"+'\0'
	void * fbotToken = (void*)botToken;
	
	memcpy(botToken,"bot",3);
	botToken+=3;
	if(tokenSize > 45)
	 fprintf(stderr, "Warning: size of token more than 32 chars(%d)\n",tokenSize);
	while(*token)
		*(botToken++)=*(token++);
	*(botToken++)='\0';
	 #ifdef DEBUG
		puts("DEBUG: Token inited");
	 #endif
	 botToken = (char*)fbotToken;
	 SSL_library_init();
}


static inline const size_t
getSizeParams(va_list * list){
	int tmp;
	size_t result=0;
	
	while( (tmp = va_arg(*list, int) ) )
		result++;
	va_end(*list);
	return result;
}

static inline const size_t
getSizeParamsValues(char ** params){
	size_t result=0;//=strlen(apiURL);
	while(*params){
		//result+=3;//  ?=...&
		result+=strlen(*params);
		*params++;
		result++;
	}
	return result;
}

static inline void *
initURL(char * URL, const char ** params){
	#ifdef DEBUG
		puts("DEBUG: Init URL");
	#endif
	size_t counter = strlen(apiURL);
	
	memcpy(URL, apiURL, strlen(apiURL) );
	memcpy(URL+counter, botToken, strlen(botToken) );
	counter+= strlen(botToken);
	memcpy(URL+counter, "/", 1);
	counter+=1;

	size_t param_size = strlen(*params);
#define ADDCHAR(ch, counter) strcpy(URL+counter, ch);counter++;
	memcpy(URL+counter, *(params++), param_size );
	counter+=param_size;
	if(*params){
		ADDCHAR("?",counter);
		param_size = strlen(*params);
		strcpy(URL+counter, *(params++));
		counter+=param_size;
	}
	while(*params){
		param_size = strlen(*params);
		ADDCHAR("&",counter);

		strcpy(URL+counter, *(params++));
		counter+=param_size;

	}
	*(URL+counter)='\0';
	#ifdef DEBUG
		printf("DEBUG: inited URL %s\n",URL);
	#endif
}


static inline int initDescriptor(const char * nameserver, const unsigned int port){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1) {
		ERROR_LIB("ERROR: Cannot init socket\n");
	}
	struct sockaddr_in sockaddr;
	struct hostent * address;
	
	address = gethostbyname(nameserver);
	if(address == NULL){
		ERROR_LIB("ERROR: CANNOT GET TELEGRAM API\n");
	}
	#ifdef DEBUG
		puts("DEBUG: Founded TELEGRAM API");
	#endif
	
	bzero(&sockaddr,sizeof(struct sockaddr_in));
	sockaddr.sin_family=AF_INET;
	sockaddr.sin_port=htons(port);
	sockaddr.sin_addr.s_addr=*(long*)(address->h_addr);
	if( connect(fd, (struct sockaddr*)&sockaddr,sizeof(sockaddr) ) == -1  ){
		close(fd);
		ERROR_LIB("ERROR: CANNOT CONNECT TO TELEGRAM\n");
	}
	return fd;
}

static inline SSL_CTX * InitCTX(void){
	SSL_METHOD *method;
	SSL_CTX *ctx;
 
	OpenSSL_add_all_algorithms(); 
	SSL_load_error_strings();   
	method = (SSL_METHOD*)TLSv1_2_client_method();  
	ctx = SSL_CTX_new(method);
	
	if ( ctx == NULL )
        ERR_print_errors_fp(stderr);
	return ctx;
}



static inline char * getOnlyJSON(const char * addr){
	while(*addr){
		while(*addr && *addr != '{'){
			*addr++;
		}
		if(!*addr) return 0;
		else if(*addr == '{') return (char*)addr;
	}
}

static inline char *
https_read(const char * url ){
 int fd = initDescriptor(apiDOMAIN, 443);
 SSL_CTX * ctx = InitCTX();
 if(!fd || !ctx) return 0;
 SSL * ssl = SSL_new(ctx);
 SSL_set_fd(ssl, fd);
 #ifdef DEBUG
	puts("DEBUG: init ssl");
 #endif
 if ( SSL_connect(ssl) == -1 ){
	close(fd);
	ERROR_LIB("ERROR: Error SSL connection\n");
 }
 
 
 char * get = (char*)strstr(url,"/bot");
 if(!*get){
	close(fd);
	ERROR_LIB("ERROR: uncorrect link\n");
 }
 //
  #ifdef DEBUG
	puts("DEBUG: init request");
 #endif
 
 char postquery[1024];
 sprintf(postquery,
	   "GET %s\r\n"
	   "Host: %s\r\n"
	   "\r\n",
	   get, apiDOMAIN
	   );
 puts(get);
 //printf("Size: %d\n",strlen(postquery));
 SSL_write(ssl, postquery, strlen(postquery));
 /*
  *   #ifdef DEBUG
	puts("DEBUG: write params");
 #endif

  * *params++;

 while(*params){
	puts(*params);
	size_t stmp = strlen(*params);
	SSL_write(ssl, *params, stmp);
	SSL_write(ssl, "&", 1);
 }
 */
 //
 
 //TODO: Init json create from params
 char buf[BLOCKSIZE];
 char * returns_tmp = (char*)malloc(BLOCKSIZE);
 size_t counter=0;
 while( SSL_read(ssl, buf, sizeof(buf)) > 0 ){
	if(counter){
		char * tmp;
		tmp = (char*)realloc(returns_tmp, BLOCKSIZE*(counter+1));
		if(!tmp){
			CANNOTGETMEMORY;
		}
		returns_tmp = tmp;
	}
	strcpy( (returns_tmp+(BLOCKSIZE*counter)),buf);
	counter++;
	//puts(buf);
 }
 #ifdef DEBUG
	printf("DEBUG: ANSWER %s\n",returns_tmp);
 #endif
 
 SSL_free(ssl);
 SSL_CTX_free(ctx);
 
 if( getOnlyJSON(returns_tmp) == 0 ){
	printf("WARNING: Cannot get JSON\n");
	free(returns_tmp);
	return 0;
 }
 char * json = calloc( strlen(getOnlyJSON(returns_tmp)) + 1, 1 );
 strcpy(json, getOnlyJSON(returns_tmp));
 
 return json;
 
}

char *
tg_request(const char * method, ... ){
  if(RAW_JSON){
	puts("WARNING: REQUEST WITHOUT GET ANSWER BEFORE\n");
	free(RAW_JSON);
	RAW_JSON=0;
  }
  va_list lp, sl; // list of params; size list
  

  
  va_start(lp, method);
  va_copy(sl, lp);
  
  char ** params = (char**)calloc(  ( getSizeParams(&sl) + 1) + 1, sizeof(char*)  );
  
  if(!params){
	CANNOTGETMEMORY;
  }
  
  size_t counter=0;
  char * tmp = (char*) method;
  register volatile size_t param_size;
  
  while( tmp ){
	param_size = strlen( tmp );
	params[counter] = (char*)calloc(param_size + 1, 1);
	
	if( !(params+counter) ){
		CANNOTGETMEMORY;
	}
	
	strcpy( params[counter++], tmp  );
	
	tmp = va_arg(lp, char*);
  }
  va_end(lp);
  
  char * requesturl = (char*)
  calloc(strlen(apiURL) + strlen(botToken) + strlen(method) + getSizeParamsValues(params) + getSizeParams(&sl) + 3, 1);
  
  if(!requesturl){
	CANNOTGETMEMORY;
  }
  
  initURL(requesturl, (const char**)params);

  RAW_JSON = https_read(requesturl);
  //TODO: Writing JSON
  for(counter;counter--;)
	free(params[counter]);
  free(params);
  

  
  #ifdef DEBUG
	printf("DEBUG: JSON RAW %s\n",RAW_JSON);
  #endif

  if(!RAW_JSON){
		puts("Raw JSON");
		CANNOTGETMEMORY;
  }
  free(requesturl);
}

const char * tg_getRAWJSON(void){
	return RAW_JSON;
}

/*
#define EQUAL_STRING(a,b){\
	while(*a && * b && *a++ == *b++);\
	if(!*b) return true;\
}
*/





const char * tg_getJSON_START(const char * value){
	char * tmp = RAW_JSON;
	while(*value && *tmp){
		if(*value == *tmp){
			size_t counter = 0;
			while( *(value+counter) && *(tmp+counter)
				&& *(value+counter)==*(tmp+counter++) );
			if(!*(value+counter)){
				counter++;
				printf("S: %c %c\n",*(tmp+counter), *(tmp+counter+1));
				
				if(*(tmp+counter) == ':' && *(tmp+counter+1) == '\"')
					return tmp+counter+2;
				//else
				return 0;
			}
			else if((tmp+counter)) return 0;
		}
		*tmp++;
	}
	
	//by default is 0
}

const char * tg_getJSON_END(const char * starting){
	if(!starting) return 0;
	while(*starting){
		if(*starting == '\"')
			return starting;
		*starting++;
	}
	
}

char * tg_getJSON(const char * value){
	const char * starting = tg_getJSON_START(value);
	const char * ending =  tg_getJSON_END(starting);
	if(!starting || !ending ) return 0;
	size_t size_json = ((size_t)ending - (size_t)starting);
	//printf("DEBUG: SIZE PARAM %d\n",size_json);
	char * returns = calloc(size_json+1,1);
	memcpy(returns, (void*)starting, size_json);
	returns[size_json+1]=0;
	return returns;
}

