#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TOLERANCIA 1e-6

/* --- 1. MODULO: COMPONENTES --- */
typedef enum { RESISTENCIA, FUENTE_V } TipoComponente;

typedef struct {
    char nombre[16];
    TipoComponente tipo;
    int nodo_a, nodo_b;
    double valor;
    double voltaje, corriente, potencia;
} Componente;

typedef struct {
    Componente *comp;
    int num_comp, capacidad;
    int num_nodos;
    int num_fuentes;
    double *v_nodo;
} Circuito;

void circuito_crear(Circuito *c) {
    c->capacidad = 10;
    c->num_comp = 0;
    c->num_nodos = 0;
    c->num_fuentes = 0;
    c->comp = malloc(c->capacidad * sizeof(Componente));
    c->v_nodo = NULL;
}

void circuito_agregar(Circuito *c, Componente nuevo) {
    if (c->num_comp >= c->capacidad) {
        c->capacidad *= 2;
        c->comp = realloc(c->comp, c->capacidad * sizeof(Componente));
    }
    c->comp[c->num_comp++] = nuevo;
    if (nuevo.nodo_a > c->num_nodos) c->num_nodos = nuevo.nodo_a;
    if (nuevo.nodo_b > c->num_nodos) c->num_nodos = nuevo.nodo_b;
    if (nuevo.tipo == FUENTE_V) c->num_fuentes++;
}

void circuito_liberar(Circuito *c) {
    free(c->comp);
    free(c->v_nodo);
}

/* --- 2. MODULO: LECTOR_CIRCUITO --- */
int leer_circuito(const char *ruta, Circuito *c) {
    FILE *f = fopen(ruta, "r");
    if (!f) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'.\n", ruta);
        return 0;
    }

    char linea[256];
    int num_linea = 0;
    while (fgets(linea, sizeof(linea), f)) {
        num_linea++;
        char *p = linea;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0') continue;

        char tipo_c, nombre[16];
        int na, nb;
        double val;

        if (sscanf(p, "%c %15s %d %d %lf", &tipo_c, nombre, &na, &nb, &val) != 5) {
            fprintf(stderr, "Error en linea %d: Formato invalido -> %s", num_linea, linea);
            fclose(f);
            return 0;
        }

        Componente comp = {0};
        strncpy(comp.nombre, nombre, 15);
        comp.nodo_a = na;
        comp.nodo_b = nb;
        comp.valor = val;

        if (tipo_c == 'R' || tipo_c == 'r') {
            comp.tipo = RESISTENCIA;
        } else if (tipo_c == 'V' || tipo_c == 'v') {
            comp.tipo = FUENTE_V;
        } else {
            fprintf(stderr, "Error en linea %d: Tipo '%c' desconocido.\n", num_linea, tipo_c);
            fclose(f);
            return 0;
        }
        circuito_agregar(c, comp);
    }
    fclose(f);
    return 1;
}

/* --- 3. MODULO: SISTEMA_LINEAL --- */
int resolver_gauss(double **A, double *b, double *x, int N) {
    for (int k = 0; k < N; k++) {
        int max_row = k;
        double max_val = fabs(A[k][k]);
        for (int i = k + 1; i < N; i++) {
            if (fabs(A[i][k]) > max_val) {
                max_val = fabs(A[i][k]);
                max_row = i;
            }
        }
        if (max_val < 1e-12) return 0; // Matriz singular

        if (max_row != k) {
            double *tmp_row = A[k]; A[k] = A[max_row]; A[max_row] = tmp_row;
            double tmp_b = b[k]; b[k] = b[max_row]; b[max_row] = tmp_b;
        }

        for (int i = k + 1; i < N; i++) {
            double factor = A[i][k] / A[k][k];
            for (int j = k; j < N; j++) A[i][j] -= factor * A[k][j];
            b[i] -= factor * b[k];
        }
    }

    for (int i = N - 1; i >= 0; i--) {
        double suma = b[i];
        for (int j = i + 1; j < N; j++) suma -= A[i][j] * x[j];
        x[i] = suma / A[i][i];
    }
    return 1;
}

