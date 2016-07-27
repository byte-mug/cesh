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
#include <string.h>
#include "compile.h"
#include "token.h"
#include "libsrc/linenoise.h"

char buffer[1<<12];

static inline int stcmp(const char* str,const char* pref){
	for(;*pref;pref++,str++){
		if(*str!=*pref)return 1;
	}
	return 0;
}


static inline int isIdentFirst(char x){
	return ((x>='a')&&(x<='z'))|((x>='A')&&(x<='Z'))|x=='_';
}

static inline int isDigit(char x){
	return ((x>='0')&&(x<='9'));
}

static size_t numvarlen(const char* c,size_t len){
	size_t i = 2;
	while(i<len){
		if(!isDigit(c[i]))break;
		i++;
	}
	return i;
}

enum {
	F_isPiped = 1<<0,
	F_isAssign = 1<<1,
	F_isSubexpr = 1<<2
};

static sds compile_stock(const char* str,size_t *pos,sds first,int cnt){
	while((cnt>0)&&next_token(str,pos)){
		switch(str[pos[0]]){
		case '(':cnt++;break;
		case ')':cnt--;
		}
		first = sdscat(first," ");
		if(!stcmp(str+pos[0],"~&")){
			first = sdscat(first,"|");
		}else if(str[pos[0]]=='?'){
			first = sdscat(first,"#");
		}else if(str[pos[0]]=='$'){
			first = sdscat(first,"_S.");
			first = sdscatlen(first,str+pos[0]+1,pos[1]-(pos[0]+1));
		}else{
			first = sdscatlen(first,str+pos[0],pos[1]-pos[0]);
		}
	}
	return first;
}
static inline sds compile_shell(const char* str,size_t len, sds first){
	switch(*str){
	case '"':
	case '\'':
		return sdscatlen(first,str,len);
	case '$':
		switch(str[1]){
		case '@':
		case '*':
			first = sdscat(first,"_tu(ARGV)");
			return first;
		case '?':
			first = sdscat(first,"'0'");
			return first;
		case '!':
			first = sdscat(first,"''");
			return first;
		case '$':
			first = sdscat(first,"_getpidi()");
			return first;
		case '0':
			len = 2;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			len = numvarlen(str,len);
			first = sdscat(first,"ARGV[");
			first = sdscatlen(first,str+1,len-1);
			first = sdscat(first,"]");
			return first;
		default:
			if(len==1){
				first = sdscat(first,"'$'");
				return first;
			}
			first = sdscat(first,"_tu(_S[");
			first = sdscatrepr(first,str+1,len-1);
			first = sdscat(first,"])");
			return first;
		}
	}
	first = sdscatrepr(first,str,len);
	return first;
}

static sds pull_off_par(sds inner){
	size_t i = sdslen(inner);
	while(i){
		i--;
		if(inner[i]<=' ')continue;
		if(inner[i]==')'){
			inner[i] = 0;
			sdssetlen(inner,i);
		}
		break;
	}
	return inner;
}

static sds compile_expr(const char* str,int flags,size_t *pos){
	sds first,s1;
	int param,isexpr;
	if(!next_token(str,pos))return NULL;
	isexpr=0;
	{
		first = sdsnewlen(str+pos[0],pos[1]-pos[0]);
		if(!strcmp(first,"do")){
			sdsfree(first);
			if(!next_token(str,pos))return NULL;
			if(str[pos[0]]=='('){
				first = compile_stock(str,pos,sdsnew("("),1);
				isexpr = 1;
			}else
				first = sdsnewlen(str+pos[0],pos[1]-pos[0]);
		}
		if(flags&F_isPiped){
			s1 = sdsnew("_slot(_execp,"); /*)*/
		}else{
			s1 = sdsnew("_spawnp(");/*)*/
		}
		if(isexpr) s1 = sdscatsds(s1,first);
		else s1 = compile_shell(first,sdslen(first),s1);
		sdsfree(first);
		if(!next_token(str,pos)) {
			s1 = sdscat(s1,")");
			return s1;
		}
		{
			first = s1;
			s1 = NULL;
			do{
				
				if(((pos[1]-pos[0])==2)&!stcmp(str+pos[0],"<>")){
					first = sdscat(first,"..");
					if(!next_token(str,pos)) break;
				} else first = sdscat(first,",");
				first = compile_shell(str+pos[0],pos[1]-pos[0],first);
			}while(next_token(str,pos));
			/*(*/
			first = sdscat(first,")");
		}
		return first;
	}
	return NULL;
}

int ln_eof=0;

static sds getLine(FILE* src){
	size_t pos[4];
	const char* str;
	pos[1]=0;
	sds data = NULL;
restart:
	if((src==NULL)){
		str = linenoise("> ");
		if(!str){ ln_eof = 1; return data; }
		data = data?sdscat(data,str):sdsnew(str);
		free(str);
	}else{
		if(!fgets(buffer,sizeof(buffer),src))return data;
		data = data?sdscat(data,buffer):sdsnew(buffer);
	}
	while(next_token(data,pos)){
		if(data[pos[0]]=='\\'){
			pos[2] = pos[0];
			pos[3] = pos[1];
			if(!next_token(data,pos)){
				data[pos[2]]=' ';
				data[pos[3]]=0;
				sdssetlen(data,pos[3]);
				goto restart;
			}
		}
	}
	return data;
}

