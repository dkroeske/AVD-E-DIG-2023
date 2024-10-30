#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


FILE *fp_in, *fp_out;

char buffer[32];


int main(int argc, char **argv) {

    if( argc < 2 ) { printf("Missing filename\n"); exit(-1); }
    if( fp_in = fopen(argv[1], "r") ) {
        fp_out = fopen("sample.dat", "wb");
        int ibuf = 0;
        char c;
        while( !feof(fp_in) ) {
            c = fgetc(fp_in);
            if( isdigit(c) ) {
                buffer[ibuf++] = c;
            }
            if( c == ',' || iscntrl(c) ) {
                buffer[ibuf] = '\0';
                ibuf = 0;
                printf("0x%.4X:", atoi(buffer));
                unsigned short n = atoi(buffer);
                fwrite((const void *) &n, sizeof(unsigned short), 1, fp_out);
            }
            if( iscntrl(c) ) {
            }


//fgets(buffer, 1024, fp_in);
//            printf("%d: %c", cnt++, c);
        }
        fclose(fp_in);
        fclose(fp_out);
    } else {
        printf("Can't open file\n"); 
        exit(-1);    
    }

    return 1;
}
