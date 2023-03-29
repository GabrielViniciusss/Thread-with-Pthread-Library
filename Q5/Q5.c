#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define P 3        // vao ter 3 produtores, e 5 consumidores
#define C 5
#define MAX_BUFFER 10 //valor maximo de itens no buffer

pthread_mutex_t mutexas = PTHREAD_MUTEX_INITIALIZER;    //mutex padrao

pthread_cond_t full = PTHREAD_COND_INITIALIZER;        //variaveis de condiçao
pthread_cond_t empty = PTHREAD_COND_INITIALIZER; 

typedef struct node
{
  int value;
  struct node *next;
}Node;

typedef struct blockingQueue
{
  unsigned int sizeBuffer,statusBuffer;
  Node *head, *last;
}BlockingQueue;

BlockingQueue* Queue; //'Queue' sera nossa blocking queue global

int producedItem = 0; //variavel que vai contar quantos itens foram produzidos no total, bem como sera o valor/ID de cada item produzido

BlockingQueue* newBlockingQueue(unsigned int bufferSize)        //funçao que inicializa nossa blocking queue
{
    BlockingQueue* queue;
    queue = (BlockingQueue*) malloc(sizeof(BlockingQueue));
    queue->last=queue->head = NULL;
    queue->sizeBuffer = bufferSize;
    queue->statusBuffer = 0;
    return queue;
}

void putBlockingQueue(BlockingQueue* Q,int Value)  // funçao que coloca um nó na fila caso tenha espaço disponivel
{
    pthread_mutex_lock(&mutexas); //a fila é global e compartilhada entre todas as threads(e todas mexem nela), logo deve-se usar mutexes aqui.

    Value = producedItem; //para caso a thread mande um valor antigo(devido a um escalonamento) de producedItem para o argumento 'Value' quando chama a funçao no while

    while(Q->statusBuffer == Q->sizeBuffer) // se a fila tiver cheia, coloca-se os produtores para dormir. While por boa pratica e para evitar Spurious WakeUp
    {
        printf("Nao cabe mais nada na fila!\n");
        pthread_cond_wait(&empty, &mutexas);  //se nao tiver entrada vazia, o produtor dorme na variavel empty
    }

    Node* newNode = (Node*) malloc(sizeof(Node)); // se a fila nao tiver cheia, podemos criar um novo nó e inseri-lo
    newNode->value = Value;
    newNode->next = NULL;

    Q->statusBuffer++; // se vamos inserir, podemos já podemos incrementar statusBuffer e a quantidade de itens produzidos
    producedItem++;

    if(Q->head==NULL) // se nao tinha nada na fila antes da inserçao (head é NULL)
    {
        Q->head = newNode;
        Q->last = Q->head; 
        printf("produzi %d\n", Value);
    }
    else                          
    {
        Q->last->next=newNode;   //se ja tinha alguem, so precisamos adicionar no final da fila como se fosse uma lista encadeada
        Q->last = newNode;
        printf("produzi %d\n", Value);
    }
    
    if(Q->statusBuffer == 1)            //se so tem 1 na fila depois da inserçao,e possivel que os consumidores estejam dormindo(pois tinha 0 antes),entao os acordamos.
        pthread_cond_broadcast(&full); // consumidores dormem na variavel full, como tem algo pra ser consumido agora, acordamos os consumidores que tao dormindo

    pthread_mutex_unlock(&mutexas); 
}

int takeBlockingQueue(BlockingQueue* Q)
{
    pthread_mutex_lock(&mutexas);     //acessando regiao critica

    int removedNodeValue;             //variavel que vai receber o valor do nó que foi retirado da fila  

    while(Q->statusBuffer == 0) //se a fila estiver vazia, coloca-se os consumidores para dormir
    {
        printf("Nao ha nada na fila para consumir!!!\n");
        pthread_cond_wait(&full, &mutexas); //se nao tiver nada para consumir, o consumidor dorme na variavel full
    }

    removedNodeValue = Q->head->value; // tendo algo na fila, podemos retirar o valor do no que head aponta e o proprio nó
    Q->statusBuffer--;

    if(!Q->head->next) // se so tiver um item na fila(head.next = NULL), a fila ficara vazia
    {
        Q->last = Q->head = NULL;
        printf("consumi %d\n", removedNodeValue);
    }
    else  //caso contrario head vai apontar proximo da fila
    {
        Q->head = Q->head->next;
        printf("consumi %d\n", removedNodeValue);
    }
    
    if(Q->statusBuffer == Q->sizeBuffer - 1)  // se algum espaço foi liberado depois da fila ter ficado cheia, acordo os produtores.
        pthread_cond_broadcast(&empty);       // acordando os produtores dormindo na variavel empty

    pthread_mutex_unlock(&mutexas);

    return removedNodeValue;
}

void* producerFunction(void* Queue)
{
    BlockingQueue* Q = (BlockingQueue*) Queue; //recast
    
    while(1)                              
        putBlockingQueue(Q, producedItem);/*vai inserir item infinitamente (respeitando as regras da blockingQueue, como a limitaçao do buffer e etc)
                                            toda vez que consegue inserir, a variavel global 'producedItem' é incrementada e ela é o valor/ID de cada item*/ 

    pthread_exit(NULL); //nunca vai chegar aqui :)
}

void* consumerFunction(void* Queue)
{
    BlockingQueue* Q = (BlockingQueue*) Queue; //recast
    int k; //save do valor do item retirado, pois takeBlockingQueue retorna inteiro segundo o enunciado da questao
    while(1)
        k=takeBlockingQueue(Q);  /*vai remover o valor de head infinitas vezes(respeitando as regras da blockingQueue, como a limitaçao do buffer e etc)
                                   esse valor removido já é printado na funçao takeBlockingQueue*/

    pthread_exit(NULL); //nunca vai chegar aqui :)
}

int main()
{
    pthread_t producer[P]; //declarando threads produtoras e consumidoras respectivamente
    pthread_t consumer[C];

    Queue = newBlockingQueue(MAX_BUFFER); // criando nossa fila

    for(int i=0;i<P;i++)
        pthread_create(&producer[i], NULL, producerFunction, (void*) Queue);  //criando threads produtoras
    
    for(int i=0;i<C;i++)  
        pthread_create(&consumer[i], NULL, consumerFunction, (void *) Queue); //criando threads consumidoras
    
    //nunca vai chegar aq, pois as threads nao terminam (loop infinito)

    //desalocando memoria
    pthread_cond_destroy(&empty);
    pthread_cond_destroy(&full);

    pthread_exit(NULL);
}
