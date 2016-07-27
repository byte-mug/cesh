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
//#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "luasrc/lua.h"
#include "luasrc/lauxlib.h"
#include "pipelib.h"

typedef const char* string_t;

static int sh_exit (lua_State *L) {
	exit(lua_tointeger(L,1));
	return 0;
}

static int sh_pipeclean (lua_State *L) {
	pipeclean();
	return 0;
}

static int sh_pipeadd (lua_State *L) {
	pipeadd(lua_toboolean(L,1));
	return 0;
}

static int sh_execp (lua_State *L) {
	int i,j,n = lua_gettop(L);
	if(n==0) { exit(0); }
	string_t args[n+1];
	args[n]=NULL;
	for(i=0,j=1;i<n;++i,++j){
		if(!(args[i] = lua_tostring(L,j) )) args[n] = "";
	}
	execvp(args[0],(char*const*)args);
	perror("exec");
	exit(0);
	return 0;
}
static int sh_fork (lua_State *L) {
	lua_settop(L,0);
	pid_t pid = fork();
	if(pid){
		lua_pushinteger(L,pid>0?1:-1);
		lua_pushlightuserdata(L,(void*)(intptr_t)pid);
		return 2;
	}
	pipeapply();
	return 0;
}

static int sh_wait (lua_State *L) {
	int status=0;
	pid_t pid = (pid_t)(intptr_t) lua_touserdata(L,1);
	waitpid(pid,&status,0);
	lua_pushboolean(L,!(WEXITSTATUS(status)));
	return 1;
}

static int sh_getenv(lua_State *L){
	const char* str = lua_tostring(L,1);
	if(!str)return 0;
	str = getenv(str);
	if(!str)return 0;
	lua_pushstring(L,str);
	return 1;
}

static int sh_setenv(lua_State *L){
	const char* k = lua_tostring(L,1);
	const char* v = lua_tostring(L,2);
	if(!k)return 0;
	if(!v)return 0;
	setenv(k,v,1);
	return 1;
}

static const string_t sh_spawnp =
	"local execp = _execp\n"
	"local fork = _fork\n"
	"local wait = _wait\n"
	"function _spawnp(...)\n"
	" local rc,pid = fork(); \n"
	" if rc and rc > 0 then return wait(pid) end \n"
	" if rc then return end \n"
	" execp(...); \n"
	"end\n"
;
static const string_t sh_slot =
	"local tp   = table.pack;\n"
	"function _slot(fu,...)\n"
	" return {fu,tp(...)} \n"
	"end\n"
;

static const string_t sh_pipe =
	"local exit = _exit\n"
	"local fork = _fork\n"
	"local wait = _wait\n"
	"local tp   = table.pack;\n"
	"local tu   = table.unpack;\n"
	"local ipairz  = ipairs; \n"
	"local pipeadd = _pipeadd; \n"
	"local pipeclean = _pipeclean; \n"
	"local function runslot(slot) \n"
	" local rc,pid,fu; \n"
	" rc,pid = fork(); \n"
	" if rc then return {rc,pid} end \n"
	" fu = slot[1]; \n"
	" fu(tu(slot[2])) ; \n"
	" exit(0); \n"
	"end\n"

	"function _pipe(...)\n"
	" local i,slot,t,j \n"
	" t = tp(...); j = {} ;\n"
	" for i,slot in ipairz(t) do \n"
	"  pipeadd(i >= #t); \n"
	"  j[i] = runslot(slot); \n"
	" end\n"
	" pipeclean(); \n"
	" t = true\n"
	" for i,slot in ipairz(j) do \n"
	"  if slot[1] > 0 then t = wait(slot[2]) end ; \n"
	" end\n"
	" return t\n"
	"end\n"
;

static const string_t sh__S =
	"local getenv = _getenv;\n"
	"local setenv = _setenv;\n"
	"local e = setmetatable({},{"
	" __index = function(t,k) "
	"  local env = getenv(k); \n"
	"  t[k] = env; \n"
	"  return env \n"
	" end\n"
	"})\n"
	"_S = e;\n"
	"function export(k)\n"
	" local v = e[k]\n"
	" if v then setenv(k,v) end\n"
	"end\n"
;

void sh_install(lua_State *L){
	pipeinit();
	lua_register(L,"_exit"  , sh_exit  );
	lua_register(L,"_execp" , sh_execp );
	lua_register(L,"_fork"  , sh_fork  );
	lua_register(L,"_wait"  , sh_wait  );
	lua_register(L,"_getenv", sh_getenv);
	lua_register(L,"_setenv", sh_setenv);
	lua_register(L,"_pipeclean", sh_pipeclean);
	lua_register(L,"_pipeadd", sh_pipeadd);
	//lua_register(L,"_spawnp",fsh_spawnp);

	lua_settop(L,0);
	luaL_dostring(L,sh_spawnp);
	luaL_dostring(L,sh_slot);
	luaL_dostring(L,sh_pipe);
	luaL_dostring(L,sh__S);
	lua_settop(L,0);
}


