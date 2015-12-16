#include <stdio.h>
#include <string.h>

int primary(char *mac);
int characteristic();
int ReadValue();
int WriteValue();
int count=0;
char add[50] = " D8:AD:4A:AA:42:B5";
int primary(char *mac)
{
    int err=0;
    char cmd[150];
    system("hciconfig hci down");
    system("hciconfig hci up");
    sprintf(cmd,"gatttool -b  %s --primary -t random >primary.txt",mac);
    system(cmd);
    FILE * fp;
    char *pch;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("primary.txt", "r");
    if (fp == NULL)
       exit(-1);

    while ((read = getline(&line, &len, fp)) != -1) {
       count++;
        printf("%s",line);
    }

    if(count==0)
    err=-1;
    count = 0;

    fclose(fp);
    if (line)
       free(line);
    return err;
}
int characteristics()
{
    int err=0;
    system("hciconfig hci down");
    system("hciconfig hci up");
    system("gatttool -b  D8:AD:4A:AA:42:B5 --characteristics -s 9 -e 0xffff -t random >characteristics.txt");
    FILE * fp;
    char *pch;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("characteristics.txt", "r");
    if (fp == NULL)
       exit(-1);

    while ((read = getline(&line, &len, fp)) != -1) {
        count++;
       if(pch = strstr(line,"uuid"))
        printf("%s",pch+6);
    }

    if(count==0)
    err=-1;
    count = 0;

    fclose(fp);
    if (line)
       free(line);
    return err;
}
int ReadValue()
{
    int err=0;
    system("hciconfig hci down");
    system("hciconfig hci up");
    system("gatttool -b  D8:AD:4A:AA:42:B5 --char-read -a 0xe -t random >ReadValue.txt");
    FILE * fp;
    char *pch;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("ReadValue.txt", "r");
    if (fp == NULL)
       exit(-1);

    while ((read = getline(&line, &len, fp)) != -1) {
       count++;
        printf("%s",line);
}

    if(count==0)
    err=-1;
    count = 0;

    fclose(fp);
    if (line)
       free(line);
    return err;

}
int WriteValue()
{
    int err=0;
    system("hciconfig hci down");
    system("hciconfig hci up");
    char v[50],cmd[150];
    printf("Enter the Value: ");
    scanf("%s",v);
    sprintf(cmd,"gatttool -b  D8:AD:4A:AA:42:B5 --char-write-req -a 0xe -n %s -t random > WriteValue.txt",v);
    system(cmd);
    FILE * fp;
    char *pch;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("WriteValue.txt", "r");
    if (fp == NULL)
       exit(-1);

    while ((read = getline(&line, &len, fp)) != -1) {
       count++;
        printf("%s",line);
    }


    if(count==0)
    err=-1;
    count = 0;

    fclose(fp);
    if (line)
       free(line);
    return err;
}
int main ()
{
    while(1){
        if(primary(add)==0)
        break;

    }
   while(1){
        if(characteristics()==0)
        break;

   }


   while(1){
       if(ReadValue()==0)
       break;

   }
    while(1)
    {
        if(WriteValue()==0)
        break;

    }


   return 0;
}
