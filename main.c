#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 1024*1024
#define NUM 100
#define Us 100
#define N 16
#define K 12
#define M 12//M个data rack
#define P 4//P个parity rack
#define DN K/M//每个data rack有的节点数（平均分配）
#define PN (N-K)/P//每个parity rack有的节点数（平均分配）
long total_slt=0,total_new=0;
char write_name[]="(16,12)-12-4.txt";

typedef struct{
char time_stamp[50];
char workload_name[10];
char volumn_id[5];
char op_type[10];
char access_offset[20];
char operated_size[10];
char duration_time[10];
}Trace;

typedef struct{
int start;
int end;
int r;
}Node;

typedef struct{
int stripe_id;
int chunk[K];//对应的块更新则置1，未更新则默认为0
}Stripe;

void trnsfm_char_to_int(char *char_data, long long *data){

    int i=0;
    *data=0LL;

    while(char_data[i]!='\0'){
        if(char_data[i]>='0' && char_data[i]<='9'){
            (*data)*=10;
            (*data)+=char_data[i]-'0';
        }
        i++;
    }
}

void new_strtok(char string[], char divider, char result[]){

    int i,j;

    for(i=0;string[i]!='\0';i++){

        if(string[i]!=divider)
            result[i]=string[i];

        else break;

    }

    // if the i reaches the tail of the string
    if(string[i]=='\0')
        result[i]='\0';

    // else it finds the divider
    else {

        // seal the result string
        result[i]='\0';

        // shift the string and get a new string
        for(j=0;string[j]!='\0';j++)
            string[j]=string[j+i+1];

    }
}

int record(int start,int end,Stripe s[])//返回的是被更新条带的数目
{
    int rtn=0;
    int i;
    int start_stripe=-1;
    int end_stripe=-1;
    int start_chunk_in_stripe=-1;
    int end_chunk_in_stripe=-1;
    start_stripe=start/K;
    end_stripe=end/K;
    start_chunk_in_stripe=start%K;
    end_chunk_in_stripe=end%K;
    while(start_stripe<end_stripe)
    {
        for(i=0;(s[i].stripe_id!=-1)&&i<NUM;i++)
        {
            if(s[i].stripe_id==start_stripe)
            {
                int j;
                for(j=start_chunk_in_stripe;j<K;j++)
                {
                    s[i].chunk[j]=1;
                }
                start_chunk_in_stripe=0;
                start_stripe++;
                break;
            }
        }
        if((s[i].stripe_id==-1)&&i<NUM)
        {
            s[i].stripe_id=start_stripe;
            int j;
            for(j=start_chunk_in_stripe;j<K;j++)
            {
                s[i].chunk[j]=1;
            }
            start_chunk_in_stripe=0;
            start_stripe++;
            rtn++;
        }
    }
    //start_stripe==end_stripe时
    for(i=0;(s[i].stripe_id!=-1)&&i<NUM;i++)
    {
        if(s[i].stripe_id==start_stripe)
        {
            int j;
            for(j=start_chunk_in_stripe;j<=end_chunk_in_stripe;j++)
            {
                s[i].chunk[j]=1;
            }
            break;
        }
    }
    if((s[i].stripe_id==-1)&&i<NUM)
    {
        s[i].stripe_id=start_stripe;
        int j;
        for(j=start_chunk_in_stripe;j<=end_chunk_in_stripe;j++)
        {
            s[i].chunk[j]=1;
        }
        rtn++;
    }
    return rtn;
}

void return_zero(Stripe s[])//有100个条带被更新就计算一次，然后归零重新一轮计算
{
    int i,j;
    for(i=0;i<NUM;i++)
    {
        s[i].stripe_id=-1;
        for(j=0;j<K;j++)
        {
            s[i].chunk[j]=0;
        }
    }

}

void write(int slt_upt,int new_idea,char txt_name[])
{
    FILE* fp;
    if((fp=fopen(txt_name,"a"))==NULL)
    {
        printf("cannot open %s\n",txt_name);
        exit(0);
    }
    fprintf(fp,"%-5d %-5d\n",slt_upt,new_idea);
    fclose(fp);
}

