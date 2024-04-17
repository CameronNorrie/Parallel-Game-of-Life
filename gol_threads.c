/*
 *  CIS*3090 Assignment
 *  author: Cameron Norrie
 *  Project: Pthread implementation of the Game Of Life
 *  Date: oct 13, 2022
 */

#include <stdio.h>              /* Input/Output */
#include <stdlib.h>             /* General Utilities */
#include <unistd.h>             /* Symbolic Constants */
#include <pthread.h>			/* Pthread Library */
#include <string.h>             /* String Library */
#include <time.h>
#include <sys/time.h>

// Default dimensions of the board 
#define DWIDTH	80
#define DHEIGHT	25

// Define renew time in us
#define RTIME	500000 

/* ------------------Begin - Global Variables------------------ */

// Boards dimension values
unsigned int width;
unsigned int height;

// Number of threads
unsigned int nthreads;

// Array for thread ids
int *t_ids;

// Barrier variable
pthread_barrier_t barr;

// Program has two arrays in order to save each state
// The needed memory is allocated in main due to dimension values
int **array1;
int **array2;

// Pointers to handle arrays (current and next states, and temp for swap).
int **curptr, **nextptr, **temp;

// Flag to specify the bench mode
unsigned int bench_flag;

// Flag to specify default dimension mode
unsigned int dflag;

// Input filename
char *filename;

// Struct variables to measure execution time
struct timeval t_start, t_end;

/* ------------------End - Global Variables------------------ */

/* ---------------------Begin - Functions--------------------- */

// Init board with zeroes if default mode, or with random 0,1.
void initialize_board (int **current, unsigned int dflag);

// Read the initial state of the board from a file in default mode
void read_file (int **current, char *file_name);

/* 
Because the bounds remain unchanged and are not taken care
in main procceding, we copy the region to the next state array.
*/ 
void copy_region (int **current, int **next);

// Function to return the number of alive neighbors of the i,j cell.
int adjacent_to (int **current, int i, int j);

/* 
Main play function for the game of life.
Takes as input two pointers for the arrays
and the segment in which to apply the game. 
*/
void play (int **current, int **next, int start, int finish);

// Threads' entry function (calculate raw bounds and play game).
void *entry_function(void *ptr);

// Print the state of the board to standard output
void print (int **current);

// Argument check
int arg_check (int argc, char *argv[]);

// Print call usage
void print_help (void);

/* ---------------------End - Functions--------------------- */

/* -----------------------Main Program---------------------- */
int main (int argc, char *argv[])
{	
	int i;
	
	// Init flags and variables
	dflag=1;
	bench_flag=0;
	height=0;
	width=0;
	filename=NULL;

	// Default value for number of threads is 1
	nthreads=1;

	// Call argument check
	if(arg_check(argc, argv)!=0)
		return -1;

	// Allocate the two arrays due to the given dimensions (default|user specified)
	array1 = (int **)malloc(width*sizeof(int *));
	if(array1==NULL)
	{
		printf("Problem in memory allocation of array1!\n");
		printf("Program exited.\n");
		return -1;
	}
	for(i = 0; i < width; i++)
	{
		array1[i] = (int *)malloc(height*sizeof(int));
		if(array1[i]==NULL)
		{
			printf("Problem in memory allocation of array1!\n");
			printf("Program exited.\n");
			return -1;
		}
	}

	array2 = (int **)malloc(width*sizeof(int *));
        if(array2==NULL)
        {
                printf("Problem in memory allocation of array2!\n");
                printf("Program exited.\n");
                return -1;
        }
        for(i = 0; i < width; i++)
        {       
                array2[i] = (int *)malloc(height*sizeof(int));
                if(array2[i]==NULL)
                {
                        printf("Problem in memory allocation of array2!\n");
                        printf("Program exited.\n");
                        return -1;
                }
        }

	// Init pointers to show to the relevant arrays
	curptr=array1;
	nextptr=array2;

	// Init current array with zeroes or random values
	// Attribute specified by default flag
	initialize_board(curptr, dflag);
	
	// If we have default dimensions read input state from file
	if(dflag)
		read_file (curptr, filename);
	else if(!dflag && !bench_flag)
	{
	//In case of play mode with user input dimensions
	//check if they are too big to print in console
		if((width > 100) || (height > 50))
		{
			printf("WARNING!!!! Too large board to print in screen.\n");
			printf("Maximum allowed dimensions is 100 x 50.\n");
			printf("Bigger values are allowed only in bench mode.\n");
			printf("Program exited!\n\n");
			return -1;
		}
	}

	// Copy the unchanged region cells
	copy_region(curptr, nextptr);
	
	// Create an array with threads given the input number
	pthread_t thr[nthreads];

	// Allocate memory for the thread ids
	t_ids = malloc(nthreads * sizeof(int));
	if(t_ids==NULL)
	{
		printf("Problem in memory allocation of ids array!\n");
		printf("Program exited.\n");
		return -1;
	}

	// Barrier initialization
    	if(pthread_barrier_init(&barr, NULL, nthreads))
    	{
        	printf("Could not create a barrier\n");
        	return -1;
    	}

	// Relevant messages in case of bench mode
	if(bench_flag)
	{	
		printf("==================Bench mode==================\n");
		printf("----------Waiting for status report-----------\n\n");
		gettimeofday(&t_start, NULL);
	}	

	// Create the threads
	for(i = 0; i < nthreads; i++)
    	{
		t_ids[i]=i;
        	if(pthread_create(&thr[i], NULL, &entry_function, (void *)&t_ids[i]))
        	{
            		printf("Could not create thread %d\n", i);
            		return -1;
        	}
    	}

	// Proccess's master thread waits for the execution of all threads
	for(i = 0; i < nthreads; i++)
    	{
        	if(pthread_join(thr[i], NULL))
        	{
            		printf("Could not join thread %d\n", i);
            		return -1;
        	}
    	}

	//Print stat in case of bench mode
	if(bench_flag)
	{
		gettimeofday(&t_end, NULL);
		printf("-----------Stats-----------\n");
		printf("Array Dimenstions  : %d x %d (Raws X Cols)\n", height, width);
		printf("Threads Number    : %d\n",nthreads);
		printf("Execution time(us): ");
		printf("%ld\n\n", ((t_end.tv_sec * 1000000 + t_end.tv_usec) - (t_start.tv_sec * 1000000 + t_start.tv_usec)));
		printf("===============End of bench mode==============\n");
	}

	return 0;
}
/* ----------------------End of Main Program--------------------- */

