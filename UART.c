#include <avr/io.h>
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// Definicion de colores ANSI


// Estructura para cada UART
typedef struct {
    volatile uint8_t *ubrrh;
    volatile uint8_t *ubrrl;
    volatile uint8_t *ucsra;
    volatile uint8_t *ucsrb;
    volatile uint8_t *ucsrc;
    volatile uint8_t *udr;
    uint8_t rxen_bit;
    uint8_t txen_bit;
    uint8_t rxcie_bit;
    uint8_t udre_bit;
    uint8_t rxc_bit;
    uint8_t ucsza0_bit;
    uint8_t ucsza1_bit;
    uint8_t upm0_bit;
    uint8_t upm1_bit;
    uint8_t usbs_bit;
} UART_Registers;

// Mapeo de los UARTs
const UART_Registers UART[] = {
    {   // UART0
        &UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UCSR0C, &UDR0,
        RXEN0, TXEN0, RXCIE0, UDRE0, RXC0,
        UCSZ00, UCSZ01, UPM00, UPM01, USBS0
    },
    {   // UART1
        &UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UCSR1C, &UDR1,
        RXEN1, TXEN1, RXCIE1, UDRE1, RXC1,
        UCSZ10, UCSZ11, UPM10, UPM11, USBS1
    },
    {   // UART2
        &UBRR2H, &UBRR2L, &UCSR2A, &UCSR2B, &UCSR2C, &UDR2,
        RXEN2, TXEN2, RXCIE2, UDRE2, RXC2,
        UCSZ20, UCSZ21, UPM20, UPM21, USBS2
    },
    {   // UART3
        &UBRR3H, &UBRR3L, &UCSR3A, &UCSR3B, &UCSR3C, &UDR3,
        RXEN3, TXEN3, RXCIE3, UDRE3, RXC3,
        UCSZ30, UCSZ31, UPM30, UPM31, USBS3
    }
};

// Implementacion de funciones
void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop) {
    if(com > 3) return;
    
    uint16_t ubrr = (F_CPU / 16 / baudrate) - 1;
    *UART[com].ubrrh = (uint8_t)(ubrr >> 8);
    *UART[com].ubrrl = (uint8_t)ubrr;
    
    *UART[com].ucsrb = (1<<UART[com].rxen_bit) | (1<<UART[com].txen_bit) | (1<<UART[com].rxcie_bit);
    
    uint8_t config = 0;
    switch(size) {
        case 5: break;
        case 6: config |= (1<<UART[com].ucsza0_bit); break;
        case 7: config |= (1<<UART[com].ucsza1_bit); break;
        case 8: config |= (1<<UART[com].ucsza1_bit) | (1<<UART[com].ucsza0_bit); break;
        default: config |= (1<<UART[com].ucsza1_bit) | (1<<UART[com].ucsza0_bit);
    }
    
    switch(parity) {
        case 0: break;
        case 1: config |= (1<<UART[com].upm1_bit); break;
        case 2: config |= (1<<UART[com].upm1_bit) | (1<<UART[com].upm0_bit); break;
    }
    
    if(stop == 2) config |= (1<<UART[com].usbs_bit);
    
    *UART[com].ucsrc = config;
}

void UART_putchar(uint8_t com, char data) {
    if(com > 3) return;
    while(!(*UART[com].ucsra & (1<<UART[com].udre_bit)));
    *UART[com].udr = data;
}

void UART_puts(uint8_t com, char *str) {
    if(com > 3) return;
    while(*str) UART_putchar(com, *str++);
}

uint8_t UART_available(uint8_t com) {
    if(com > 3) return 0;
    return (*UART[com].ucsra & (1<<UART[com].rxc_bit)) ? 1 : 0;
}

char UART_getchar(uint8_t com) {
    if(com > 3) return 0;
    while(!UART_available(com));
    return *UART[com].udr;
}


void UART_gets(uint8_t com, char *str) {
    if (com > 3) return;
    char c;
    uint8_t i = 0;

    while (1) {
        if (i < 20) {  // Permitir entradas mientras no se alcance el limite de 20
            c = UART_getchar(com);

            if (c == '\r' || c == '\n') {  // Finalizar si se presiona "Enter"
                break;
            } else if (c == '\b' && i > 0) {  // Manejo del retroceso
                i--;
                UART_putchar(com, '\b');
                UART_putchar(com, ' ');
                UART_putchar(com, '\b');
            } else if (c >= '0' && c <= '9') {  // Aceptar solo numeros
                str[i++] = c;
                UART_putchar(com, c);  // Eco de la entrada
            } else {
                UART_putchar(com, '\a');  // Ignorar caracteres no validos
            }
        } else {  // Limite alcanzado, esperar "Enter"
            c = UART_getchar(com);

            if (c == '\r' || c == '\n') {  // Continuar solo si se presiona "Enter"
                break;
            } else if (c == '\b' && i > 0) {  // Permitir retroceso si se borra un valor
                i--;
                UART_putchar(com, '\b');
                UART_putchar(com, ' ');
                UART_putchar(com, '\b');
            } else {
                UART_putchar(com, '\a');  // Ignorar cualquier otra entrada
            }
        }
    }

    str[i] = '\0';  // Finalizar la cadena
}

void UART_clrscr(uint8_t com) {
    UART_puts(com, "\x1B[2J");
}

void UART_setColor(uint8_t com, uint8_t color) {
    char cmd[8] = "\x1B[1;3Xm";
    cmd[5] = '0' + (color % 8);
    UART_puts(com, cmd);
}

void UART_gotoxy(uint8_t com, uint8_t x, uint8_t y) {
    char cmd[12] = "\x1B[00;00H"; 
    
    // Coordenada Y (fila)
    cmd[3] = '0' + (y % 10);
    if(y >= 10) cmd[2] = '0' + (y / 10);
    
    // Coordenada X (columna)
    cmd[6] = '0' + (x % 10);
    if(x >= 10) cmd[5] = '0' + (x / 10);
    
    UART_puts(com, cmd);
}

void itoa(uint16_t number, char* str, uint8_t base) {
    const char digits[] = "0123456789ABCDEF";
    char buf[17];
    uint8_t i = 0;
    
    if(base < 2 || base > 16) {
        str[0] = '\0';
        return;
    }
    
    if(number == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    while(number > 0) {
        buf[i++] = digits[number % base];
        number /= base;
    }
    
    uint8_t j;
    for(j = 0; j < i; j++) {
        str[j] = buf[i-1-j];
    }
    str[j] = '\0';
}

uint16_t atoi(char *str) {
    uint16_t result = 0;
    while(*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}
