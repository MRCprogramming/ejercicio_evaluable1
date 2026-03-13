#include <stdio.h>
#include "claves.h"

int main(int argc, char **argv)
{
    int err;

    /* ── 1. destroy: inicializar el servicio ─────────────────────────────── */
    printf("=== destroy ===\n");
    err = destroy();
    printf("destroy()          -> %d (esperado 0)\n\n", err);

    /* ── 2. set_value: insertar tuplas válidas ───────────────────────────── */
    printf("=== set_value ===\n");

    char *key1  = "clave1";
    char *val1a = "valor1_de_clave1";
    float vec1[] = {1.0, 2.0, 3.0};
    struct Paquete p1 = {10, 20, 30};

    err = set_value(key1, val1a, 3, vec1, p1);
    printf("set_value clave1   -> %d (esperado 0)\n", err);

    char *key2  = "clave2";
    char *val1b = "valor1_de_clave2";
    float vec2[] = {4.5, 5.5};
    struct Paquete p2 = {1, 2, 3};

    err = set_value(key2, val1b, 2, vec2, p2);
    printf("set_value clave2   -> %d (esperado 0)\n", err);

    /* clave duplicada: debe devolver -1 */
    err = set_value(key1, val1a, 3, vec1, p1);
    printf("set_value duplicada -> %d (esperado -1)\n", err);

    /* N_value2 fuera de rango (0 y 33): deben devolver -1 */
    err = set_value("clave_rango", "v1", 0, vec1, p1);
    printf("set_value N=0      -> %d (esperado -1)\n", err);
    err = set_value("clave_rango", "v1", 33, vec1, p1);
    printf("set_value N=33     -> %d (esperado -1)\n\n", err);

    /* ── 3. exist ────────────────────────────────────────────────────────── */
    printf("=== exist ===\n");
    err = exist(key1);
    printf("exist clave1       -> %d (esperado 1)\n", err);
    err = exist("no_existe");
    printf("exist no_existe    -> %d (esperado 0)\n\n", err);

    /* ── 4. get_value ────────────────────────────────────────────────────── */
    printf("=== get_value ===\n");

    char   out_v1[256];
    int    out_n;
    float  out_vec[32];
    struct Paquete out_p;

    err = get_value(key1, out_v1, &out_n, out_vec, &out_p);
    printf("get_value clave1   -> %d (esperado 0)\n", err);
    if (err == 0) {
        printf("  value1=%s\n", out_v1);
        printf("  N_value2=%d\n", out_n);
        printf("  V_value2=");
        for (int i = 0; i < out_n; i++) printf("%.2f ", out_vec[i]);
        printf("\n");
        printf("  value3=(%d,%d,%d)\n", out_p.x, out_p.y, out_p.z);
    }

    err = get_value("no_existe", out_v1, &out_n, out_vec, &out_p);
    printf("get_value no_existe -> %d (esperado -1)\n\n", err);

    /* ── 5. modify_value ─────────────────────────────────────────────────── */
    printf("=== modify_value ===\n");

    char *nuevo_v1 = "valor1_modificado";
    float nuevo_vec[] = {9.9, 8.8, 7.7, 6.6};
    struct Paquete nuevo_p = {100, 200, 300};

    err = modify_value(key1, nuevo_v1, 4, nuevo_vec, nuevo_p);
    printf("modify_value clave1 -> %d (esperado 0)\n", err);

    err = get_value(key1, out_v1, &out_n, out_vec, &out_p);
    if (err == 0) {
        printf("  value1=%s\n", out_v1);
        printf("  N_value2=%d\n", out_n);
        printf("  V_value2=");
        for (int i = 0; i < out_n; i++) printf("%.2f ", out_vec[i]);
        printf("\n");
        printf("  value3=(%d,%d,%d)\n", out_p.x, out_p.y, out_p.z);
    }

    /* modificar clave que no existe */
    err = modify_value("no_existe", nuevo_v1, 4, nuevo_vec, nuevo_p);
    printf("modify_value no_existe -> %d (esperado -1)\n", err);

    /* N_value2 fuera de rango */
    err = modify_value(key1, nuevo_v1, 0, nuevo_vec, nuevo_p);
    printf("modify_value N=0   -> %d (esperado -1)\n\n", err);

    /* ── 6. delete_key ───────────────────────────────────────────────────── */
    printf("=== delete_key ===\n");

    err = delete_key(key2);
    printf("delete_key clave2  -> %d (esperado 0)\n", err);
    err = exist(key2);
    printf("exist clave2 tras borrar -> %d (esperado 0)\n", err);

    err = delete_key("no_existe");
    printf("delete_key no_existe -> %d (esperado -1)\n\n", err);

    /* ── 7. destroy: borrar todo ─────────────────────────────────────────── */
    printf("=== destroy final ===\n");
    err = destroy();
    printf("destroy()          -> %d (esperado 0)\n", err);
    err = exist(key1);
    printf("exist clave1 tras destroy -> %d (esperado 0)\n", err);

    return 0;
}