/* --- 4. MODULO: ANALISIS_NODAL --- */
int validar_circuito(Circuito *c) {
    int tiene_tierra = 0;
    for (int i = 0; i < c->num_comp; i++) {
        Componente *cp = &c->comp[i];
        if (cp->tipo == RESISTENCIA && cp->valor <= 0) {
            fprintf(stderr, "Error de validacion: %s tiene valor %.2f <= 0.\n", cp->nombre, cp->valor);
            return 0;
        }
        if (cp->nodo_a == cp->nodo_b) {
            fprintf(stderr, "Error de validacion: %s tiene ambos terminales en el nodo %d.\n", cp->nombre, cp->nodo_a);
            return 0;
        }
        if (cp->nodo_a == 0 || cp->nodo_b == 0) tiene_tierra = 1;
        for (int j = i + 1; j < c->num_comp; j++) {
            if (strcmp(cp->nombre, c->comp[j].nombre) == 0) {
                fprintf(stderr, "Error de validacion: Nombre duplicado '%s'.\n", cp->nombre);
                return 0;
            }
        }
    }
    if (!tiene_tierra) {
        fprintf(stderr, "Error de validacion: No existe conexion al nodo de referencia (0).\n");
        return 0;
    }
    return 1;
}

int analizar_circuito(Circuito *c) {
    int n = c->num_nodos;
    int k = c->num_fuentes;
    int N = n + k;

    double **A = malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) A[i] = calloc(N, sizeof(double));
    double *b = calloc(N, sizeof(double));
    double *x = calloc(N, sizeof(double));

    int idx_fuente = 0;
    for (int i = 0; i < c->num_comp; i++) {
        Componente *cp = &c->comp[i];
        int na = cp->nodo_a;
        int nb = cp->nodo_b;

        if (cp->tipo == RESISTENCIA) {
            double g = 1.0 / cp->valor;
            if (na > 0) A[na - 1][na - 1] += g;
            if (nb > 0) A[nb - 1][nb - 1] += g;
            if (na > 0 && nb > 0) {
                A[na - 1][nb - 1] -= g;
                A[nb - 1][na - 1] -= g;
            }
        } else if (cp->tipo == FUENTE_V) {
            int f_idx = n + idx_fuente;
            if (na > 0) { A[na - 1][f_idx] -= 1.0; A[f_idx][na - 1] += 1.0; }
            if (nb > 0) { A[nb - 1][f_idx] += 1.0; A[f_idx][nb - 1] -= 1.0; }
            b[f_idx] = cp->valor;
            idx_fuente++;
        }
    }

    int exito = resolver_gauss(A, b, x, N);
    if (exito) {
        c->v_nodo = calloc(n + 1, sizeof(double));
        for (int i = 1; i <= n; i++) c->v_nodo[i] = x[i - 1];

        idx_fuente = 0;
        for (int i = 0; i < c->num_comp; i++) {
            Componente *cp = &c->comp[i];
            double va = c->v_nodo[cp->nodo_a];
            double vb = c->v_nodo[cp->nodo_b];
            cp->voltaje = va - vb;

            if (cp->tipo == RESISTENCIA) {
                cp->corriente = cp->voltaje / cp->valor;
                cp->potencia = cp->corriente * cp->corriente * cp->valor;
            } else {
                cp->corriente = x[n + idx_fuente];
                cp->potencia = cp->valor * cp->corriente;
                idx_fuente++;
            }
        }
    } else {
        fprintf(stderr, "Error matematico: El sistema es singular (sin solucion unica).\n");
    }

    for (int i = 0; i < N; i++) free(A[i]);
    free(A); free(b); free(x);
    return exito;
}

/* --- 5. MODULO: VERIFICACION --- */
void verificar_leyes(Circuito *c, double *max_res_lck, double *diff_potencia) {
    *max_res_lck = 0.0;
    for (int nodo = 0; nodo <= c->num_nodos; nodo++) {
        double suma_i = 0.0;
        for (int i = 0; i < c->num_comp; i++) {
            Componente *cp = &c->comp[i];
            if (cp->tipo == RESISTENCIA) {
                if (cp->nodo_a == nodo) suma_i += cp->corriente;
                if (cp->nodo_b == nodo) suma_i -= cp->corriente;
            } else {
                // La corriente de fuente sale del terminal positivo (nodo_a)
                if (cp->nodo_a == nodo) suma_i -= cp->corriente;
                if (cp->nodo_b == nodo) suma_i += cp->corriente;
            }
        }
        if (fabs(suma_i) > *max_res_lck) *max_res_lck = fabs(suma_i);
    }

    double p_sum = 0.0, p_dis = 0.0;
    for (int i = 0; i < c->num_comp; i++) {
        if (c->comp[i].tipo == RESISTENCIA) p_dis += c->comp[i].potencia;
        else p_sum += c->comp[i].potencia;
    }
    *diff_potencia = fabs(p_sum - p_dis);
}

