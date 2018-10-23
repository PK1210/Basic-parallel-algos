#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

#define N 1024

pthread_mutex_t print_mutex;

typedef struct{
    int id;
    int number_of_voters;
    int number_of_evms;
    int remaining_voters;
    int voters_arrived;
    int evms_available;
    int voters_waiting_for_evm;
    int evm_signaling;
    pthread_t *voters;
    pthread_t *evms;
    int *evms_slots_busy;
    pthread_cond_t *votes_casted_cond;
    pthread_mutex_t booth_mutex;
    pthread_cond_t evms_available_cond;
    pthread_cond_t voters_waiting_cond;
}Booth;

typedef struct{
    int id;
    int number_of_slots;
    int is_free;
    //int slots_filled;
    Booth * booth;
}Evm;

typedef struct{
    int id;
    int voted;
    int evm_id;
    Booth * booth;
}Voter;

void safePrint(char * str)
{
    pthread_mutex_lock(&print_mutex);
    printf("%s",str);
    pthread_mutex_unlock(&print_mutex);
}

int slotCount()
{
    return rand()%10+1;
}
void polling_ready_evm(Booth * booth,int count,int id)
{
    //Takes 1 second to get evm ready
    sleep(1);
    char str[1024];
    sprintf(str,"Evm %d ready with %d slot in booth %d\n",id,count,booth->id);
    safePrint(str);
    pthread_mutex_lock(&(booth->booth_mutex));
    //booth->evms_available++;
    //printf("Don;t %d\n",booth->evms_slots_busy==NULL);
    booth->evms_slots_busy[id]=0;
    int signalled = 0;
    //printf("Do\n");
    while(booth->voters_waiting_for_evm==0 && booth->remaining_voters!=0)
        pthread_cond_wait(&(booth->voters_waiting_cond),&(booth->booth_mutex));
    //pthread_mutex_unlock(&(booth->booth_mutex));
    //Reduce number of ready evms
    //Signal to voters to call them to evm
    //ISSUE:   Some extra signal might be broadcasted right now
    //pthread_mutex_lock(&(booth->booth_mutex));
    booth->evm_signaling=id;
    booth->evms_available++;
    for(int i=0;signalled<count && i<booth->voters_waiting_for_evm;i++,signalled++)
        pthread_cond_signal(&(booth->evms_available_cond));
    pthread_mutex_unlock(&(booth->booth_mutex));
    //printf("Retunred safely evm %d\n",booth->evm_signaling);
}

void voter_wait_for_evm(Booth * booth,Voter * voter)
{
    int id = voter->id;
    char str[1024];
    sprintf(str,"\tVoter %d waiting for evm on booth %d\n",id,booth->id);
    safePrint(str);
    //Voter pings evm to see if any available
    pthread_mutex_lock(&(booth->booth_mutex));
    booth->voters_waiting_for_evm++;
    pthread_cond_signal(&(booth->voters_waiting_cond));
    pthread_mutex_unlock(&(booth->booth_mutex));
    //printf("debug Siffhlk %d\n",booth->evm_signaling);

    pthread_mutex_lock(&(booth->booth_mutex));
    while(booth->evms_available==0)
        pthread_cond_wait(&(booth->evms_available_cond),&(booth->booth_mutex));
    //printf("debug Siffhlk %d\n",booth->evm_signaling);
    //voter identifies evm with evm_signaling
    voter->evm_id = booth->evm_signaling;
    booth->evms_slots_busy[voter->evm_id]++;
    sprintf(str,"Voter %d has got slot %d at evm %d in booth %d allocated\n",id,booth->evms_slots_busy[voter->evm_id],voter->evm_id,booth->id);
    safePrint(str);
    booth->voters_waiting_for_evm--;
    booth->remaining_voters--;
    if(booth->remaining_voters==0)
    {
        //For removing all evms from wait condition
        pthread_cond_broadcast(&(booth->voters_waiting_cond));
    }
    pthread_mutex_unlock(&(booth->booth_mutex));
}

