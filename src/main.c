#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "sincronizacao.h"

/* Contador global de pedidos para gerar IDs únicos */
static int contador_pedidos = 0;
static pthread_mutex_t mutex_contador = PTHREAD_MUTEX_INITIALIZER;

/* Gera um ID único para cada pedido */
static int proximo_id_pedido(void) {
    int id;
    pthread_mutex_lock(&mutex_contador);
    id = ++contador_pedidos;
    pthread_mutex_unlock(&mutex_contador);
    return id;
}

/* Função executada por cada thread garçom (produtor) */
static void *thread_garcom(void *arg) {
    DadosGarcom *dados = (DadosGarcom *)arg;
    SistemaRestaurante *sis = dados->sistema;

    registrar_log(sis, "Garçom %d iniciou (mesas %d a %d)",
                  dados->id_garcom, dados->mesa_inicio, dados->mesa_fim);

    for (int mesa = dados->mesa_inicio; mesa <= dados->mesa_fim; mesa++) {
        for (int p = 0; p < NUM_PEDIDOS_POR_MESA; p++) {
            Pedido pedido;
            pedido.id_pedido = proximo_id_pedido();
            pedido.id_mesa = mesa;
            clock_gettime(CLOCK_REALTIME, &pedido.hora_criacao);

            registrar_log(sis,
                "Garçom %d: coletou pedido #%d da mesa %d",
                dados->id_garcom, pedido.id_pedido, mesa);

            enfileirar_pedido(&sis->fila, pedido);

            registrar_log(sis,
                "Garçom %d: colocou pedido #%d na fila",
                dados->id_garcom, pedido.id_pedido);

            /* Pequena pausa entre pedidos para simular coleta */
            usleep(100000 + (rand() % 200000));
        }
    }

    registrar_log(sis, "Garçom %d encerrou o turno", dados->id_garcom);
    return NULL;
}

/* Função executada por cada thread cozinheiro (consumidor) */
static void *thread_cozinheiro(void *arg) {
    DadosCozinheiro *dados = (DadosCozinheiro *)arg;
    SistemaRestaurante *sis = dados->sistema;

    registrar_log(sis, "Cozinheiro %d iniciou", dados->id_cozinheiro);

    while (1) {
        Pedido pedido = desenfileirar_pedido(&sis->fila);

        /* Pedido sentinela indica fim da simulação */
        if (pedido.id_pedido == -1) {
            registrar_log(sis, "Cozinheiro %d recebeu sinal de encerramento",
                          dados->id_cozinheiro);
            break;
        }

        /* Calcula o tempo de espera do pedido */
        struct timespec agora;
        clock_gettime(CLOCK_REALTIME, &agora);
        double tempo_espera = calcular_tempo_decorrido(pedido.hora_criacao, agora);

        registrar_log(sis,
            "Cozinheiro %d: pegou pedido #%d (mesa %d, espera: %.3fs)",
            dados->id_cozinheiro, pedido.id_pedido, pedido.id_mesa,
            tempo_espera);

        /* Adquire um fogão (semáforo contador) */
        sem_wait(sis->sem_fogoes);

        registrar_log(sis,
            "Cozinheiro %d: usando fogão para preparar pedido #%d",
            dados->id_cozinheiro, pedido.id_pedido);

        /* Simula o tempo de preparo */
        sleep(TEMPO_PREPARO);

        /* Libera o fogão */
        sem_post(sis->sem_fogoes);

        registrar_log(sis,
            "Cozinheiro %d: pedido #%d pronto! (mesa %d)",
            dados->id_cozinheiro, pedido.id_pedido, pedido.id_mesa);

        /* Atualiza estatísticas globais */
        atualizar_estatisticas(&sis->stats, tempo_espera);
    }

    registrar_log(sis, "Cozinheiro %d encerrou", dados->id_cozinheiro);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    SistemaRestaurante sistema;
    inicializar_sistema(&sistema);

    printf("========================================\n");
    printf("    RESTAURANTE CONCORRENTE\n");
    printf("========================================\n");
    printf("Cozinheiros: %d\n", NUM_COZINHEIROS);
    printf("Fogões:      %d\n", NUM_FOGOES);
    printf("Mesas:       %d\n", NUM_MESAS);
    printf("Garçons:     %d\n", NUM_GARCONS);
    printf("Tempo de preparo: %d segundo(s)\n", TEMPO_PREPARO);
    printf("Pedidos por mesa: %d\n", NUM_PEDIDOS_POR_MESA);
    printf("Total de pedidos: %d\n", NUM_MESAS * NUM_PEDIDOS_POR_MESA);
    printf("========================================\n\n");

    /* Marca o início da execução */
    clock_gettime(CLOCK_REALTIME, &sistema.stats.inicio_execucao);

    /* Cria as threads dos garçons */
    pthread_t threads_garcons[NUM_GARCONS];
    DadosGarcom dados_garcons[NUM_GARCONS];
    int mesas_por_garcom = NUM_MESAS / NUM_GARCONS;

    for (int i = 0; i < NUM_GARCONS; i++) {
        dados_garcons[i].id_garcom = i;
        dados_garcons[i].mesa_inicio = i * mesas_por_garcom;
        dados_garcons[i].mesa_fim = (i + 1) * mesas_por_garcom - 1;
        dados_garcons[i].sistema = &sistema;

        if (pthread_create(&threads_garcons[i], NULL,
                           thread_garcom, &dados_garcons[i]) != 0) {
            perror("Erro ao criar thread garçom");
            exit(EXIT_FAILURE);
        }
    }

    /* Cria as threads dos cozinheiros */
    pthread_t threads_cozinheiros[NUM_COZINHEIROS];
    DadosCozinheiro dados_cozinheiros[NUM_COZINHEIROS];

    for (int i = 0; i < NUM_COZINHEIROS; i++) {
        dados_cozinheiros[i].id_cozinheiro = i;
        dados_cozinheiros[i].sistema = &sistema;

        if (pthread_create(&threads_cozinheiros[i], NULL,
                           thread_cozinheiro, &dados_cozinheiros[i]) != 0) {
            perror("Erro ao criar thread cozinheiro");
            exit(EXIT_FAILURE);
        }
    }

    /* Aguarda todos os garçons terminarem */
    for (int i = 0; i < NUM_GARCONS; i++) {
        pthread_join(threads_garcons[i], NULL);
    }

    registrar_log(&sistema, "Todos os garçons terminaram. Enviando sinais de encerramento...");

    /* Envia pedidos sentinela para cada cozinheiro encerrar */
    for (int i = 0; i < NUM_COZINHEIROS; i++) {
        Pedido sentinela;
        sentinela.id_pedido = -1;
        sentinela.id_mesa = -1;
        enfileirar_pedido(&sistema.fila, sentinela);
    }

    /* Aguarda todos os cozinheiros terminarem */
    for (int i = 0; i < NUM_COZINHEIROS; i++) {
        pthread_join(threads_cozinheiros[i], NULL);
    }

    /* Marca o fim da execução e imprime estatísticas */
    clock_gettime(CLOCK_REALTIME, &sistema.stats.fim_execucao);
    imprimir_estatisticas(&sistema.stats);

    /* Limpeza de recursos */
    destruir_sistema(&sistema);
    pthread_mutex_destroy(&mutex_contador);

    printf("\nSimulação encerrada com sucesso.\n");
    return 0;
}
