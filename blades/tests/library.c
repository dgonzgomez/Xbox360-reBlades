#define _DARWIN_C_SOURCE
#include "library.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static void put32(unsigned char *p,unsigned value)
{p[0]=value>>24;p[1]=value>>16;p[2]=value>>8;p[3]=value;}
int main(void)
{
    char root[]="/tmp/blades-library-XXXXXX";assert(mkdtemp(root));
    char title[512],folder[512],package[512],bad[512];
    snprintf(title,sizeof title,"%s/1234ABCD",root);assert(!mkdir(title,0700));
    snprintf(folder,sizeof folder,"%s/1234ABCD/00007000",root);assert(!mkdir(folder,0700));
    snprintf(package,sizeof package,"%s/1234ABCD/00007000/package",root);
    unsigned char header[0x511]={0};memcpy(header,"LIVE",4);
    put32(header+0x340,0xB000);put32(header+0x344,0x7000);put32(header+0x360,0x1234ABCD);
    const char *name="A Real GOD Title";
    for(int i=0;name[i];i++)header[0x412+i*2]=(unsigned char)name[i];
    FILE *file=fopen(package,"wb");assert(file);assert(fwrite(header,1,sizeof header,file)==sizeof header);fclose(file);
    snprintf(bad,sizeof bad,"%s/1234ABCD/00007000/truncated",root);
    file=fopen(bad,"wb");assert(file);fwrite(header,1,100,file);fclose(file);
    BladesGame games[2];assert(library_scan(root,games,2)==1);
    assert(!strcmp(games[0].name,name)&&games[0].title_id==0x1234ABCD);
    assert(!strcmp(games[0].path,package));
    header[0x347]=1;file=fopen(package,"wb");assert(file);fwrite(header,1,sizeof header,file);fclose(file);
    assert(library_scan(root,games,2)==0); /* Wrong content type is not a game. */
    assert(library_scan("/nonexistent-blades-content",games,2)==-1);
    assert(!unlink(package));assert(!unlink(bad));assert(!rmdir(folder));assert(!rmdir(title));assert(!rmdir(root));
    puts("GOD discovery validates metadata, names, paths, truncation and content type.");
}