void voter_in_slot(Booth *booth,Voter * voter)
{
    int id = voter->id;
    char str[1024];
    sprintf(str,"Voter %d casting vote on evm %d in booth %d\n",id,voter->evm_id,booth->id);
    safePrint(str);
    //Assuming voters takes some seconds to walk down the evm
    usleep(50000 * (rand()%3 +1));
    pthread_mutex_lock(&(booth->booth_mutex));
    voter->voted=1;
    //pthread_cond_signal(&(booth->voters_waiting_cond));
    booth->evms_slots_busy[voter->evm_id]--;
    pthread_cond_signal(&(booth->votes_casted_cond[voter->evm_id]));
    pthread_mutex_unlock(&(booth->booth_mutex));
}
void evm_cast_votes(Evm *evm)
{
    pthread_mutex_lock(&(evm->booth->booth_mutex));
    int to_vote = evm->booth->evms_slots_busy[evm->id];
    while(evm->booth->evms_slots_busy[evm->id]>0)
    {
        //printf("Castle of glass\n");
        pthread_cond_wait(&(evm->booth->votes_casted_cond[evm->id]),&(evm->booth->booth_mutex));
    }
    pthread_mutex_unlock(&(evm->booth->booth_mutex));
    if(to_vote)
    {
        char str[1024];
        sprintf(str,"Evm %d in booth %d has completed taking votes\n",evm->id,evm->booth->id);
        safePrint(str);
    }
}
void * voter_thread(void * args)
{
    Voter * voter = (Voter *)args;
    char str[1024];
    sprintf(str,"Voter %d enters booth %d\n",voter->id,voter->booth->id);
    safePrint(str);
    voter_wait_for_evm(voter->booth,voter);
    voter_in_slot(voter->booth,voter);
    //printf("\t\tBye voter %d thread in booth %d\n",voter->id,voter->booth->id);
    return NULL;
}
void * evm_thread(void * args)
{
    Evm * evm = (Evm *)args;
    //while(evm->booth->voters_waiting_for_evm==0)
    //    pthread_cond_wait(&(evm->booth->voters_waiting_cond),&(evm->booth->booth_mutex));
    while(evm->booth->remaining_voters)
    {
        polling_ready_evm(evm->booth,slotCount(),evm->id);
        evm_cast_votes(evm);
        //printf("Remaining voters = %d\n",evm->booth->remaining_voters);
    }
    printf("\t\tEvm %d at booth %d finished voting stage\n",evm->id,evm->booth->id);
    return NULL;
}
void booth_init(Booth * booth)
{
    int number_of_voters = booth->number_of_voters;
    int number_of_evms = booth->number_of_evms;
    pthread_t * voters = (pthread_t *)malloc(sizeof(pthread_t) * number_of_voters);
    pthread_t * evms = (pthread_t *)malloc(sizeof(pthread_t) * number_of_evms);
    pthread_cond_t * votes_casted_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t) * number_of_evms);
    int * evms_slots_busy = (int *)malloc(sizeof(int) * number_of_evms);
    for(int i=0;i<number_of_evms;i++)
        pthread_cond_init(&(votes_casted_cond[i]),NULL);
    booth->voters = voters;
    booth->evms = evms;
    booth->evms_slots_busy = evms_slots_busy;
    booth->votes_casted_cond = votes_casted_cond;
}

void * booth_thread(void * args)
{
    Booth * booth = (Booth *)args;
    char str[1024];
    sprintf(str,"Booth %d has opened\n",booth->id);
    safePrint(str);
    booth_init(booth);
    for(int i=0;i<booth->number_of_evms;i++)
    {
        Evm * evm = (Evm *)malloc(sizeof(Evm));
        evm->id = i;
        evm->booth = booth;
        evm->number_of_slots = 0;
        evm->is_free=1;
        //evm->slots_filled=0;
        int check = pthread_create(&(booth->evms[i]),NULL,evm_thread,evm);
        if(check==-1)
        {
            perror("Can't create thread for evm");
            _exit(1);
        }
    }
    while(booth->voters_arrived<booth->number_of_voters)
    {
        //Voters might arrive at same time or max delay of 1 second
        sleep(rand()%2);
        int id =booth->voters_arrived;
        Voter * voter = (Voter *)malloc(sizeof(Voter));
        voter->id = id;
        voter->booth = booth;
        voter->voted = 0;
        int check = pthread_create(&(booth->voters[id]),NULL,voter_thread,voter);
        if(check==-1)
        {
            perror("Can't create thread for voter");
            _exit(1);
        }
        booth->voters_arrived++;
    }
    for(int i=0;i<booth->number_of_evms;i++)
        pthread_join(booth->evms[i],NULL);

    for(int i=0;i<booth->number_of_voters;i++)
        pthread_join(booth->voters[i],NULL);
    //Closing booth
    free(booth->voters);
    free(booth->evms);
    free(booth->evms_slots_busy);
    printf("\t\tVoters at booth %d done voting\n",booth->id);
    return NULL;
}

Booth * booth_constructor(int id,int number_of_voters,int number_of_evms)
{
    Booth * booth = (Booth *)malloc(sizeof(Booth));
    booth->id = id;
    booth->number_of_voters = number_of_voters;
    booth->number_of_evms = number_of_evms;
    booth->remaining_voters = number_of_voters;
    booth->evms_available = 0;
    booth->voters_waiting_for_evm = 0;
    booth->voters_arrived=0;
    booth->evm_signaling=-1;
    booth->voters = NULL;
    booth->evms = NULL;
    booth->evms_slots_busy = NULL;
    booth->votes_casted_cond = NULL;
    pthread_mutex_init(&(booth->booth_mutex),NULL);
    pthread_cond_init(&(booth->evms_available_cond),NULL);
    pthread_cond_init(&(booth->voters_waiting_cond),NULL);
    return booth;
}
int main()
{
    pthread_mutex_init(&print_mutex,NULL);

    int n,i,number_of_voters,number_of_evms;
    int input[N][2];
    printf("Enter the number of booths: ");
    scanf("%d",&n);
    printf("Enter the number of voters and evm for each booth \n");
    for(i=0;i<n;i++)
    {
        scanf("%d",&input[i][0]);
        scanf("%d",&input[i][1]);
    }

    pthread_t * booths = (pthread_t *)malloc(sizeof(Booth)*n);
    Booth * booth;
    int check;

    for(i=0;i<n;i++)
    {
        //scanf("%d",&number_of_voters);
        //scanf("%d",&number_of_evms);
        number_of_voters=input[i][0];
        number_of_evms=input[i][1];
        booth = booth_constructor(i,number_of_voters,number_of_evms);
        check=pthread_create(&booths[i],NULL,booth_thread,booth);
        if(check==-1)
        {
            perror("Can't create thread for booth");
            _exit(1);
        }
    }
    for(i=0;i<n;i++)
        pthread_join(booths[i],NULL);

    free(booths);
    return 0;
}
