
#define ERROR_LIB(msg)\
	fprintf(stderr, msg);\
	return 0;
#define CANNOTGETMEMORY ERROR_LIB("ERROR: Cannot get memory\n");

void tg_init(const char * token);
char * tg_request(const char * method, ... );
void tg_clearJSON(void);
char * tg_getFromJSON(const char * param);
const char * tg_getRAWJSON(void);
void tg_clearJSON(void);
char * tg_getJSON(const char * value);