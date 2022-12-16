#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

struct Page
{
    int v_page_num;
    int valid_bit;
    int dirty_bit;
    int PN;
    int time_stamp;
};

struct Memory
{
    int address;
    int data;
};


//functions
char **getCommands(char *line);
int getNumberOfTokens(char *line);
void showptable();
void initializeEverything();
void initializeptable();
void initializemainmemory();
int addresstopagenum(int vaddr);
bool ispageinmainmemory(int pgn);
void movepagetomainmem();
int getfreemempage();
void showmain();
int findMpnBasedOnTime();
void write(int vir_add, int n);
void moveVtoM(int v_page, int m_page);
void moveMtoV(int v_page, int m_page);

//variables
bool algo = 0; //0 -> FIFO; 1 -> LRU;
struct Page p_table[16];
struct Memory main_memory[32];
struct Memory disk_memory[128];
int counter = 1;

int main(int argc, const char *argv[])
{
    initializeEverything();

    if (argc == 2 && strcmp(argv[1], "LRU") == 0)
    {
        algo = 1;
    }

    while (1)
    {

        char str[80];
        printf("%s", "> ");
        if (!fgets(str, sizeof str, stdin))
            continue;

        //parsing input
        int numTk = getNumberOfTokens(str);
        char **tkns = {NULL};
        tkns = getCommands(str);

        if (numTk == 1)
        {
            char *input_1 = tkns[0];
            if (strcmp(input_1, "quit") == 0)
            {
                break;
            }
            if (strcmp(input_1, "showptable") == 0)
            {
                showptable();
            }
        }

        if (numTk == 2)
        {
            char *input_1 = tkns[0];
            if (strcmp(input_1, "showmain") == 0)
            {
                char *input_2 = tkns[1];
                int num = atoi(&input_2[0]);
                printf("num is: %d\n", num);
                showmain(num);
            }
            else if (strcmp(input_1, "read") == 0)
            {
                char *input_2 = tkns[1];
                int addr = atoi(&input_2[0]);
                // printf("%s%d\n", "read virtual address: ", addr);
                int pgnum = addresstopagenum(addr);
                int idx = addr % 8;
                //IF LRU, INCREMENT.
                if (algo == 1){
                    p_table[pgnum].time_stamp = counter;
                    counter++;
                }

                if (ispageinmainmemory(pgnum) == false)
                {
                    printf("%s\n%d\n", "A Page Fault Has Occurred", -1);
                    int freeMemPage = getfreemempage();
                    if (freeMemPage == -1)
                    {
                        // find what page to swap
                        //identify page num in main memory to remove based on time stamp value
                        //determine based on minimum value of time stamp -> iterate through page table for valid bit == 1
                        //move page <pgnum> to main memory
                        int mpn = findMpnBasedOnTime();
                        // printf("the mpn based on time is: %d\n", mpn);
                        int thisPage = addresstopagenum(main_memory[mpn*8].address);
                        p_table[thisPage].valid_bit = 0;
                        p_table[thisPage].dirty_bit = 0;
                        p_table[thisPage].PN = p_table[thisPage].v_page_num;
                        moveMtoV(pgnum, mpn);
                        moveVtoM(pgnum, mpn);
                        p_table[pgnum].valid_bit = 1;
                        p_table[pgnum].PN = mpn;
                    }
                    else
                    {
                        // printf("\nMove VP: %d to Main Mem %d \n", pgnum, freeMemPage);
                        p_table[pgnum].valid_bit = 1;
                        p_table[pgnum].PN = freeMemPage;
                        //IF FIFO, INCREMENT.
                        if (algo == 0){
                            p_table[pgnum].time_stamp = counter;
                            counter++;
                        }
                        //move page <pgnum> to main memory ex: move pgnum 7 to main memory 2
                        // printf("pgnum = %d, pgnum*8 + 1 = %d\n", pgnum, pgnum*8 + 1);
                        moveVtoM(pgnum, freeMemPage);
                    }
                }
                else
                {
                    // printf("page already in memory \n ");
                    int res = -1;
                    for(int i = 0; i < 32; i++){
                        if (main_memory[i].address == addr){
                            res = main_memory[i].data;
                        }
                    }
                    printf("%d\n", res);
                }
            }
        }
        else if (numTk == 3)
        {
            char *input_2 = tkns[1];
            char *input_3 = tkns[2];
            int vpn = atoi(&input_2[0]);
            int val = atoi(&input_3[0]);
            write(vpn, val);

        }
        free(tkns);
    }
    return 0;
}

