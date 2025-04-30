/**
 * AULA IoT - Embarcatech - Ricardo Prates - 002 - varredura wifi
 *
 * Material de suporte
 * 
 * https://www.raspberrypi.com/documentation/pico-sdk/networking.html#group_pico_cyw43_arch_1ga33cca1c95fc0d7512e7fef4a59fd7475 
 */

#include <stdio.h> // Biblioteca padrão para entrada e saída
#include "pico/stdlib.h" // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "pico/cyw43_arch.h" // Biblioteca para arquitetura Wi-Fi da Pico com CYW43

#include "hardware/vreg.h" // Biblioteca para controle do regulador de tensão
#include "hardware/clocks.h" // Biblioteca para manipulação dos clocks

// Define o número do pino GPIO para o LED vermelho
const uint led_pin_red = 12;

// Varredura wifi
static void verif_wifi(bool *scan_in_progress, absolute_time_t *scan_time);
// Callback para tratar os resultados da varredura Wi-Fi
static int scan_result(void *env, const cyw43_ev_scan_result_t *result);

int main() {

    stdio_init_all(); // Inicializa a entrada/saída padrão
    sleep_ms(2000); // Aguarda 2 segundos (útil para estabilizar a conexão ao iniciar)
    printf("ProgWiFiScanOK\n"); // Imprime mensagem indicando que o programa iniciou com sucesso

    gpio_init(led_pin_red); // Configura o pino do LED vermelho
    gpio_set_dir(led_pin_red, GPIO_OUT); // Define o pino do LED como saída

    // Inicializa a arquitetura CYW43 (Wi-Fi e Bluetooth)
    if (cyw43_arch_init()) { // Verifica se houve erro na inicialização
        printf("Falha ao inicializar Wi-Fi\n"); // Mensagem de erro
        return 1; // Sai do programa com código de erro
    }
    printf("Wi-Fi inicializado com sucesso\n"); // Mensagem de sucesso na inicialização
    cyw43_arch_enable_sta_mode(); // Habilita o modo estação (client) para a Pico

    absolute_time_t scan_time = make_timeout_time_ms(0); // Configura o tempo inicial para 0 (varredura imediata)
    printf("Tempo absoluto inicial: %d", scan_time); //Tempo absoluto inicial
    bool scan_in_progress = false; // Flag para indicar se há uma varredura em andamento

    while (true) { // Loop infinito para manter o programa em execução
        // Controla o LED vermelho (pisca em intervalos de 200 ms)
        gpio_put(led_pin_red, true); // Acende o LED
        sleep_ms(200); // Aguarda 200 ms
        gpio_put(led_pin_red, false); // Apaga o LED
        sleep_ms(200); // Aguarda mais 200 ms

        // Varredura wifi
        verif_wifi(&scan_in_progress, &scan_time);
        
#if PICO_CYW43_ARCH_POLL // Verifica se o modo de polling está habilitado
        cyw43_arch_poll(); // Executa a verificação manual de eventos
        cyw43_arch_wait_for_work_until(scan_time); // Aguarda até o próximo evento ou o tempo configurado
#else
        sleep_ms(1000); // Aguarda 1 segundo antes de continuar (caso o modo de polling não esteja ativo)
#endif
    }

    cyw43_arch_deinit(); // Desliga e limpa os recursos do hardware Wi-Fi
    return 0; // Retorna 0 para indicar que o programa foi finalizado sem erros
}

// ------------------------------- Funções --------------------------------------

// Varredura wifi
static void verif_wifi(bool *scan_in_progress, absolute_time_t *scan_time){
    //Verifica se o tempo configurado para iniciar a próxima varredura já passou
    if (absolute_time_diff_us(get_absolute_time(), *scan_time) < 0) {
        if (!*scan_in_progress) { // Se nenhuma varredura estiver em andamento
            cyw43_wifi_scan_options_t scan_options = {0}; // Configura opções padrão para varredura
            int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result); // Inicia a varredura
            if (err == 0) { // Verifica se a varredura foi iniciada com sucesso
                printf("\nIniciando varredura Wi-Fi....\n"); // Mensagem indicando início da varredura
                *scan_in_progress = true; // Atualiza a flag para indicar que a varredura está em andamento
            } else { // Caso ocorra um erro ao iniciar a varredura
                printf("Erro ao iniciar varredura: %d\n", err); // Imprime o erro
                *scan_time = make_timeout_time_ms(10000); // Agenda nova tentativa em 10 segundos
            }
        } else if (!cyw43_wifi_scan_active(&cyw43_state)) { // Verifica se a varredura foi concluída
            printf("Varredura concluída\n"); // Mensagem indicando que a varredura terminou
            *scan_time = make_timeout_time_ms(10000); // Configura o próximo início da varredura para 10 segundos
            *scan_in_progress = false; // Reseta a flag indicando que nenhuma varredura está em andamento
        }
    }
}

// Callback para tratar os resultados da varredura Wi-Fi
static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) { // Verifica se há um resultado válido
        // Imprime informações detalhadas da rede encontrada (SSID, RSSI, canal, endereço MAC, etc.)
        printf("SSID: %-32s RSSI: %4d CHAN: %3d MAC: %02x:%02x:%02x:%02x:%02x:%02x SEC: %u\n",
               result->ssid, result->rssi, result->channel,
               result->bssid[0], result->bssid[1], result->bssid[2],
               result->bssid[3], result->bssid[4], result->bssid[5],
               result->auth_mode);
    }
    return 0; // Retorna 0 para continuar a varredura
}