/* ------------------------Functions Code----------------------- */

void initialize_board (int **curptr, unsigned int dflag)
{
//If default flag init with zeroes, else with random 0,1 values
	int i, j;

	if(dflag)
	{
		for (i=0; i<width; i++) for (j=0; j<height; j++) 
			curptr[i][j] = 0;
	}
	else
	{
		for (i=0; i<width; i++) for (j=0; j<height; j++) 
			curptr[i][j] = rand() % 2;
	}
}

void read_file (int **curptr, char *name) 
{
//If default mode read initial state from file
	FILE	*f;
	int	i, j;
	char	s[100];

	f = fopen (name, "r");
	for (j=0; j<height; j++) 
	{

		fgets (s, 100, f);

		for (i=0; i<width; i++) 
		{
			curptr[i][j] = s[i] == 'x';
		}
	}
	fclose (f);
}//End of read_file()

void copy_region (int **curptr, int **nextptr)
{
	int i,j;
	for(j=0; j<height; j++)
		for(i=0; i<width; i++)
			if((i==0)|(j==0)|(j==height-1)|(i==width-1))
				nextptr[i][j]=curptr[i][j];
}

int adjacent_to (int **curptr, int i, int j) 
{
	int row, col, count;

	count = 0;

	// Examine all the neighbors
	for (row=-1; row<=1; row++) 
		for (col=-1; col<=1; col++)
		{
			// exclude current cell from count
			if (row || col)
				if (curptr[i+row][j+col]) count++;		
			// we don't need to keep counting if the number is >3 (no change in behaviour)		
			if(count>3)
			{//break nested loops
				break;
				break;
			}
		}		
	return count;
}//End of adjacent_to()

void play (int **curptr, int **nextptr, int start, int finish) 
{
	int i, j, alive;

	// Exclude board region and apply for each cell the game's rules
	for (i=1; i<width-1; i++) for (j=start; j<finish; j++) 
	{
		alive = adjacent_to (curptr, i, j);
		if (alive == 2) nextptr[i][j] = curptr[i][j];
		if (alive == 3) nextptr[i][j] = 1;
		if (alive < 2) nextptr[i][j] = 0;
		if (alive > 3) nextptr[i][j] = 0;
	}
}//End of play()

void print (int **curptr) 
{
	int i, j;

	for (j=0; j<height; j++) 
	{

		for (i=0; i<width; i++) 
		{
			printf ("%c", curptr[i][j] ? 'x' : ' ');
		}
		printf ("\n");
	}
}//End of print()

