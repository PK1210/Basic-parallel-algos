#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/wait.h>
#include<unistd.h>
#include<pthread.h>

#include<string.h>

#define N 100000

int sorted[N];
//int temp[N];

//CPU_TIME returns the current reading on the CPU clock.
double timer(void)
{
    double value;
    value = (double)clock()/(double)CLOCKS_PER_SEC;
    return value;
}

//Function to fill array with random integers
void generateArr(int n,int *arr,int seed)
{
    srand(seed);
    int i;
    for(i=0;i<n;i++)
    {
        arr[i]=rand()%10000;
        sorted[i]=arr[i];
        //printf("%d ",arr[i]);
    }
    printf("\n");
}

//Comparator function for qsort sort
int comp(const void*a ,const void *b)
{
    return *(int *)a - *(int *)b;
}
void inBuiltSort(int n,int *arr)
{
    double start = timer();
    qsort(arr,n,sizeof(int),comp);
    double stop = timer();
    printf("Time taken by in-built qsort = %9f\n",stop-start);
}

//Function to check validity of 2 sorts
int compareSorts(int *arr,int n)
{
    int i;
    for(i=0;i<n;i++)
        if(arr[i]!=sorted[i])
        {
            printf("Sorting not working\n");
            return 0;
        }
    printf("Sorting worked\n");
    return 1;
}


//For base cases of merge sort
int baseSort(int *arr,int l,int r)
{
    int i,j,min,temp;
    for(i=l;i<=r;i++)
    {
        min=i;
        for(j=i+1;j<=r;j++)
            if(arr[j]<arr[min])
                min=j;
        temp=arr[i];
        arr[i]=arr[min];
        arr[min]=temp;
    }
    return 1;
}


/***************************************************************/
//Normal merge sort merge function
/***************************************************************/
int merger(int *arr,int l,int mid,int r)
{
    int temp[N];
    int x=l;
    int y=mid+1;
    int i=l;
    while(x<=mid && y<=r)
    {
        if(arr[x]<=arr[y])
            temp[i++]=arr[x++];
        else
            temp[i++]=arr[y++];
    }
    while(x<=mid)
        temp[i++]=arr[x++];
    while(y<=r)
        temp[i++]=arr[y++];
    for(i=l;i<=r;i++)
        arr[i]=temp[i];
    return 1;
}
//Normal merge sort caller Function
int normalMergeSort(int *arr,int l,int r)
{
    if(r-l<6)
    {
        baseSort(arr,l,r);
        return 1;
    }
    int mid = (l+r)>>1;
    normalMergeSort(arr,l,mid);
    normalMergeSort(arr,mid+1,r);
    merger(arr,l,mid,r);
    return 1;
}
int normalMergeSortCaller(int *arr,int n)
{
    int i,brr[N];
    for(i=0;i<n;i++)
        brr[i]=arr[i];
    double start = timer();
    normalMergeSort(brr,0,n-1);
    double stop = timer();
    printf("Time taken by normal merge sort = %9f\n",stop-start);
    return 0;
}
/***************************************************************/




/***************************************************************/
//Shared Memory merge sort
/***************************************************************/
struct shmStruct {
    int shmid;
    int *shmaddr;
};
struct shmStruct createSharedMemory(int *arr,int n)
{
    struct shmStruct shm;
    shm.shmid = -1;
    shm.shmaddr = NULL;
    int shmid = shmget(IPC_PRIVATE,sizeof(int)*n,IPC_CREAT|0666);
    if(shmid==-1)
    {
        perror("SHM creation failed");
        return shm;
    }
    shm.shmid = shmid;
    int *brr = (int *)shmat(shmid,NULL,0);
    if(*brr==-1)
    {
        perror("SHM linking failed");
        return shm;
    }
    int i;
    for(i=0;i<n;i++)
        brr[i]=arr[i];
    shm.shmaddr = brr;
    return shm;
}
int deleteSharedMemory(struct shmStruct shm)
{
    int *shmaddr = shm.shmaddr;
    int shmid = shm.shmid;
    int check = shmdt(shmaddr);
    if(check==-1)
    {
        perror("Can't free shm");
        return -1;
    }
    check = shmctl(shmid,IPC_RMID,NULL);
    if(check==-1)
    {
        perror("Can't free shm:");
        return -1;
    }
    return 0;
}
int multiProcessMergeSort(int *arr,int l,int r)
{
    if(r-l<17)
    {
        baseSort(arr,l,r);
        return 1;
    }
    int mid = (l+r)>>1;
    sleep(0);
    int pid1 = fork();
    if(pid1==-1)
    {
        perror("Shared Memory Merge Sort Forking");
        _exit(1);
    }
    if(pid1==0)
    {
        //Child process first one to sort left half
        multiProcessMergeSort(arr,l,mid);
        _exit(0);
    }
    if(pid1>0)
    {
        /*int pid2 = fork();
        if(pid2<0)
        {
            perror("Shared Memory Merge Sort Forking");
            _exit(1);
        }
        if(pid2==0)
        {*/
            //Second child process to sort right half
            multiProcessMergeSort(arr,mid+1,r);
        //  _exit(0);
        //}
        //if(pid2>0)
        {
            int status;
            //Parent process aka original one
            int check = waitpid(pid1,&status,0);
            if(check<0)
            {
                perror("");
                _exit(1);
            }
            //check = waitpid(pid2,&status,0);
            if(check<0)
            {
                perror("");
                return -1;
            }
            if(check<0)
            {
                perror("");
                return -1;
            }
            //Merge function can be same as before
            merger(arr,l,mid,r);
        }
    }
    return 0;
}
int shmMergeSortCaller(int *arr,int n)
{
    struct shmStruct shm = createSharedMemory(arr,n);
    int  *brr = shm.shmaddr;
    double start = timer();
    multiProcessMergeSort(brr,0,n-1);
    double stop = timer();
    compareSorts(brr,n);
    deleteSharedMemory(shm);
    printf("Time taken by shared memory based merge sort = %9f\n",stop-start);
    return 0;
}
/***************************************************************/



