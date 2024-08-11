#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>


//Defenição dos Buffers
#define BUFF_CIRCULAR_SIZE 10
#define BUFF_FINAL_CRESCENTE_SIZE 30000

//Criação dos buffers
int buffer_circular[BUFF_CIRCULAR_SIZE];
int buffer_final_crescente[BUFF_FINAL_CRESCENTE_SIZE];

int nsignals=0;

//Estrutrua a ser utlizada para a threads Produtoras
struct
{
	
	pthread_mutex_t lock; // Indicação da thread que vao estar bloqueadas
	
	pthread_cond_t cond; // Variavel auxiliar que indica se pode continuar a produzir ou não

	int IndexProd; //Variavel auxiliar correspondente ao indice das produtoras

	int ValProd; //Variavel auxiliar correspondente ao index das produtoras
		
} put = {PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER};



//Estrutrua a ser utlizada para a threads Consumidoras
struct
{
	
	pthread_mutex_t lock; // Indicação da thread que vao estar bloqueadas
	fi
	pthread_cond_t cond; // Variavel auxiliar que indica se pode continuar a produzir ou não
			
	int pronta; // Variavel auxiliar indica o numero de indices no buffer_circular que estao a ser utilizados
	
	int IndexOut; //Variavel auxiliar indica os indices do buffer_final_crescente
	
	int IndexCirc; //Variavel que indica os indices do buffer_circular
	
} get = {PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER};



//funções das Threads consumidoras e produtoras
void *producer(void *), *consumer(void *);

int main(){
	//inicializaçao das variaveis
	get.IndexOut=30000;
	get.IndexCirc=0;
	put.IndexProd=0;
	put.ValProd=40000;
	get.pronta=0;
	int i;
	
	//Inicialização do buffer_final_crescente com todos os indices a 0
	for(i=0; i<BUFF_FINAL_CRESCENTE_SIZE;i++)
	buffer_final_crescente[i]=0;

	//Criação das threads
	pthread_t PM_T1, PM_T2, CM_T1, CM_T2, CM_T3;
	//inicializaçao de thread produtora1
 	pthread_create(&PM_T1, NULL, producer, NULL); 
	//inicializaçao de thread produtora2
    	pthread_create(&PM_T2, NULL, producer, NULL);
	//inicializaçao de thread consumidora1
    	pthread_create(&CM_T1, NULL, consumer, NULL); 
	//inicializaçao de thread consumidora2
    	pthread_create(&CM_T2, NULL, consumer, NULL); 
	//inicializaçao de thread consumidora3
    	pthread_create(&CM_T3, NULL, consumer, NULL); 
	
	//esperar pelas threads para a execução
        pthread_join(PM_T1, NULL);
        pthread_join(PM_T2, NULL);
        pthread_join(CM_T1, NULL);
        pthread_join(CM_T2, NULL);
        pthread_join(CM_T3, NULL);
    
	printf("nsignals = %d", nsignals);
	
	
 return 0;
}


void *producer(void *arg)
{
	//Condição que faz a execução das produtoras
	while (put.ValProd > 10000){
	//coloca o processador em "lock" para as produtoras
	pthread_mutex_lock (&put.lock);
	//condiçao que coloca a thread a dormir
	while(get.pronta >= BUFF_CIRCULAR_SIZE){
        pthread_cond_wait(&put.cond, &put.lock);
		printf("A thread produtora adormece p>BUFFCirc\n");
    }	
	//condicao que verifica se as produtoras têm elementos
	if(put.ValProd<=10000)
		{
			pthread_cond_signal(&put.cond);
			pthread_mutex_unlock(&put.lock); //funçao que quando a thread produtora estiver ativa, bloquea-la e termina o programa
			printf("A thread produtora acorda valprod<10001\n");
			break;
		}
		
		//Inserção das threads produtoras no buffer_circular
		buffer_circular[put.IndexProd] = put.ValProd-1;
		
		put.IndexProd=(put.IndexProd+1) % BUFF_CIRCULAR_SIZE;
		
		put.ValProd--;
		
		pthread_mutex_unlock(&put.lock);
		
		pthread_mutex_lock(&get.lock);
		
		get.pronta++;
		//condição que o processador acorda as threads consumidoras caso haja valores no buffer_circular
		if(get.pronta > 0){
			pthread_cond_signal(&get.cond);
			nsignals++;
			printf("A thread consumidora acorda p>0\n");
		}
		//coloca o processador em "unlock" para as produtoras
		pthread_mutex_unlock(&get.lock);
		
		*((int *) arg) += 1;
}
return (NULL);
}
//Criação da funcao consumer  

void *consumer(void *arg)
{
	//Ciclo infinito para as consumidoras
	for( ; ; ){
		//Verifica se o buffer circular tem elementos
		if(get.pronta <= 0 && put.ValProd < 10001){
			pthread_mutex_unlock(&get.lock);
			pthread_cond_signal(&get.cond);
			printf("A thread consumidora acorda p=0 e valprod<10001\n");
			break;
		}
		//coloca o processador em "lock" para as consumidoras
			pthread_mutex_lock(&get.lock);
			
		//Adormece threads consumidoras se não existirem valores no buffer_circular
		while(get.pronta <= 0 && put.ValProd >= 10001){
            pthread_cond_wait(&get.cond, &get.lock);
			printf("A thread consumidora acormece p=0 e valprod > 10000\n");
        }

		//Inserção das threads consumidoras no buffer_final_crescente
		buffer_final_crescente[get.IndexOut] = buffer_circular[get.IndexCirc]-10000;
		buffer_circular[get.IndexCirc]=NULL;
		get.IndexCirc = (get.IndexCirc + 1) % BUFF_CIRCULAR_SIZE;
		get.IndexOut--;
		get.pronta--;

		//acorda threads produtoras caso já exista "espaço" para colocarem os valores
        if(get.pronta < BUFF_CIRCULAR_SIZE){
            pthread_cond_signal(&put.cond);
            nsignals++;
			printf("A thread produtora acorda p<BuffCirc\n");
        }
		//coloca o processador em "unlock" para as consumidoras
        pthread_mutex_unlock(&get.lock);

	    *((int *) arg) += 1;
	}
}
