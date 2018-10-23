#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>

#define N 1000005
pthread_mutex_t organizer_free_mutex;
pthread_mutex_t print_mutex;
pthread_mutex_t allow_enterCourt_mutex;
pthread_mutex_t set_ready_mutex;
pthread_mutex_t match_started_mutex;

pthread_cond_t organizer_free_cond;
pthread_cond_t sufficient_people_cond;
pthread_cond_t allow_enterCourt_cond;
pthread_cond_t players_ready_cond;
pthread_cond_t match_started_cond;

int organizer_people = 0;
// 0 means organizer has no person
// 1 means 1 player
// 2 means 2 players
// 10 means 1 referee
// 11 means 1 referee and 1 player
// 12 means 1 referee and 2 players

int allow_enterCourt = 0;
//Number of people allowed to enter the court

int players_ready = 0;
//Number of players finished warmup

int match_started = 0;
//If match is started then 1 else 0

typedef struct {
    int id;
} person;

int randomPerson(int playerRemaining,int refereeRemaining)
{
    int random=rand()%(playerRemaining + refereeRemaining);
    if(random<playerRemaining)
        return 1;
    return 0;
}

void safePrint(char * str)
{
    pthread_mutex_lock(&print_mutex);
    printf("%s",str);
    pthread_mutex_unlock(&print_mutex);
}

void enterAcedemyPlayer(int id)
{
    //Player has entered Acedemy
    char str[1024];
    sprintf(str,"Player %d entered the acedemy\n",id);
    safePrint(str);
}
void enterAcedemyReferee(int id)
{
    //Referee enter the acedemy
    char str[1024];
    sprintf(str,"Referee %d entered the acedemy\n",id);
    safePrint(str);
}


void getPeople()
{
    pthread_mutex_lock(&organizer_free_mutex);
    safePrint("\tOrganizer waiting for people\n");
    while(organizer_people!=12)
        pthread_cond_wait(&sufficient_people_cond,&organizer_free_mutex);
    safePrint("\tOrganizer got players\n");
    pthread_mutex_unlock(&organizer_free_mutex);
}
//Organizer working Function
void organizerJob(int match)
{
    pthread_mutex_lock(&organizer_free_mutex);
    char str[1024];
    sprintf(str,"\tMatch %d is starting\n",match);
    safePrint(str);

    //Allow 3 persons to enter the court
    pthread_mutex_lock(&allow_enterCourt_mutex);
    allow_enterCourt=3;
    pthread_cond_broadcast(&allow_enterCourt_cond);
    pthread_mutex_unlock(&allow_enterCourt_mutex);

    //Wait for game started signal from referee
    sprintf(str,"\tOrganizer waiting for referee's signal for start of match %d\n",match);
    safePrint(str);
    pthread_mutex_lock(&match_started_mutex);
    while(match_started==0)
        pthread_cond_wait(&match_started_cond,&match_started_mutex);
    match_started = 0;
    pthread_mutex_unlock(&match_started_mutex);
    sprintf(str,"\tMatch %d started\n",match);
    safePrint(str);

    //Ending the match
    organizer_people=0;
    pthread_cond_broadcast(&organizer_free_cond);
    pthread_mutex_unlock(&organizer_free_mutex);
    sprintf(str,"\tOrganizer is free\n");
    safePrint(str);
}
void meetOrganizerPlayer(int id)
{
    //No two persons should meet organizer at the same time
    pthread_mutex_lock(&organizer_free_mutex);
    while(organizer_people%10==2)
        pthread_cond_wait(&organizer_free_cond,&organizer_free_mutex);
    organizer_people++;
    if(organizer_people==12)
        pthread_cond_signal(&sufficient_people_cond);
    char str[1024];
    sprintf(str,"Player %d met organizer\n",id);
    safePrint(str);
    pthread_mutex_unlock(&organizer_free_mutex);
}
void meetOrganizerReferee(int id)
{
    //No two referee should meet organizer at the same time
    pthread_mutex_lock(&organizer_free_mutex);
    while(organizer_people/10==1)
        pthread_cond_wait(&organizer_free_cond,&organizer_free_mutex);
    organizer_people+=10;
    if(organizer_people==12)
        pthread_cond_signal(&sufficient_people_cond);
    char str[1024];
    sprintf(str,"Referee %d met organizer\n",id);
    safePrint(str);
    pthread_mutex_unlock(&organizer_free_mutex);
}
void enterCourtPlayer(int id)
{
    pthread_mutex_lock(&allow_enterCourt_mutex);
    while(allow_enterCourt==0)
        pthread_cond_wait(&allow_enterCourt_cond,&allow_enterCourt_mutex);
    allow_enterCourt--;
    pthread_mutex_unlock(&allow_enterCourt_mutex);
    char str[1024];
    sprintf(str,"\tPlayer %d enters the court\n",id);
    safePrint(str);
}
void enterCourtReferee(int id)
{
    pthread_mutex_lock(&allow_enterCourt_mutex);
    while(allow_enterCourt==0)
        pthread_cond_wait(&allow_enterCourt_cond,&allow_enterCourt_mutex);
    allow_enterCourt--;
    pthread_mutex_unlock(&allow_enterCourt_mutex);
    char str[1024];
    sprintf(str,"\tReferee %d enters the court\n",id);
    safePrint(str);
}
void warmUp(int id)
{
    char str[1024];
    sprintf(str,"\t\tPlayer %d has started warmup\n",id);
    safePrint(str);
    sleep(1);
    //Warm up can occur parallely but set ready should not be
    pthread_mutex_lock(&set_ready_mutex);
    players_ready++;
    pthread_cond_signal(&players_ready_cond);
    pthread_mutex_unlock(&set_ready_mutex);
    sprintf(str,"\t\tPlayer %d has finished warmup\n",id);
    safePrint(str);
}
void adjustEquipments(int id)
{
    char str[1024];
    sprintf(str,"\t\tReferee %d has started adjusting the equipments\n",id);
    safePrint(str);
    //sleep for 0.5 seconds
    usleep(50000);
    sprintf(str,"\t\tReferee %d has finished adjusting the equipments\n",id);
    safePrint(str);
}
void startGame(int id)
{
    //waits for 2 players to get ready
    char str[1024];
    sprintf(str,"\tReferee %d waiting for players to finish warmup\n",id);
    safePrint(str);
    pthread_mutex_lock(&set_ready_mutex);
    while(players_ready<2)
        pthread_cond_wait(&players_ready_cond,&set_ready_mutex);
    players_ready = 0;
    //As two ready players are now taken for match
    pthread_mutex_unlock(&set_ready_mutex);

    //Signal organizer so that he can be free
    pthread_mutex_lock(&match_started_mutex);
    match_started=1;
    pthread_cond_signal(&match_started_cond);
    pthread_mutex_unlock(&match_started_mutex);

    sprintf(str,"\tReferee %d singnals that match has started to organizer\n",id);
    safePrint(str);
}

