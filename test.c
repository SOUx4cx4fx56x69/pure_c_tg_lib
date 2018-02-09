#include"tg.h"
#include<stdio.h>


int main(int argcount, char**arguments){
	if(argcount < 2){
		return fprintf(stderr,"%s botToken\n", arguments[0]);
	}
	tg_init(arguments[1]);
	tg_request("getMe",0);
	printf("Request sended, first_name: %s\n",tg_getJSON("first_name"));
	tg_clearJSON();
	tg_request("sendMessage","chat_id=@sooqatime","text=Hello?",0);
}