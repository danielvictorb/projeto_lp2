#include <stdio.h>
#include "sincronizacao.h"

int main(void) {
    printf("=== Restaurante Concorrente ===\n");
    printf("Cozinheiros: %d\n", NUM_COZINHEIROS);
    printf("Fogões: %d\n", NUM_FOGOES);
    printf("Mesas: %d\n", NUM_MESAS);
    printf("Garçons: %d\n", NUM_GARCONS);
    printf("Tempo de preparo: %d segundo(s)\n", TEMPO_PREPARO);
    return 0;
}