/* --- 6. MODULO: REPORTE --- */
void mostrar_resultados(Circuito *c, double res_lck, double diff_pot) {
    printf("\n=== 1. VOLTAJES NODALES ===\n");
    for (int i = 0; i <= c->num_nodos; i++) {
        printf("  Nodo %d : %10.4f V %s\n", i, c->v_nodo[i], (i == 0) ? "(Referencia)" : "");
    }

    printf("\n=== 2. RESULTADOS POR COMPONENTE ===\n");
    printf("%-8s | %-4s | %-7s | %-10s | %-10s | %-10s | %-10s\n",
           "Nombre", "Tipo", "Nodos", "Voltaje(V)", "Corr.(A)", "Pot.(W)", "Estado");
    printf("----------------------------------------------------------------------------\n");

    for (int i = 0; i < c->num_comp; i++) {
        Componente *cp = &c->comp[i];
        const char *estado = (cp->tipo == RESISTENCIA) ? "Disipa" :
                             (cp->potencia >= 0 ? "Suministra" : "Absorbe");
        printf("%-8s | %-4c | %2d -> %-2d | %10.4f | %10.4f | %10.4f | %-10s\n",
               cp->nombre, (cp->tipo == RESISTENCIA ? 'R' : 'V'),
               cp->nodo_a, cp->nodo_b, cp->voltaje, cp->corriente, fabs(cp->potencia), estado);
    }

    printf("\n=== 3. VERIFICACION DE KIRCHHOFF Y ENERGIA ===\n");
    printf("  Residuo max. LCK     : %.3e A  [%s]\n", res_lck, (res_lck < TOLERANCIA) ? "CUMPLE" : "NO CUMPLE");
    printf("  Balance de Potencia  : %.3e W  [%s]\n", diff_pot, (diff_pot < TOLERANCIA) ? "CUMPLE" : "NO CUMPLE");

    printf("\n=== 4. ESQUEMA DE RAMAS (ASCII) ===\n");
    for (int i = 0; i < c->num_comp; i++) {
        Componente *cp = &c->comp[i];
        if (cp->tipo == RESISTENCIA) {
            printf("  (%d)---[ %s = %.2f Ohm ]---(%d)   I = %.4f A\n",
                   cp->nodo_a, cp->nombre, cp->valor, cp->nodo_b, cp->corriente);
        } else {
            printf("  (%d)---( + %s = %.2f V - )---(%d)   I = %.4f A\n",
                   cp->nodo_a, cp->nombre, cp->valor, cp->nodo_b, cp->corriente);
        }
    }
}

void exportar_csv(const char *ruta_csv, Circuito *c) {
    FILE *f = fopen(ruta_csv, "w");
    if (!f) {
        fprintf(stderr, "No se pudo crear el archivo CSV.\n");
        return;
    }
    fprintf(f, "componente,tipo,nodo_a,nodo_b,valor,voltaje_V,corriente_A,potencia_W,estado\n");
    for (int i = 0; i < c->num_comp; i++) {
        Componente *cp = &c->comp[i];
        const char *estado = (cp->tipo == RESISTENCIA) ? "Disipa" :
                             (cp->potencia >= 0 ? "Suministra" : "Absorbe");
        fprintf(f, "%s,%c,%d,%d,%.4f,%.4f,%.4f,%.4f,%s\n",
                cp->nombre, (cp->tipo == RESISTENCIA ? 'R' : 'V'),
                cp->nodo_a, cp->nodo_b, cp->valor, cp->voltaje, cp->corriente, fabs(cp->potencia), estado);
    }
    fclose(f);
    printf("\n[+] Reporte exportado exitosamente a '%s'\n", ruta_csv);
}

/* --- MAIN --- */
int main(int argc, char *argv[]) {
    char ruta[256];
    if (argc >= 2) {
        strncpy(ruta, argv[1], sizeof(ruta) - 1);
        ruta[sizeof(ruta) - 1] = '\0';
    } else {
        printf("Ingrese la ruta del archivo del circuito (.txt): ");
        if (scanf("%255s", ruta) != 1) return 1;
    }

    Circuito c;
    circuito_crear(&c);

    if (leer_circuito(ruta, &c) && validar_circuito(&c)) {
        if (analizar_circuito(&c)) {
            double res_lck, diff_pot;
            verificar_leyes(&c, &res_lck, &diff_pot);
            mostrar_resultados(&c, res_lck, diff_pot);
            exportar_csv("resultados.csv", &c);
        }
    }

    circuito_liberar(&c);
    return 0;
}