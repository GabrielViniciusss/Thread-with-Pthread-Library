#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 4   //numero de threads do programa será o numero de vertices do grafo definido
pthread_mutex_t mutexas = PTHREAD_MUTEX_INITIALIZER; //inicializando mutexas (piada interna)

typedef struct args{    // cada thread realizara uma dfs partindo de um vertice especifico(v)
    int v;
    int *visited;      // variaveis necessarias para as threads realizarem a DFS em seus vertices designados
    int cycleFound;  
}ThreadsArgs;            

static int graph[4][2] ={{1,-1},{2,3},{3,-1},{0,-1}}; // -1 indica que nao ha mais vertices adjecentes
/*
    GRAFO:  0 -> 1 -> 2  .Há ciclo partindo de todos os vertices: 2 ciclos partindo de 0, 2 ciclos partindo de 1, 1 ciclo partindo de 2, 2 ciclos partindo de 3
            ^    |    |
            |    v    | 
            ---- 3 <--|
*/  
void DFS_core(int v,int* visited,int* cycleFound,int ID)
{
    visited[v] = 1; //o vertice v foi visitado pela thread atual, logo marcamos visited[v]

    // para cada vertice adjecente ao vertice v: 
    for(int i = 0;graph[v][i]!=-1/* && !*cycleFound*/;i++)  //se tiver vertices adjecentes 
    {   
        // se quiser que a thread notifique o ciclo só uma vez, caso exista, descomenta '&& !*cycleFound'
        if(!visited[graph[v][i]]) //se um vertice adjecente ao nosso vertice atual nao foi visitado ainda, aplicamos a DFS nele
            DFS_core(graph[v][i],visited,cycleFound,ID); //Recursao do DFS
        
        else //se o vertice adjacente ja foi visitado, é pq há ciclo
        {   
            //duas threads podem achar um ciclo ao mesmo tempo e com isso tentar printar essa mensagem, logo é importante colocar mutexes aq
            pthread_mutex_lock(&mutexas);
            printf("A thread que busca ciclo a partir do vertice %d encontrou um ciclo\n",ID); 
            pthread_mutex_unlock(&mutexas);
            *cycleFound = 1; //cada thread tem seu cycleFound e só escreve nele, portanto nao precisa de mutex aq.
            return;
        }
    }
}

void* DFS_start(void* args) //as threads iniciarao por essa funçao
{
    ThreadsArgs* threadArgs = (ThreadsArgs*) args;  //como na main agt casta args como (void*), temos que recastar para (ThreadsArgs*) para poder acessar os dados da struct corretamente
    DFS_core(threadArgs->v,threadArgs->visited,&threadArgs->cycleFound,threadArgs->v); //nao precisamos de mutexes aqui, pois cada thread vai acessar somente um vertice especifico (o vertice enumerado por threadArgs->v) e cada uma delas tem seu vetor visitados
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];      //declarando threads
    ThreadsArgs threadArg[NUM_THREADS]; // 4 threads, cada uma com seu contexto interno representado por threadArg

    for (int i=0;i<NUM_THREADS;i++) // inicializando contexto interno de cada thread
    {             
        threadArg[i] = (ThreadsArgs) {
            .v = i,                                  //o id da thread sera o proprio v
            .visited = (int*)calloc(4,sizeof(int)),  // 4 por que só há 4 vertices no nosso grafo escolhido
            .cycleFound = 0
        };
    }

    //a partir daqui,temos o contexto inicial de cada thread, e entao podemos criar as threads que vao operar o DFS
    int i, rc;
    for(i = 0;i<NUM_THREADS; i++)
    {  
        rc = pthread_create(&threads[i], NULL,DFS_start,(void*) &threadArg[i]); //&threadArg[i] é o ponteiro para struct de argumentos da thread[i]
        if(rc)
            exit(1); //caso de algo errado na criaçao da thread i, apenas finalizamos o processo
    }

    for(i = 0; i<NUM_THREADS; i++)         //para que a thread main espere o termino das outras threads, usamos pthread_join
        pthread_join(threads[i], NULL);  

    pthread_exit(NULL); //boa pratica para evitar o termino do processo inteiro caso a main termine antes das outras threads

   return 0;
}