void *entry_function(void *t_id)
{
	int *thread_id=(int*)t_id;

	// Calculate the array bounds that each thread will process
	int bound = height / nthreads;
	int start = *thread_id * bound;
	int finish = start + bound;

	int i,bn;

	// exclude extern cells
	if(*thread_id==0) start++;
	if(*thread_id==nthreads-1) finish=height-1;

	// Play the game for 100 rounds
	for (i=0; i<100; i++)
	{	
		play (curptr, nextptr, start, finish);
	
		/* ------ Synchronization point with barries ------
		The pthread_barrier_wait subroutine synchronizes participating threads
		 at the barrier referenced by barrier. 
		The calling thread blocks until the required number of threads have called 
		pthread_barrier_wait specifying the barrier.

		When the required number of threads have called pthread_barrier_wait specifying the barrier, 
		the constant PTHREAD_BARRIER_SERIAL_THREAD is returned to one unspecified thread 
		and 0 is returned to the remaining threads. 

		At this point, the barrier resets to the state it had as a result of 
		the most recent pthread_barrier_init function that referenced it.
		*/
    		bn = pthread_barrier_wait(&barr);
    		if(bn != 0 && bn != PTHREAD_BARRIER_SERIAL_THREAD)
    		{
        		printf("Could not wait on barrier\n");
        		exit(-1);
   		}
		
		/* 
		The thread with ID=0 is responsible to swap the pointers and
		print the board's status in each round.
		ps: The screen prints are omitted in bench mode (bench_flag)
		*/
		if(bn==PTHREAD_BARRIER_SERIAL_THREAD)
		{
			if((i==0) && (!bench_flag))
				{
					system("clear");
					print (curptr);
					printf("------------------------------");
					printf("Board's Initial State");
					printf("-----------------------------\n");
					printf("   \"Set console dimensions at least %dx%d to have a full view of the board.\"",width,height+4);
					printf("\n\t\tRenew every %d us  ",RTIME);
					printf("Hit ENTER to start play!\n");
					getchar();  
					system("clear");
				}	
				else
				{	//Swap pointers
					temp=curptr;
					curptr=nextptr;
					nextptr=temp;
					
					if(!bench_flag)	
					{
						print (curptr);
						printf("----------------------------");
						printf("Board's state in round: %d",i);
						printf("---------------------------\n");
						usleep(RTIME);
						system("clear");	
					}
				}
		}

		//One more barrier is needed in order to ensure
		//that the pointers have been swapped before go to next round
		bn = pthread_barrier_wait(&barr);
    		if(bn != 0 && bn != PTHREAD_BARRIER_SERIAL_THREAD)
    		{
        		printf("Could not wait on barrier\n");
        		exit(-1);
   		}
	}// End of 100 play rounds for loop
	return 0;
}//End of entry function

/*
In order to specify the behaviour of the program we check the passed
arguments. We make the required checks and print relevant messages.
The basic idea is that we have two modes. Play and bench modes. 
For more details read the relevant paragraph in the full report file.
*/
int arg_check(int argc, char *argv[])
{
	int opt_char;
	if(argc == 1)
	{
		print_help();
		return -1;
	}
	while ((opt_char = getopt(argc, argv, "n:h:w:f:b")) != -1)
        {
                switch(opt_char)
                {
                        case 'n':
				nthreads=atoi(optarg);
                                break;
                        case 'h':
				height=atoi(optarg);
                                break ;
                        case 'w':
				width=atoi(optarg);
                                break ;
                        case 'f':
				filename=optarg;
                                break ;
			case 'b':
				bench_flag=1;
				break ;
                        default:
                                print_help();
                                break;
                }
        }
	//In case of user given dimensions,
	//check if both of them have been given
	if(height!=0 && width==0)
	{
		printf("Give width dimension too!\n");
		printf("Or leave default.\n");
		return -1;
	}
	else if(width!=0 && height==0)
	{
		printf("Give length dimension too!\n");
		printf("Or leave default.\n");
		return -1;
	}
	// If all correct given disable default flag
	else if(width!=0 && height!=0)
	{
		dflag=0;
		if(filename!=NULL)
		{
			printf("\nWARNING!\n");
			printf("Dimensions arguments have overscale default input file\n");
			printf("Hit ENTER to continue.\n");
			getchar();
		}
	}
	// If default flag has not been disabled,
	// assign default dimension to variables
	if(dflag)
	{
		width=DWIDTH;
		height=DHEIGHT;
		
		//In default mode we need an input file for the initial state
		//So check if it has been given, otherwise exit with message
		if(filename==NULL)
		{
			printf("In default mode give input file too!\n");
			printf("Program exited!\n\n");
			return -1;
		}
	}
	return 0;
}// End of arg_check()

// Print argument usage
void print_help()
{
        printf("\nUsage:\t ./GOLPT OPTIONS\n");
	printf("\tOPTIONS:\n");
	printf("\t\t-n  Number Of Threads\n");
        printf("\t\t-h  Set Board Height(Raws)\n");
        printf("\t\t-w  Set Board Width(Columns)\n");
	printf("\t\t-f  Input File\n");
	printf("\t\t-b  Bench Mode\n\n");
}

/* ---------------------End of functions Code------------------- */
