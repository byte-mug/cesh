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

static int pout[2];
static int pin;

void pipeinit(){
	pin = pout[1] = pout[0] = -1;
}
void pipeapply(){
	if(pin>=0){ dup2(pin,0); close(pin); }
	if(pout[0]>=0)close(pout[0]);
	if(pout[1]>=0){ dup2(pout[1],1); close(pout[1]); }
	pin = pout[1] = pout[0] = -1;
}
void pipeclean(){
	if(pin>=0)close(pin);
	if(pout[0]>=0)close(pout[0]);
	if(pout[1]>=0)close(pout[1]);
	pin = pout[1] = pout[0] = -1;
}
void pipeadd(int last){
	if(pin>=0)close(pin);
	pin = pout[0];
	if(pout[1]>=0)close(pout[1]);
	if(last)pout[1] = pout[0] = -1;
	else pipe(pout);
}

