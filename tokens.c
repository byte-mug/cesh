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
#include <stdlib.h>
#include <stdio.h>
#include "token.h"

static inline int stcmp(const char* str,const char* pref){
	for(;*pref;pref++,str++){
		if(*str!=*pref)return 1;
	}
	return 0;
}

static inline int isIdent(char x){
	return ((x>='a')&&(x<='z'))|((x>='A')&&(x<='Z'))|((x>='0')&&(x<='9'))|x=='_';
}
static inline int isIdentFirst(char x){
	return ((x>='a')&&(x<='z'))|((x>='A')&&(x<='Z'))|x=='_';
}

static inline int isHex(char x){
	return ((x>='a')&&(x<='f'))|((x>='A')&&(x<='F'))|((x>='0')&&(x<='9'));
}

static inline int isDigit(char x){
	return ((x>='0')&&(x<='9'));
}
static inline int isOct(char x){
	return ((x>='0')&&(x<='7'));
}

int next_token(const char* str,size_t* pos){
	size_t i=pos[1];
	char echr;
	while((str[i]>0) && (str[i]<=' '))i++;
	if(str[i]==0)return 0;
	pos[0]=i;
	switch(echr = str[i]){
	case 0: return 0;
	case '"':
	case '\'':
		i++;
		while(str[i]!=echr){
			if(str[i]==0)return 0;
			if(str[i]=='\\')i++;
			i++;
		}
		i++;
		pos[1]=i;
		return 1;
	case '#': return 0;
	case '$':
		i++;
		while(isIdent(str[i]))i++;
		pos[1]=i;
		return 1;
	default:
		
		if(!stcmp(str+i,"...")){
			pos[1]=i+3;
			return 1;
		}
		if(
			(!stcmp(str+i,"<<"))||
			(!stcmp(str+i,">>"))||
			(!stcmp(str+i,".."))||
			(!stcmp(str+i,"//"))||
			(!stcmp(str+i,"<="))||
			(!stcmp(str+i,">="))||
			(!stcmp(str+i,"=="))||
			(!stcmp(str+i,"~="))||
			(!stcmp(str+i,"~&"))
		){
			pos[1]=i+2;
			return 1;
		}
		if(isIdentFirst(echr)){
			for(i++;isIdent(str[i]);i++);
			pos[1]=i;
			return 1;
		}
		if(echr=='0'){
			i++;
			switch(str[i]){
			case 'x':
				for(i++;isHex(str[i]);i++);
				break;
			default:
				for(;isOct(str[i]);i++);
			}
			pos[1]=i;
			return 1;
		}
		if(isDigit(echr)){
			for(i++;isDigit(str[i]);i++);
			pos[1]=i;
			return 1;
		}
		pos[1]=i+1;
		return 1;
	}
	return 0;
}
