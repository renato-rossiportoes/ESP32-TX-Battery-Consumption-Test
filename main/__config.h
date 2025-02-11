#define RELE1_PIN               GPIO_NUM_32
#define RELE2_PIN               GPIO_NUM_33
#define RELE_DURATION_MS        50
#define RELE_INTERVAL_MS        700

#define GREEN_OK_LED_PIN        GPIO_NUM_26
#define RF_RX_PIN               GPIO_NUM_15   // Pino onde recebe os dados do Receptor RX

// Definições dos pinos dos botões
#define BUTTON1_GPIO 13
#define BUTTON2_GPIO 12
#define BUTTON3_GPIO 14

// Defines para a rotina que recebe os pulsos do RF e decodifica
#define PREAMBLE_PULSE_MIN_US 300
#define PREAMBLE_PULSE_MAX_US 500
#define PREAMBLE_EDGE_COUNT 23    // Quantidade de bordas no preambulo, após a primeira borda
#define THEADER_MIN_US 2700
#define THEADER_MAX_US 6200
#define DECODER_BITS    66
#define DECODER_S_PREAMBLE      0
#define DECODER_S_THEADER       1
#define DECODER_S_66_BITS       2

#define TX_REG_TIMEOUT_MS   2000    // Timeout para quando esperar receber um TX para registrar

#define NUM_OF_RX_TO_VALIDADE   3     // # de recepções iguais em sequência para considerar uma transmissão válida.
#define OK_LED_TIME_MS          30    // Tempo que o LED fica acesso ao receber RX validado

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"
#define PROMPT_STR CONFIG_IDF_TARGET


#define TX_SLOTS_NUM    10  // Quantidade de slots de TX registrados. Range: 01 - 99
#define TX_PINS_NUM     10  // Número de pinos configuráveis para associar a TXs
#define TX_TYPES_NUM    10  // Número de tipos diferentes de acionamento

// Text Colors
#define TCOLORY        "\033[0;93m"
#define TCOLORB        "\033[0;94m"
#define TCOLOR_RESET    "\033[0m"