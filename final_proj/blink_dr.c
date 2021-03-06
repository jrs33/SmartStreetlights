//****************************************************************************
// EC450 Blink Example 0 -DriverLib version
//****************************************************************************
/* This example is identical in spirit to the software Driverlib blink example from TI.
 * The only differences are that:
 *   a) the count between toggles is a variable we can change in the debugger
 *   b) The LED used is connected to P2.4 (Port 2, bit 4) which connects to the
 *      bright green pin in the 3-color LED on the EDU-MKII booster pack
 */
/* DriverLib Includes */
#include <LcdDriver/Crystalfontz128x128_ST7735.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <ti/grlib/grlib.h>


// Initializing for alarm Timer usage for a 1kHz tone
#define initialHalfPeriod 12000

// Initialized to allow button use to enable certain alarms
#define BUTTON_PORT5 P5
#define BUTTON_BIT1 BIT1
#define BUTTON_PORT3 P3
#define BUTTON_BIT5  BIT5
#define TEXTCOL GRAPHICS_COLOR_YELLOW
#define BACKCOL GRAPHICS_COLOR_BLACK
#define DOTCOL GRAPHICS_COLOR_LIGHT_GREEN
#define DOTCOL_PRESSED GRAPHICS_COLOR_RED
#define RADIUS 2
Graphics_Context g_sContext;    // graphics context for grlib
uint16_t xscreen, yscreen;      // current screen location coordinates

void init_button()
{
    // Set up the first button for interrupts
    BUTTON_PORT5->OUT |= BUTTON_BIT1;
    BUTTON_PORT5->REN |= BUTTON_BIT1;
    BUTTON_PORT5->IES |= BUTTON_BIT1;
    BUTTON_PORT5->IFG &= ~BUTTON_BIT1;
    BUTTON_PORT5->IE  |= BUTTON_BIT1;
    // Set up the second button for interrupts
    BUTTON_PORT3->OUT |= BUTTON_BIT5;
    BUTTON_PORT3->REN |= BUTTON_BIT5;
    BUTTON_PORT3->IES |= BUTTON_BIT5;
    BUTTON_PORT3->IFG &= ~BUTTON_BIT5;
    BUTTON_PORT3->IE  |= BUTTON_BIT5;
}

void mapports(){
    PMAP->KEYID=PMAP_KEYID_VAL; // unlock PMAP
    P2MAP->PMAP_REGISTER7=PMAP_TA0CCR0A;  // map TA0CCR0 to P2.7 as primary module
    PMAP->KEYID=0;               // relock PMAP until next hard reset
}

void init_timer()
{
    TIMER_A0->CTL |= TIMER_A_CTL_CLR ;// reset clock
    TIMER_A0->CTL =  TIMER_A_CTL_TASSEL_2  // clock source = SMCLK
                    +TIMER_A_CTL_ID_0      // clock prescale=1
                    +TIMER_A_CTL_MC_1;     // Up Mode
    TIMER_A0->EX0 = TIMER_A_EX0_TAIDEX_2;  // additional divisor=3

    TIMER_A0->CCTL[0]=TIMER_A_CCTLN_OUTMOD_4; // compare mode, output mode 4 (toggle)
    TIMER_A0->CCR[0] = initialHalfPeriod-1; // in up mode TAR=0... TACCRO-1
    P2->SEL0|=BIT7; // connect timer output to pin (select alternate function for pin)
    P2->DIR |=BIT7; // output mode on P2.7 (direction output completes setting the function)
}

void init_display(){
    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,&g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, TEXTCOL);
    Graphics_setBackgroundColor(&g_sContext, BACKCOL);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawString(&g_sContext,"J:",AUTO_STRING_LENGTH,0,116,OPAQUE_TEXT);
    xscreen=0;
    yscreen=0;  // just use origin, first write is a background
}

// PIR IRQ
void PORT6_IRQHandler(){
    volatile unsigned int half_period=1000000;
    volatile unsigned int countdown;
    uint32_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P6);
    GPIO_clearInterruptFlag(GPIO_PORT_P6, status);

        TIMER_A0->CCTL[0]^=TIMER_A_CCTLN_OUTMOD_4;
        // Replace this logic with triggering the GREEN light for a bit and then set back to default RED
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN6);
        for (countdown=half_period;countdown>0;--countdown){
        }
        TIMER_A0->CCTL[0]^=TIMER_A_CCTLN_OUTMOD_4;
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN6);

    MAP_GPIO_enableInterrupt(GPIO_PORT_P6, GPIO_PIN7);
}

// Distance Sensor IRQ
void PORT4_IRQHandler(){
    volatile unsigned int half_period=250000;
    volatile unsigned int countdown;
    uint32_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P5);
    GPIO_clearInterruptFlag(GPIO_PORT_P5, status);

        // If motion is detected within range set motion detection
        if(P4IN & GPIO_PIN4) {
            TIMER_A0->CCTL[0]^=TIMER_A_CCTLN_OUTMOD_4;
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN4);
            for (countdown=half_period;countdown>0;--countdown){
            }
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4);
        }
}

int main(void)
{
    /* Halting the Watchdog */
    MAP_WDT_A_holdTimer();

    mapports();
    init_timer();
    init_display();

    MAP_Interrupt_disableSleepOnIsrExit();
    MAP_Interrupt_enableMaster();

    /* Configuring the PIR sensor interrupt */
    MAP_Interrupt_enableInterrupt(INT_PORT6);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P6, GPIO_PIN7);
    MAP_GPIO_setAsInputPin(GPIO_PORT_P6, GPIO_PIN7);

    /* Configuring the Distance sensor interrupt */
    MAP_Interrupt_enableInterrupt(INT_PORT4);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN4);
    MAP_GPIO_setAsInputPin(GPIO_PORT_P4, GPIO_PIN4);

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN6);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN4);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN6);

    volatile unsigned int half_period=250000;
    volatile unsigned int countdown;
    while (1)
    {
        /* Delay Loop */
        for (countdown=half_period;countdown>0;--countdown){
        }
    }
}
