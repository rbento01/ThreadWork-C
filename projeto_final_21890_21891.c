#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>

// declara que o buffer de produção tem só 10 posições
#define MAX_BUFFER_PROD 10
// declara que o buffer de consumo tem só 30000 posições
#define MAX_BUFFER_CONS 30000
// declara que só haverá 2 threads produtoras
#define MAX_NTHREAD_PROD 2
// declara que só haverá 3 threads consumidora
#define MAX_NTHREAD_CONS 3

// declaração do buffer circular com 10 de índice
int buffer_circular[MAX_BUFFER_PROD];
// declaração do buffer final crescente com 30000 de índice
int buffer_final_crescente[MAX_BUFFER_CONS];

// criação da estrutura que irá possuir info das threads produtoras
struct {
    pthread_mutex_t mutex; // irá controlar o possivel bloqueio e desbloqueio das threads produtoras
    pthread_cond_t cond; // condição se pode ou não continuar a produzir
    int nprod; //posicão em que se vai colocar no buffer_circular
    int nval; //valor que se vai colocar no buffer_circular
} prod = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 40000 };

// criação da estrutura que irá possuir info das threads consumidoras
struct {
    pthread_mutex_t mutex; // irá controlar o possivel bloqueio e desbloqueio das threads consumidoras
    pthread_cond_t cond; // condição se pode ou não continuar a consumir
    int nready; //Número inteiro que vai circular entre 0 e 10, caso seja 10  existem valores para se retirar do buffer_circular, caso seja 0 não haverá nenhum valor para se retirar do buffer
    int ntake; //valor que se vai copiar do buffer_circular e depois leva um decréscimo de 10000 valores
    int ncons; //índice que a thread consumidor irá possuir, retirarando do buffer_circular 
} nready = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 29999 };

// vai possuir a quantidade de sinais entre threads
int nsignals;

// inicialização das funções produce e consume
void *produce(void *), *consume(void *);

// função main
int main(){
	// criação de um iterador, de um contador para a quantidade de threads de produção e de consumo
    int i, contProd[MAX_NTHREAD_PROD], contCons[MAX_NTHREAD_CONS];

    // criação dos id's das threads de produção e de consumo
    pthread_t tid_produce[MAX_NTHREAD_PROD], tid_consume[MAX_NTHREAD_CONS];
    
    
    //criacao das threads 2 produtoras
    for(i=0; i < MAX_NTHREAD_PROD; i++){
        contProd[i] = 0;
        pthread_create(&tid_produce[i], NULL, produce, &contProd[i]);
    }

    //criacao das threads 3 consumidoras
    for(i=0; i< MAX_NTHREAD_CONS; i++){
        contCons[i] = 0;
        pthread_create(&tid_consume[i], NULL, consume, &contProd[i]);
    }

    //probir o processo "mae" de acabar antes que as threads "filhas" acabem de cumprir a sua função
    for(i=0; i< MAX_NTHREAD_CONS; i++){
        pthread_join(tid_consume[i], NULL);
        printf("Consumidora[%d] = %d\n", i, contProd[i]);
    }

    //probir o processo "mae" de acabar antes que as threads "filhas" acabem de cumprir a sua função
    for(i=0; i< MAX_NTHREAD_PROD; i++){
        pthread_join(tid_produce[i], NULL);
        printf("Produtora[%d] = %d\n", i, contProd[i]);
    }

    // vai possuir a quantidade de sinais entre threads
    printf("nsignals = %d\n", nsignals);

    //apresentação dos valores do buffer_circular
    for(i = 0; i < MAX_BUFFER_PROD; i++ ){
        printf("buffer_circular[%d] = %d\n", i, buffer_circular[i]);
    }
    
    //apresentação dos valores do buffer_final_crescente
    for(i = 29980; i < MAX_BUFFER_CONS ; i++ ){
        printf("buffer_final_crescente[%d] = %d\n", i, buffer_final_crescente[i]);
    }
    exit(0);
}