void write(int vir_add, int n){
    // printf("writing %d to the virtual address %d\n", n, vir_add);
    int pgnum = addresstopagenum(vir_add);
    p_table[pgnum].dirty_bit = 1;
    //IF LRU, INCREMENT.
    if (algo == 1){
        p_table[pgnum].time_stamp = counter;
        counter++;
    }
    if (ispageinmainmemory(pgnum) == false){
        printf("%s\n", "A Page Fault Has Occurred");
        int freeMemPage = getfreemempage();
        if (freeMemPage == -1)
            {
                int mpn = findMpnBasedOnTime();
                // printf("the mpn based on time is: %d\n", mpn);
                int thisPage = addresstopagenum(main_memory[mpn*8].address);
                p_table[thisPage].valid_bit = 0;
                p_table[thisPage].dirty_bit = 0;
                p_table[thisPage].PN = p_table[thisPage].v_page_num;
                moveMtoV(pgnum, mpn);
                moveVtoM(pgnum, mpn);
                p_table[pgnum].valid_bit = 1;
                p_table[pgnum].PN = mpn;

            }
        else{
            // printf("\nMove VP: %d to Main Mem %d \n", pgnum, freeMemPage);
            p_table[pgnum].valid_bit = 1;
            p_table[pgnum].PN = freeMemPage;
            //IF FIFO, INCREMENT.
            if (algo == 0){
                p_table[pgnum].time_stamp = counter;
                counter++;
            }
            moveVtoM(pgnum, freeMemPage);
        }
    }
    //fifo only increment if page fault, lru increment no matter what (read or write)
    // printf("page in memory - just update it \n ");
    for(int i = 0; i < 32; i++){
        if (main_memory[i].address == vir_add){
            // printf("found it !\n");
            main_memory[i].data = n;
            return;
        }
    }
}

void moveVtoM(int v_page, int m_page){
    for (int i = 0; i < 8; i++){
        main_memory[m_page*8 + i].data = disk_memory[v_page*8 + i].data;
        main_memory[m_page*8 + i].address = v_page*8 + i;
    }
}

void moveMtoV(int v_page, int m_page){ //ex: mpn is 1(8-15) and disk is 8(64-71). taking 24-31 and putting it into disk_memory[64 to 71].
    //mpn is 0. these have addresses 8-15
    //disk_memory[8-15] = main_memory[page 0, which is 0-7]
    int v_addr = main_memory[m_page*8].address;
    for (int i = 0; i < 8; i++){
        disk_memory[v_addr].data = main_memory[m_page*8 + i].data;
        v_addr++;
    }
    // printf("printing disk:\n");
    // for(int i = 0; i < 128; i++){
    //     printf("index: %d: %d\n", disk_memory[i].address, disk_memory[i].data);
    // }
}

int getfreemempage()
{
    int freemempage = -1;
    int memPage[4];

    for (int i = 0; i < 4; i++)
        memPage[i] = 0;

    for (int i = 0; i < 16; i++)
    {
        if (p_table[i].valid_bit == 1)
        {
            memPage[p_table[i].PN] = 1;
        }
    }

    for (int i = 0; i < 4; i++)
        //printf("Mem %d-%d ", i, memPage[i]);

    for (int i = 0; i < 4; i++)
    {
        if (memPage[i] == 0)
        {
            freemempage = i;
            break;
        }
    }

    return freemempage;
}

void movepagetomainmem()
{
    int freemempage = getfreemempage();
    //return freemempage;
}

void showptable()
{
    int i;
    for (i = 0; i < 16; i++)
    {
    
        printf("%d:%d:%d:%d\n", p_table[i].v_page_num, p_table[i].valid_bit, p_table[i].dirty_bit, p_table[i].PN);
    }
}

void showmain(int p)
{
    for (int i = 0; i < 8; i++)
    {
        printf("%d: %d\n", p * 8 + i, main_memory[p * 8 + i].data);
    }
    return;
};

int findMpnBasedOnTime(){
    int curIndex = 0;
    int mymin = 100;
    for(int i = 0; i < 16; i++){
        if (p_table[i].time_stamp == 0 || p_table[i].valid_bit==0 ){continue;} 
        if (p_table[i].time_stamp < mymin){
            mymin = p_table[i].time_stamp;
            curIndex = i;
        }
    }
    return p_table[curIndex].PN;
}

void initializeptable()
{
    int j;
    int i;

    for (i = 0; i < 16; i++)
    {
        p_table[i].v_page_num = i;
        p_table[i].PN = i;
        p_table[i].valid_bit = 0;
        p_table[i].dirty_bit = 0;
        p_table[i].time_stamp = 0;
    }
}

void initializemainmemory()
{
    for (int i = 0; i < 32; i++)
    {
        main_memory[i].address = i;
        main_memory[i].data = -1;
    }
}

void initializediskmemory()
{
    for (int i = 0; i < 128; i++)
    {
        disk_memory[i].address = i;
        disk_memory[i].data = -1;
    }
}

void initializeEverything()
{
    initializeptable();
    initializemainmemory();
    initializediskmemory();
}

int addresstopagenum(int vaddr)
{
    return vaddr / 8;
}

bool ispageinmainmemory(int pgn)
{
    bool pageinmem = false;
    if (p_table[pgn].valid_bit == 1)
    {
        pageinmem = true;
    }
    else
    {
        pageinmem = false;
    }
    return pageinmem;
}


// parse input into an array of strings where each string is a command separated by a space
char **getCommands(char *orig)
{
    char **sub_str = malloc(10 * sizeof(char *));
    for (int i = 0; i < 10; i++)
    {
        sub_str[i] = malloc(20 * sizeof(char));
    }

    char line[80];
    strcpy(line, orig);

    int numTokens = 0;
    char *token = strtok(line, " \n");
    while (token != NULL)
    {
        strcpy(sub_str[numTokens], token);
        numTokens++;
        token = strtok(NULL, " \n");
    }

    return sub_str;
}

//count the number of tokens (commands) in a string
int getNumberOfTokens(char *orig)
{
    char line[80];
    strcpy(line, orig);

    int numTokens = 0;
    char *token = strtok(line, " \n");
    while (token != NULL)
    {
        numTokens++;
        token = strtok(NULL, " \n");
    }

    return numTokens;
}