void * organizerThread(void * argv)
{
    const int total_matches = *(int *)argv;
    int matches = 0;
    while(matches < total_matches)
    {
        getPeople();
        organizerJob(matches);
        matches++;
    }
    return NULL;
}
void * playerThread(void * argv)
{
    const person * player = argv;
    int id = player->id;

    enterAcedemyPlayer(id);
    meetOrganizerPlayer(id);
    enterCourtPlayer(id);
    warmUp(id);

    return NULL;
}
void * refereeThread(void * argv)
{
    const person * referee = argv;
    int id = referee->id;

    enterAcedemyReferee(id);
    meetOrganizerReferee(id);
    enterCourtReferee(id);
    adjustEquipments(id);
    startGame(id);

    return NULL;
}

int main()
{
    int n;
    printf("Enter number of matches: ");
    scanf("%d",&n);

    srand(6);

    const int playerCount = (n<<1);
    const int refereeCount = n;
    int playerRemaining = playerCount;
    int refereeRemaining = refereeCount;


    pthread_t players[playerCount];
    pthread_t referees[refereeCount];
    pthread_t organizer;
    person playerInfo[playerCount];
    person refereeInfo[refereeCount];

    pthread_mutex_init(&print_mutex,NULL);
    pthread_mutex_init(&organizer_free_mutex,NULL);
    pthread_mutex_init(&allow_enterCourt_mutex,NULL);
    pthread_mutex_init(&set_ready_mutex,NULL);
    pthread_mutex_init(&match_started_mutex,NULL);

    pthread_cond_init(&organizer_free_cond,NULL);
    pthread_cond_init(&sufficient_people_cond,NULL);
    pthread_cond_init(&allow_enterCourt_cond,NULL);
    pthread_cond_init(&players_ready_cond,NULL);
    pthread_cond_init(&match_started_cond,NULL);

    int check = pthread_create(&organizer,NULL,organizerThread,&n);
    if(check==-1)
    {
        perror("Can't create thread for organizer");
        _exit(1);
    }

    while(playerRemaining || refereeRemaining)
    {
            if(randomPerson(playerRemaining,refereeRemaining))
            {
                int id = playerCount - playerRemaining;
                playerInfo[id].id = id;
                int check = pthread_create(&players[id],NULL,playerThread,&playerInfo[id]);
                if(check==-1)
                {
                    perror("Can't create thread for player");
                    _exit(1);
                }
                playerRemaining--;
            }
            else
            {
                int id = refereeCount - refereeRemaining;
                refereeInfo[id].id = id;
                int check = pthread_create(&referees[id],NULL,refereeThread,&refereeInfo[id]);
                if(check==-1)
                {
                    perror("Can't create thread for referee");
                    _exit(1);
                }
                refereeRemaining--;
            }
       sleep(rand()%3);
   }
   for (int i = 1; i <= 2*n; i++)
        pthread_join(players[i],NULL);
   for(int i=1;i<=n;i++)
        pthread_join(referees[i],NULL);

   pthread_join(organizer,NULL);

   printf("Bye\n");
   return 0;
}