//função produzir
void *produce(void *arg){
	//enquanto os valores a inserir no buffer_circular sejam entre 10001 e 40000
    while (prod.nval <= MAX_BUFFER_CONS + 10000 && prod.nval >= 10001){
    	// atribuir o privilégio à função produce
        pthread_mutex_lock(&prod.mutex);

       	// Enquanto estejam valores disponiveis para consumo 
        while(nready.nready >= MAX_BUFFER_PROD){
        	// fazer com que as threads produtoras façam uma espera
            pthread_cond_wait(&prod.cond, &prod.mutex);
        }
       	
       	// caso o índice do buffer_circular chegue ao seu limite, começa outra vez do ínicio
       	if (prod.nprod > 9){
       		// índice volta ao seu ínicio
       		prod.nprod = 0;
       	}
        // controlo dos valores da thread produtora
        if (prod.nval <= 10000){
            // faz com que threads produtores acordem
            pthread_cond_signal(&prod.cond);
            // faz com que as threads produtoras percam o seu privilégio
            pthread_mutex_unlock(&prod.mutex);
            // sai do while
            break;
        }
        // faz-se a injeção de novos valores no buffer_circular
        buffer_circular[prod.nprod] = prod.nval;
        // mostra os valores do buffer_circular
        printf("buffer_circular[%d] => %d\n", prod.nprod, buffer_circular[prod.nprod]);
        // incrimenta o índice do buffer_circular
        prod.nprod++;
        // decrementa o valor a injetar no buffer circular
        prod.nval--;

        //faz com que as threads produtoras percam o seu privilégio
        pthread_mutex_unlock(&prod.mutex);

        // faz com que as threads nready tenham o privilégio
        pthread_mutex_lock(&nready.mutex);

        // aumenta o nready, querendo dizer que inseriu novos valores
        nready.nready++;
        
        // caso existam valores para consumir 
        if(nready.nready > 0){
        	// as threads nready acordam
            pthread_cond_signal(&nready.cond);

            // incremento da variável nsignals querendo dizer que houve uma troca de quem possuia os privilégios
            nsignals++;
        }
        // as threads nready perdem os privilégios
        pthread_mutex_unlock(&nready.mutex);

        // irá possuir o número de quantidade de vezes que as threads passaram
        *((int *) arg) += 1;            
    }
    
    return(NULL);
}

// função consumir
void *consume(void *arg){
	// ciclo infinito
    while(1){
    	// vai possuir os valores que foram inseridos no buffer circular 
        pthread_mutex_lock(&prod.mutex);
        int nval = prod.nval;
        pthread_mutex_unlock(&prod.mutex);

        //se a thread consumidora chegar aos seus intervalos as threads consumidoras perdem o seu privilégio
        if(nready.nready <= 0 && nval >= MAX_BUFFER_CONS){
            // as threads nready perdem os privilégios
            pthread_mutex_unlock(&nready.mutex);
            break;
        }

        // as threads nready ganham os privilégios
        pthread_mutex_lock(&nready.mutex);
        // caso as threads consumidoras tenham valores impossiveis
        while(nready.nready <= 0 && nval < MAX_BUFFER_CONS){
            // vai adormecer as threads consumidoras
            pthread_cond_wait(&nready.cond, &nready.mutex);
        } 

        // caso o índice do buffer final crescente chegue ao seu limite, começa outra vez do ínicio
		if (nready.ntake > 9){
			// índice volta ao seu ínicio
       		nready.ntake = 0;
       	}

        // injeta os valores no buffer final
        buffer_final_crescente[nready.ncons] = buffer_circular[nready.ntake]-10000; 
        // mostra os valores
        printf("buffer_final_crescente[%d] => %d\n", nready.ncons, buffer_final_crescente[nready.ncons]);
        // decrementa o índice
        nready.ncons--;
        // aumenta o índice
        nready.ntake++;
        // decrementa nready a dizer que retirou um dos espaços que continham valores
        nready.nready--;

        // acordam as threads produtoras
        if(nready.nready < MAX_BUFFER_PROD){
        	// faz com que as threads produtras acordem
            pthread_cond_signal(&prod.cond);
            // incremento da variável nsignals querendo dizer que houve uma troca de quem possuia os privilégios
            nsignals++;
            
        }
        // as threads consumidoras perdem os seus privilégios
        pthread_mutex_unlock(&nready.mutex);
        
    }
    return(NULL);
}