#include "sincronizacao.h"
#include <stdio.h>
#include <stdlib.h>

/* Inicializa a fila de pedidos com mutex e semáforos */
void inicializar_fila(FilaPedidos *fila) {
    fila->inicio = 0;
    fila->fim = 0;
    fila->quantidade = 0;

    if (pthread_mutex_init(&fila->mutex_fila, NULL) != 0) {
        perror("Erro ao inicializar mutex_fila");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&fila->sem_vagas, 0, TAM_FILA) != 0) {
        perror("Erro ao inicializar sem_vagas");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&fila->sem_itens, 0, 0) != 0) {
        perror("Erro ao inicializar sem_itens");
        exit(EXIT_FAILURE);
    }
}

/* Destroi os recursos de sincronização da fila */
void destruir_fila(FilaPedidos *fila) {
    pthread_mutex_destroy(&fila->mutex_fila);
    sem_destroy(&fila->sem_vagas);
    sem_destroy(&fila->sem_itens);
}

/* Inicializa as estatísticas com mutex */
void inicializar_estatisticas(Estatisticas *stats) {
    stats->total_pedidos = 0;
    stats->tempo_total_espera = 0.0;

    if (pthread_mutex_init(&stats->mutex_estatisticas, NULL) != 0) {
        perror("Erro ao inicializar mutex_estatisticas");
        exit(EXIT_FAILURE);
    }
}

/* Destroi os recursos de sincronização das estatísticas */
void destruir_estatisticas(Estatisticas *stats) {
    pthread_mutex_destroy(&stats->mutex_estatisticas);
}