static char compile_split_1(const char* str,size_t *pos){
	register char chr;
	while(next_token(str,pos)){
		switch(chr=str[pos[0]]){
//		case '=':
//			if(pos[1]>(pos[0]+1))continue;
		case '|':
			return chr;
		}
	}
	return 0;
}

static char compile_split(const char* str,size_t *pos){
	register char chr;
	while(next_token(str,pos)){
		switch(chr=str[pos[0]]){
		case '|':
			return chr;
		}
	}
	return 0;
}

static sds compile_exprq(sds str,int flags){
	size_t pos[2];
	pos[1]=0;
	sds res = compile_expr(str,flags,pos);
	sdsfree(str);
	return res;
}
static sds compile_stockq(sds str,int cnt){
	size_t pos[2];
	pos[1]=0;
	sds res = compile_stock(str,pos,sdsempty(),cnt);
	sdsfree(str);
	return res;
}


sds compile_line_str(sds line){
	size_t pos[3];
	sds comp,part;
	
	pos[1]=0;
	switch( compile_split_1(line,pos) ){
		case '|':{
			comp = sdsnew("_pipe(");
			part = compile_exprq(sdsnewlen(line,pos[0]),F_isPiped);
			comp = sdscatsds(comp,part);
			sdsfree(part);
			pos[2] = pos[1];
			while(compile_split(line,pos)){
				comp = sdscat(comp,",");
				part = compile_exprq(sdsnewlen(line+pos[2],pos[0]-pos[2]),F_isPiped);
				comp = sdscatsds(comp,part);
				sdsfree(part);
				pos[2] = pos[1];
			}
			comp = sdscat(comp,",");
			pos[1] = 0;
			part = compile_expr(line+pos[2],F_isPiped,pos);
			comp = sdscatsds(comp,part);
			comp = sdscat(comp,");");
			sdsfree(line);
			return comp;
		}break;
		case '=':{
			comp = sdsnewlen(line,pos[0]);
			comp = compile_stockq(comp,1);
			comp = sdscat(comp," = ");
			part = compile_expr(line,F_isAssign,pos);
			comp = sdscatsds(comp,part);
			comp = sdscat(comp," ;\n");
			sdsfree(part);
			sdsfree(line);
			return comp;
		}break;
	}
	pos[1]=0;
	if(!next_token(line,pos)) { sdsfree(line); return NULL; }
	
	switch(pos[1]-pos[0]){
	case 2:
		if(!stcmp(line+pos[0],"if")){
			comp = sdsnew("if ");
			part = compile_expr(line,0,pos);
			sdsfree(line);
			comp = sdscatsds(comp,part);
			sdsfree(part);
			comp = sdscat(comp," then\n");
			return comp;
		}
		break;
	case 3:
		if(!stcmp(line+pos[0],"end")){
			sdsfree(line);
			return sdsnew("end ; \n");
		}
		if(!stcmp(line+pos[0],"for")){
			comp = sdsnew("for ");
			comp = compile_stock(line,pos,comp,1);
			sdsfree(line);
			comp = sdscat(comp," do\n");
			return comp;
		}
		break;
	case 4:
		if(!stcmp(line+pos[0],"else")){
			sdsfree(line);
			return sdsnew("else \n");
		}
		break;
	case 5:
		if(!stcmp(line+pos[0],"while")){
			comp = sdsnew("while ");
			part = compile_expr(line,0,pos);
			sdsfree(line);
			comp = sdscatsds(comp,part);
			sdsfree(part);
			comp = sdscat(comp," do\n");
			return comp;
		}
		if(!stcmp(line+pos[0],"local")){
			comp = sdsnew("local");
			comp = compile_stock(line,pos,comp,1);
			sdsfree(line);
			comp = sdscat(comp," ;\n");
			return comp;
		}
		break;
	case 6:
		if(!stcmp(line+pos[0],"elseif")){
			comp = sdsnew("elseif ");
			part = compile_expr(line,0,pos);
			sdsfree(line);
			comp = sdscatsds(comp,part);
			sdsfree(part);
			comp = sdscat(comp," then\n");
			return comp;
		}
		break;
	case 8:
		if(!stcmp(line+pos[0],"function")){
			comp = sdsnew("function");
			comp = compile_stock(line,pos,comp,1);
			sdsfree(line);
			comp = sdscat(comp," \n");
			return comp;
		}
	}
	
	comp = compile_exprq(line,0);
	comp = sdscat(comp," ;\n");
	//printf(__FILE__ " %d: comp=%s\n",__LINE__,comp);
	return comp;
}


sds compile_line(FILE* src){
	sds line = getLine(src);
	if(!line)return NULL;
	return compile_line_str(line);
}


