/*
 * Copyright (c) 2016 Simon Schmidt
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>


#include <stdio.h>
#include "luasrc/lua.h"
#include "luasrc/lualib.h"
#include "luasrc/lauxlib.h"

#include "token.h"
#include "compile.h"

//char buffer[1<<12];

struct reader{
	sds current;
	FILE* in;
};
static const char * sh_read (lua_State *L,void *data,size_t *size){	
	struct reader *r = data;
	if(r->current)sdsfree(r->current);
	r->current = NULL;
	while(!(r->current)){
		*size = 0;
		if(feof(r->in)) { return NULL; }
		r->current = compile_line(r->in);
	}
	*size = r->current?sdslen(r->current):0;
	//printf("%s",r->current);
	return r->current;
}

/* see functions.c */
void sh_install(lua_State *L);
void sh_install2(lua_State *L);

int main(int argc,const char* const* argv){
	/*
	int n;
	struct reader re = {NULL,NULL};
	if(argc<2) {
		printf("usage: %s <script> args...\n",argv[0]);
		return 1;
	}
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	sh_install(L);
	sh_install2(L);
	re.in = fopen(argv[1],"r");
	if(!re.in)return 1;
	//luaL_loadfile(L,argv[1]);
	lua_load(L,sh_read,&re,argv[1],"bt");

	for(n=1;n<argc;++n){
		lua_pushstring(L,argv[n]);
	}
	lua_pcall(L,n-1,0,0);
	
	return 0;*/

	size_t pos[2];
	int count=0;
	for(;;){
		
		sds str = compile_line(stdin);
		if(!str)continue;
		printf("%s\n",str);
		sdsfree(str);
	}
}
