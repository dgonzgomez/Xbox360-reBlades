#include "library.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static uint32_t be32(const unsigned char *p)
{return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int compare(const void *a,const void *b)
{return strcmp(((const BladesGame *)a)->name,((const BladesGame *)b)->name);}
int library_scan(const char *root,BladesGame *games,int max)
{
    if(!root||!games||max<1)return -1;
    DIR *titles=opendir(root);if(!titles)return -1;
    int count=0;struct dirent *title;
    while(count<max&&(title=readdir(titles))) {
        if(strlen(title->d_name)!=8)continue;
        int valid=1;for(int i=0;i<8;i++)if(!isxdigit((unsigned char)title->d_name[i]))valid=0;
        if(!valid)continue;
        char folder[512];
        int len=snprintf(folder,sizeof folder,"%s/%s/00007000",root,title->d_name);
        if(len<0||(size_t)len>=sizeof folder)continue;
        DIR *packages=opendir(folder);if(!packages)continue;
        struct dirent *entry;
        while(count<max&&(entry=readdir(packages))) {
            if(entry->d_name[0]=='.'||strstr(entry->d_name,".data"))continue;
            BladesGame game={0};
            len=snprintf(game.path,sizeof game.path,"%s/%s",folder,entry->d_name);
            if(len<0||(size_t)len>=sizeof game.path)continue;
            FILE *file=fopen(game.path,"rb");if(!file)continue;
            unsigned char header[0x511];size_t got=fread(header,1,sizeof header,file);fclose(file);
            /* XContentMetadata layout: Xenia stfs_xbox.h, header 0x344,
             * content type 0x344, execution title ID 0x360, name 0x411. */
            if(got!=sizeof header||(memcmp(header,"LIVE",4)&&memcmp(header,"PIRS",4)&&memcmp(header,"CON ",4)))continue;
            if(be32(header+0x344)!=0x7000||be32(header+0x340)<sizeof header)continue;
            game.title_id=be32(header+0x360);
            if(game.title_id!=(uint32_t)strtoul(title->d_name,NULL,16))continue;
            for(int i=0;i<127;i++) {
                unsigned ch=(unsigned)header[0x411+i*2]<<8|header[0x412+i*2];
                if(!ch)break;
                game.name[i]=ch>=32&&ch<127?(char)ch:'?';
            }
            if(!game.name[0])snprintf(game.name,sizeof game.name,"Title %08X",(unsigned)game.title_id);
            games[count++]=game;
        }
        closedir(packages);
    }
    closedir(titles);qsort(games,(size_t)count,sizeof *games,compare);return count;
}