/***************************************************************/
//Threaded merge sort
/***************************************************************/
struct argvStruct{
  int *arr;
  int l;
  int r;
};
int threadedMerger(int *arr,int *brr,int *crr,int l,int mid,int r)
{
    printf("Merger\n");
    int x=0;
    int y=0;
    int i=l;
    while(x<=mid-l && y<=r-mid-1)
    {
        if(brr[x]<=crr[y])
            arr[i++]=brr[x++];
        else
            arr[i++]=crr[y++];
    }
    while(x<=mid-l)
        arr[i++]=brr[x++];
    while(y<=r-mid-1)
        arr[i++]=crr[y++];
    return 1;
    printf("Acquistion\n");
}
void * threadedMergeSort(void * argv)
{
    struct argvStruct *args = (struct argvStruct *)argv;
    int *arr = args->arr;
    int l = args->l;
    int r = args->r;
    if(r-l<33)
    {
        baseSort(arr,l,r);
        return 0;
    }
    int mid = (l+r)>>1;
    //printf("Debug left= %d right=%d mid = %d\n",l,r,mid);
    pthread_t thread1,thread2;
    struct argvStruct * argv1=(struct argvStruct *)malloc(sizeof(struct argvStruct));
    struct argvStruct * argv2=(struct argvStruct *)malloc(sizeof(struct argvStruct));



    int size_left = mid-l+1;
    int size_right = r - mid;
    //int * brr = (int *)malloc( sizeof(int) * size_left);
    //int * crr = (int *)malloc( sizeof(int) * size_right);
    //memcpy(brr, arr, size_left * sizeof(int));
    //memcpy(crr, arr + size_left, size_right * sizeof(int));

    argv1->arr = arr;
    argv1->l = l;
    argv1->r = mid;
    //argv1->l = 0;
    //argv1->r = size_left - 1;

    argv2->arr = arr;
    argv2->l = mid + 1;
    argv2->r = r;
    //argv2->l = 0;
    //argv2->r = size_right - 1;


    int iret1 = pthread_create(&thread1,NULL,threadedMergeSort,argv1);
    int iret2 = pthread_create(&thread2,NULL,threadedMergeSort,argv2);
    if(iret1==-1 || iret2==1)
    {
        perror("Threads not created\n");
    }

    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);

    //threadedMerger(arr,brr,crr,l,mid,r);
    merger(arr,l,mid,r);

    //free(brr);
    //free(crr);
    free(argv1);
    free(argv2);
    return NULL;
}
int ptreadMergeSortCaller(int *arr,int n)
{
    struct argvStruct * args=(struct argvStruct *)malloc(sizeof(struct argvStruct));
    args->arr = arr;
    args->l = 0;
    args->r = n-1;
    double start = timer();
    threadedMergeSort(args);
    double stop = timer();
    printf("Time taken by thread based merge sort = %9f\n",stop-start);
    compareSorts(arr,n);
    //for(int i=0;i<n;i++)
    //    printf("%d ",arr[i]);
    //printf("\n");
    return 0;
}
/***************************************************************/

int main()
{
    int arr[N],n,seed;
    printf("Enter any integer: ");
    scanf("%d",&seed);
    printf("Enter the size of array: ");
    scanf("%d",&n);
    generateArr(n,arr,seed);
    inBuiltSort(n,sorted);
    normalMergeSortCaller(arr,n);
    shmMergeSortCaller(arr,n);
    ptreadMergeSortCaller(arr,n);
    return 0;
}
