#include <stdio.h>
#include <stdlib.h>

int my_write(FILE *pf,long long len,unsigned char *data)
{
    long long ret = 0;
    long long total = 0;
    while(1)
    {
        ret = fwrite(data,1,len,pf);
        if (ret == len)
            break;
        else if (ret > 0)
            total += ret;
        else if (ret <= 0)
        {
            fprintf(stderr,"fwrite failed,ret = %d",ret);
            exit(-1);
        }
        if (total == len)
            break;
    }

    return 0;
}