void calculate(Stripe s[])//计算之后会写入txt
{
    int i,j;
    int slt_upt=0,new_idea=0;
    for(i=0;(s[i].stripe_id!=-1)&&i<NUM;i++)
    {
        //计算Selective Parity Updates的traffic
        for(j=0;j<M;j++)
        {
            int ci=0;
            int k;
            for(k=DN*j;k<DN*j+DN;k++)
                ci=ci+s[i].chunk[k];
            if(ci>0)
            {
                if(ci<=PN)
                {
                    slt_upt=slt_upt+P*ci;
                }
                else
                {
                    slt_upt=slt_upt+PN*P;
                }
            }

        }

        //计算new_idea的traffic
        int cmax=0;
        for(j=0;j<M;j++)
        {
            int ci=0;
            int k;
            for(k=DN*j;k<DN*j+DN;k++)
                ci=ci+s[i].chunk[k];
            if(ci>cmax) cmax=ci;
            new_idea=new_idea+ci;
        }

        if(cmax<PN)
        {
            if(new_idea<PN)
            {
                new_idea=new_idea+new_idea*(P-1);
            }
            else
            {
                cmax=PN;
                new_idea=new_idea+N-K-cmax;
            }
        }
        else
        {
            new_idea=new_idea+N-K-cmax;
        }

        /*if(cmax<PN) cmax=PN;
        new_idea=new_idea+N-K-cmax;*/
        //write(slt_upt,new_idea,"CAMRESSDPA01-lvm1.txt");
        printf("%-5d %-5d\n",slt_upt,new_idea);
        total_slt=total_slt+slt_upt;
        total_new=total_new+new_idea;
        slt_upt=0;
        new_idea=0;
    }
}
void simulation(Stripe s[],char trace_name[])
{
    FILE *fp;
    if((fp=fopen(trace_name,"r"))==NULL)
    {
        printf("cannot open file\n");
        exit(0);
    }
    char item[150];
    char divider=',';
    char time_stamp[50];
    char workload_name[10];
    char volumn_id[5];
    char op_type[10];
    char access_offset[20];
    char operated_size[10];
    char duration_time[10];

    long long offset_int=0,size_int=0;
    int start_chunk=0,end_chunk=0;

    int cnt=0;//更新条带数目
    while(fgets(item, sizeof(item), fp))
    {
        new_strtok(item,divider,time_stamp);
        new_strtok(item,divider,workload_name);
        new_strtok(item,divider,volumn_id);
        new_strtok(item,divider,op_type);
        new_strtok(item,divider,access_offset);
        new_strtok(item,divider,operated_size);
        new_strtok(item,divider,duration_time);

        if((strcmp(op_type, "Read"))==0)
        {
            continue;
        }

        trnsfm_char_to_int(access_offset,&offset_int);
        trnsfm_char_to_int(operated_size,&size_int);
        start_chunk=(offset_int)/((long long)CHUNK_SIZE);
        end_chunk=(offset_int+size_int-1)/((long long)CHUNK_SIZE);

        cnt=cnt+record(start_chunk,end_chunk,s);
        if(cnt==Us)
        {
            cnt=0;
            //开始计算
            calculate(s);
            return_zero(s);
        }
    }
    if(s[0].stripe_id!=-1)
    {
        calculate(s);
    }
    fclose(fp);
}

void r(char trace_name[])
{

    //char trace_name[]="rsrch_1.csv";

    Stripe s[NUM];
    return_zero(s);
    simulation(s,trace_name);

    FILE* fp;
    /*if((fp=fopen("(9,6)-3-3.txt","a"))==NULL)
    {
        printf("cannot open file\n");
        exit(0);
    }*/
    if((fp=fopen(write_name,"a"))==NULL)
    {
        printf("cannot open file\n");
        exit(0);
    }
    printf("%s %ld %ld\n",trace_name,total_slt,total_new);
    fprintf(fp,"%-20s %-10ld %-10ld\n",trace_name,total_slt,total_new);
    fclose(fp);
}

int main()
{
    char trace_name[12][20];
    int i;
    FILE *fp;
    if((fp=fopen("trace_name.txt","r"))==NULL)
    {
        printf("cannot open trace_name\n");
        exit(0);
    }
    for(i=0;i<12;i++)
    fscanf(fp,"%s",trace_name[i]);
    fclose(fp);

    for(i=0;i<12;i++)
    {
        r(trace_name[i]);
        total_new=0;
        total_slt=0;
    }
    return 0;
}
