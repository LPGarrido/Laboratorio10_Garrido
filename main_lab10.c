/* 
 * File:   main.c
 * Author: Luis Garrido
 *
 * Comunicación serial, oscilador de 1MHz y baud rate de 9600.
 * Leer potenciometro y enviar ascii
 * 
 * Created on 17 april 2022, 20:12
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>


/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint8_t indice = 0;                     // Variable para saber que posición del mensaje enviar al serial
char    mensaje2[3] = {0,0,0};          // Mensaje a mostrar en leer pot (cen,dec,uni)
uint8_t indice2 = 0;                    // Bandera para enviar el valor del pot
uint8_t val_POT = 0;                    // Guarda el valor del potenciometro
uint8_t val_ASCII = 0;                  // Guarda el valor recibido por el ascii
uint8_t modo = 0;                       // Modo de leer valor pot o enviar ascii

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);                       // configuraciones
void obtener_valor(uint8_t VALOR);      // Obtener cen,dec,uni en ascii
void imprimir(char *str);               // Imprimir str
void TX_usart(char data);               // Enviar valores data

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.RCIF){              // Hay datos recibidos?
        val_ASCII = RCREG;          // Valor recibido
        if(modo == 1){              // Si estamos en el modo enviar ascii, 
            PORTB = val_ASCII;      // colocar el ascii en el puerto B
            modo = 0;               // Colocar modo normal
            indice = 0;             // Imprimir menu
        }
    }
    else if(PIR1bits.ADIF){         // Interrupcion ADC
        val_POT = ADRESH;           // Valor POT
        PIR1bits.ADIF = 0;          // Limpiar bandera
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){
        if(ADCON0bits.GO == 0){             // No hay proceso de conversion            
            ADCON0bits.GO = 1;              // Iniciamos proceso de conversión
        } 
        if (indice == 0){                   // Indice en 0, imprime el str
            imprimir("\n1 Leer Potenciometro \n2 Enviar ascii \n"); // mensaje
            indice = 1;                     // Indice en 1 para dejar de imprimir
        }
        if(val_ASCII == '1'){               // Si el ascii es 1, modo leer pot
            indice = 0;                     // Dar la instrucción de imprimir el menu
            modo = 0;                       // Modo normal
            obtener_valor(val_POT);         // Obtiene cen,dec,uni en ascii
            indice2 = 0;                    // Indice2 = 0 para imprimir valor pot
            if (indice2 == 0){              // Condicion
                TX_usart(mensaje2[0]);      // Imprime cen
                TX_usart(mensaje2[1]);      // Imprime dec
                TX_usart(mensaje2[2]);      // Imprime uni
                indice2 = 1;                // Dejar de imprimir
            }
            val_ASCII = 0;                  // Limpia el valor del ascii
        }
        if(val_ASCII == '2'){               // ascii=2, modo enviar ascii
            modo = 1;                       // Cambia el modo a 1
            val_ASCII = 0;                  // Limpia el valor del ascii
        }
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    ANSEL = 0x01;               // AN0 como entrada analogica
    ANSELH = 0;                 // I/O digitales
    
    TRISA = 0x01;               // AN0 como entrada
    PORTA = 0;
    TRISB = 0;
    PORTB = 0;                  // PORTD como salida
    
    // Configuración reloj interno
    OSCCONbits.IRCF = 0b0100;   // 1MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
    
    // Configuración ADC
    ADCON0bits.ADCS = 0b00;     // Fosc/XX  para 1MHz
    ADCON1bits.VCFG0 = 0;       // VDD
    ADCON1bits.VCFG1 = 0;       // VSS
    ADCON0bits.CHS = 0b0000;    // Seleccionamos el AN0
    ADCON1bits.ADFM = 0;        // Justificado a la izquierda
    ADCON0bits.ADON = 1;        // Habilitamos modulo ADC
    __delay_us(40);             // Sample time
    
    // Configuraciones de comunicacion serial
    //SYNC = 0, BRGH = 1, BRG16 = 1, SPBRG=25 <- Valores de tabla 12-5
    TXSTAbits.SYNC = 0;         // Comunicación ascincrona (full-duplex)
    TXSTAbits.BRGH = 1;         // Baud rate de alta velocidad 
    BAUDCTLbits.BRG16 = 1;      // 16-bits para generar el baud rate
    
    SPBRG = 25;
    SPBRGH = 0;                 // Baud rate ~9600, error -> 0.16%
    
    RCSTAbits.SPEN = 1;         // Habilitamos comunicación
    TXSTAbits.TX9 = 0;          // Utilizamos solo 8 bits
    TXSTAbits.TXEN = 1;         // Habilitamos transmisor
    RCSTAbits.CREN = 1;         // Habilitamos receptor
    
    // Configuraciones de interrupciones
    PIR1bits.ADIF = 0;          // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;          // Habilitamos interrupcion de ADC
    INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitamos interrupciones de perifericos
    PIE1bits.RCIE = 1;          // Habilitamos Interrupciones de recepción
}

/*------------------------------------------------------------------------------
 * FUNCIONES 
 ------------------------------------------------------------------------------*/

void obtener_valor(uint8_t VALOR){
    mensaje2[0] = VALOR/100;                             // centenas
    mensaje2[1] = (VALOR-mensaje2[0]*100)/10;            // decenas
    mensaje2[2] = VALOR-mensaje2[0]*100-mensaje2[1]*10;  // unidades
    //Conversion a ascii
    mensaje2[0] = mensaje2[0]+0x30;
    mensaje2[1] = mensaje2[1]+0x30;
    mensaje2[2] = mensaje2[2]+0x30;
}

void TX_usart(char data){
    while(TXSTAbits.TRMT==0);   // Esperar a que el TSR esté vacio
    TXREG = data;               // Enviar data especificada
}

void imprimir(char *str){
    while (*str != '\0'){       // Siempre y cuando no este vacio
        TX_usart(*str);         // Enviar cada elemento del str
        str++;                  // Siguiente elemento
    }
}