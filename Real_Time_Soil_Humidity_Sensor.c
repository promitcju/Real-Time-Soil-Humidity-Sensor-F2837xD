// ================= LIBRARIES =================
#include "F2837xD_device.h"
#include <stdio.h>
#include <stdlib.h> // Required for rand()
extern void DelayUs(Uint16);
// ================= DEFINES =================
//--- I2C LCD (External 1602)--
#define LCD_ADDR 0x27
#define LCD_BACKLIGHT 0x08
#define EN 0x04
#define RS 0x01
// ================= GLOBALS =================
volatile Uint16 adcResult = 0;
float voltage = 0.0;
volatile float moisture = 0.0; // Marked volatile for ISR safety
float filtered_adc = 0.0;
char line1[16];
char line2[16];
int last_state =-1; // Tracks the size tier of the patch
// ================= FUNCTION PROTOTYPES =================
// System & Peripherals
void InitSysCtrl(void);
void configure_GPIO(void);
void configure_SPI(void);
void configure_I2C(void);
void configure_EPWM1(void);
void configure_CPU_timer0(void);
void configure_interrupts(void);
void initADC(void);
void initADCSOC(void);
1
// SPI LCD Functions (Updated for External ILI9341)
void LCD_reset_init(void);
void command_SPI_LCD(Uint16 command);
void data_SPI_LCD(Uint16 data);
void setAddrWindow(Uint16 x0, Uint16 y0, Uint16 x1, Uint16 y1);
void fillScreen(Uint16 color);
void drawPixelation(float moisture_level, int state);
// I2C LCD Functions
void I2C_LCD_Write(Uint16 data);
void LCD_Enable(Uint16 data);
void LCD_Write4Bits(Uint16 data);
void LCD_Command_I2C(Uint16 cmd);
void LCD_Data_I2C(Uint16 data);
void LCD_Init_I2C(void);
void LCD_Print(char *str);
// ISR
interrupt void timer_ISR(void);
// ================= MAIN =================
void main(void)
{
EALLOW;
WdRegs.WDCR.all = 0x0068; // Disable watchdog (CRITICAL:
LEAVE DISABLED)
EDIS;
InitSysCtrl();
// Initialize all hardware pins and modules
configure_GPIO();
configure_SPI();
configure_I2C();
configure_EPWM1();
// Timer and Interrupt Setup
configure_CPU_timer0();
configure_interrupts();
initADC();
initADCSOC();
DelayUs(20000);
// Initialize both screens
LCD_reset_init(); // External SPI
LCD_Init_I2C(); // External I2C
fillScreen(0x0000); // Clear SPI screen to black initially
2
// Start CPU timer for background PWM updates
CpuTimer0Regs.TCR.bit.TSS = 0;
while(1)
{
//-------- 1. READ ADC-------
AdcaRegs.ADCSOCFRC1.bit.SOC0 = 1;
while(AdcaRegs.ADCINTFLG.bit.ADCINT1 == 0);
AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
adcResult = AdcaResultRegs.ADCRESULT0;
//-------- 2. CALCULATE MOISTURE-------
filtered_adc = (filtered_adc * 0.99f) + ((float)adcResult
* 0.01f);
voltage = (filtered_adc * 3.3f) / 4095.0f;
float air_voltage = 1.14f;
float water_voltage = 0.57f;
moisture = ((air_voltage- voltage) / (air_voltage
water_voltage)) * 100.0f;
if(moisture > 100.0f) moisture = 100.0f;
if(moisture < 0.0f) moisture = 0.0f;
//-------- 3. UPDATE I2C TEXT LCD-------
sprintf(line1, "V=%5.2f␣V␣␣␣", voltage);
sprintf(line2, "M=%6.1f␣%%␣␣", moisture);
LCD_Command_I2C(0x80);
LCD_Print(line1);
LCD_Command_I2C(0xC0);
LCD_Print(line2);
//-------- 4. UPDATE SPI EXTERNAL LCD (SCALING
PIXELATION)-------
int current_state = 0;
if (moisture >= 80.0f) {
current_state = 3; // Large
} else if (moisture >= 60.0f) {
current_state = 2; // Medium
} else if (moisture > 40.0f) {
current_state = 1; // Small
} else {
current_state = 0; // Off
}
// Handle Cleanup
if (current_state != last_state)
{
3
if (current_state < last_state)
{
setAddrWindow(80, 40, 239, 199);
Uint32 i;
for(i=0; i<25600; i++) {
data_SPI_LCD(0x00);
data_SPI_LCD(0x00);
}
}
last_state = current_state;
}
// Draw the pixelation if active
if (current_state > 0)
{
drawPixelation(moisture, current_state);
}
//-------- 5. DELAY-------
int d;
for(d=0; d<4; d++) {
DelayUs(50000);
}
}
}
// ================= ISR (BACKGROUND PWM UPDATE)
=================
interrupt void timer_ISR(void)
{
// The PWM duty logic has been moved here. It will run
independently
// based on the Timer0 period, using the latest global
moisture value.
float duty = moisture / 100.0f;
if(duty < 0.05f) duty = 0.05f; // minimum visibility
Uint16 cmp = (Uint16)(duty * EPwm1Regs.TBPRD);
EPwm1Regs.CMPB.bit.CMPB = cmp; // LED brightness
EPwm1Regs.CMPA.bit.CMPA = cmp; // Buzzer intensity
// Acknowledge the interrupt
PieCtrlRegs.PIEACK.all = 0x0001;
}
// ================= HARDWARE CONFIGURATIONS =================
void InitSysCtrl(void)
{
EALLOW;
4
ClkCfgRegs.CLKSRCCTL1.bit.OSCCLKSRCSEL = 1;
ClkCfgRegs.SYSPLLCTL1.bit.PLLCLKEN = 0;
ClkCfgRegs.SYSCLKDIVSEL.all = 2;
ClkCfgRegs.SYSPLLMULT.all = 20;
while(ClkCfgRegs.SYSPLLSTS.bit.LOCKS != 1);
ClkCfgRegs.SYSCLKDIVSEL.all = 1;
ClkCfgRegs.SYSPLLCTL1.bit.PLLCLKEN = 1;
ClkCfgRegs.SYSCLKDIVSEL.all = 0;
EDIS;
}
void configure_GPIO(void)
{
EALLOW;
//--- I2C PINS--
GpioCtrlRegs.GPDGMUX1.bit.GPIO105 = 0;
GpioCtrlRegs.GPDMUX1.bit.GPIO105 = 1; // I2C SCL
GpioCtrlRegs.GPDPUD.bit.GPIO105 = 0;
GpioCtrlRegs.GPDGMUX1.bit.GPIO104 = 0;
GpioCtrlRegs.GPDMUX1.bit.GPIO104 = 1; // I2C SDA
GpioCtrlRegs.GPDPUD.bit.GPIO104 = 0;
//--- PWM PINS--
GpioCtrlRegs.GPAGMUX1.bit.GPIO0 = 0;
GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1; // EPWM1A
GpioCtrlRegs.GPAGMUX1.bit.GPIO1 = 0;
GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1; // EPWM1B
//--- SPI LCD PINS--
GpioCtrlRegs.GPBGMUX2.bit.GPIO58 = 3;
GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 3; // SPI SIMO
GpioCtrlRegs.GPBGMUX2.bit.GPIO60 = 3;
GpioCtrlRegs.GPBMUX2.bit.GPIO60 = 3; // SPI CLK
// SPI CS-> GPIO124
GpioCtrlRegs.GPDGMUX2.bit.GPIO124 = 0;
GpioCtrlRegs.GPDMUX2.bit.GPIO124 = 0; // GPIO mode
GpioCtrlRegs.GPDDIR.bit.GPIO124 = 1; // Output
GpioDataRegs.GPDSET.bit.GPIO124 = 1; // Idle High
// SPI RST-> GPIO122
GpioCtrlRegs.GPDGMUX2.bit.GPIO122 = 0;
GpioCtrlRegs.GPDMUX2.bit.GPIO122 = 0; // GPIO mode
GpioCtrlRegs.GPDDIR.bit.GPIO122 = 1; // Output
GpioDataRegs.GPDSET.bit.GPIO122 = 1; // Idle High
5
// SPI DC/RS-> GPIO24
GpioCtrlRegs.GPAGMUX2.bit.GPIO24 = 0;
GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 0; // GPIO mode
GpioCtrlRegs.GPADIR.bit.GPIO24 = 1; // Output
EDIS;
}
void configure_SPI(void)
{
EALLOW;
CpuSysRegs.PCLKCR8.bit.SPI_A = 1;
asm("␣NOP"); asm("␣NOP");
SpiaRegs.SPICCR.bit.SPISWRESET = 0;
SpiaRegs.SPICTL.bit.MASTER_SLAVE = 1;
SpiaRegs.SPICCR.bit.SPICHAR = 7;
SpiaRegs.SPICCR.bit.CLKPOLARITY = 1; // 1 preferred for
ILI9341
SpiaRegs.SPICTL.bit.CLK_PHASE = 0;
SpiaRegs.SPIBRR.bit.SPI_BIT_RATE = 49;
SpiaRegs.SPIPRI.bit.TRIWIRE = 0;
SpiaRegs.SPICCR.bit.SPISWRESET = 1;
EDIS;
}
void configure_CPU_timer0(void)
{
EALLOW;
CpuTimer0Regs.PRD.all = 199999999;
CpuTimer0Regs.TCR.bit.TSS = 1; // Stop timer during config
CpuTimer0Regs.TCR.bit.TRB = 1;
CpuTimer0Regs.TCR.bit.TIE = 1;
EDIS;
}
void configure_interrupts(void)
{
DINT;
PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
// CRITICAL: Must be mapped with EALLOW since PieVectTable is
protected
EALLOW;
PieVectTable.TIMER0_INT = &timer_ISR;
EDIS;
PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
IER = M_INT1;
EINT;
}
6
void configure_I2C(void)
{
EALLOW;
DevCfgRegs.CPUSEL7.bit.I2C_A = 0;
CpuSysRegs.PCLKCR9.bit.I2C_A = 1;
I2caRegs.I2CMDR.bit.IRS = 0;
I2caRegs.I2CPSC.bit.IPSC = 19;
I2caRegs.I2CCLKL = 45;
I2caRegs.I2CCLKH = 45;
I2caRegs.I2CMDR.bit.IRS = 1;
EDIS;
}
void configure_EPWM1(void)
{
EALLOW;
CpuSysRegs.PCLKCR2.bit.EPWM1 = 1;
CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
EPwm1Regs.TBPRD = 2000;
EPwm1Regs.TBCTL.bit.CTRMODE = 0;
EPwm1Regs.AQCTLA.bit.ZRO = 2; // Buzzer
EPwm1Regs.AQCTLA.bit.CAU = 1;
EPwm1Regs.AQCTLB.bit.ZRO = 2; // LED
EPwm1Regs.AQCTLB.bit.CBU = 1;
CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
EDIS;
}
void initADC(void)
{
EALLOW;
CpuSysRegs.PCLKCR13.bit.ADC_A = 1;
AdcaRegs.ADCCTL2.bit.PRESCALE = 6;
AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
DelayUs(1000);
EDIS;
}
void initADCSOC(void)
{
EALLOW;
AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0; // ADCINA0
AdcaRegs.ADCSOC0CTL.bit.ACQPS = 14;
AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0;
AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;
7
EDIS;
}
// ================= SPI EXTERNAL LCD FUNCTIONS =================
void command_SPI_LCD(Uint16 command)
{
GpioDataRegs.GPDCLEAR.bit.GPIO124 = 1; // CS LOW
GpioDataRegs.GPACLEAR.bit.GPIO24 = 1; // DC LOW
SpiaRegs.SPICTL.bit.TALK = 1;
SpiaRegs.SPITXBUF = (Uint16)((Uint32)command << 8);
while(SpiaRegs.SPISTS.bit.INT_FLAG == 0);
(void)(SpiaRegs.SPIRXBUF);
GpioDataRegs.GPDSET.bit.GPIO124 = 1; // CS HIGH
}
void data_SPI_LCD(Uint16 data)
{
GpioDataRegs.GPDCLEAR.bit.GPIO124 = 1; // CS LOW
GpioDataRegs.GPASET.bit.GPIO24 = 1; // DC HIGH
SpiaRegs.SPICTL.bit.TALK = 1;
SpiaRegs.SPITXBUF = (Uint16)((Uint32)data << 8);
while(SpiaRegs.SPISTS.bit.INT_FLAG == 0);
(void)(SpiaRegs.SPIRXBUF);
GpioDataRegs.GPDSET.bit.GPIO124 = 1; // CS HIGH
}
void setAddrWindow(Uint16 x0, Uint16 y0, Uint16 x1, Uint16 y1)
{
command_SPI_LCD(0x2A);
data_SPI_LCD(x0 >> 8); data_SPI_LCD(x0 & 0xFF);
data_SPI_LCD(x1 >> 8); data_SPI_LCD(x1 & 0xFF);
command_SPI_LCD(0x2B);
data_SPI_LCD(y0 >> 8); data_SPI_LCD(y0 & 0xFF);
data_SPI_LCD(y1 >> 8); data_SPI_LCD(y1 & 0xFF);
command_SPI_LCD(0x2C);
}
void fillScreen(Uint16 color)
{
Uint32 i;
setAddrWindow(0, 0, 319, 239);
for(i=0; i < (Uint32)320 * 240; i++)
{
8
data_SPI_LCD(color >> 8);
data_SPI_LCD(color & 0xFF);
}
}
void LCD_reset_init(void)
{
GpioDataRegs.GPDCLEAR.bit.GPIO122 = 1;
DelayUs(20000);
GpioDataRegs.GPDSET.bit.GPIO122 = 1;
int i;
for(i=0; i<4; i++) { DelayUs(50000); }
command_SPI_LCD(0x11); // Sleep Out
for(i=0; i<4; i++) { DelayUs(50000); }
command_SPI_LCD(0x36); // MAC
data_SPI_LCD(0x28); // Landscape
command_SPI_LCD(0x3A); // Color Format
data_SPI_LCD(0x55); // 16-bit
fillScreen(0x0000);
command_SPI_LCD(0x29); // Display On
}
// DYNAMIC PIXELATION GENERATOR
void drawPixelation(float moisture_level, int state)
{
Uint32 i, max_pixels;
int threshold = (int)moisture_level;
if (state == 1) {
// Small: 80x80 patch
setAddrWindow(120, 80, 199, 159);
max_pixels = 6400;
}
else if (state == 2) {
// Medium: 120x120 patch
setAddrWindow(100, 60, 219, 179);
max_pixels = 14400;
}
else {
// Large: 160x160 patch
setAddrWindow(80, 40, 239, 199);
max_pixels = 25600;
}
for(i = 0; i < max_pixels; i++)
9
{
if ((rand() % 100) < threshold)
{
Uint16 random_color = rand();
data_SPI_LCD(random_color >> 8);
data_SPI_LCD(random_color & 0xFF);
}
else
{
data_SPI_LCD(0x00);
data_SPI_LCD(0x00);
}
}
}
// ================= I2C EXTERNAL LCD FUNCTIONS =================
void I2C_LCD_Write(Uint16 data)
{
while(I2caRegs.I2CMDR.bit.STP == 1);
I2caRegs.I2CSAR.bit.SAR = LCD_ADDR;
I2caRegs.I2CCNT = 1;
I2caRegs.I2CDXR.bit.DATA = data;
I2caRegs.I2CMDR.all = 0x6E20;
while(I2caRegs.I2CMDR.bit.STP == 1);
}
void LCD_Enable(Uint16 data)
{
I2C_LCD_Write(data | EN);
DelayUs(1000);
I2C_LCD_Write(data & ~EN);
}
void LCD_Write4Bits(Uint16 data)
{
I2C_LCD_Write(data | LCD_BACKLIGHT);
LCD_Enable(data | LCD_BACKLIGHT);
}
void LCD_Command_I2C(Uint16 cmd)
{
Uint16 high = (cmd & 0xF0);
Uint16 low = ((cmd << 4) & 0xF0);
LCD_Write4Bits(high);
LCD_Write4Bits(low);
}
void LCD_Data_I2C(Uint16 data)
{
Uint16 high = (data & 0xF0) | RS;
Uint16 low = ((data << 4) & 0xF0) | RS;
10
LCD_Write4Bits(high);
LCD_Write4Bits(low);
}
void LCD_Init_I2C(void)
{
DelayUs(20000);
LCD_Write4Bits(0x30);
DelayUs(5000);
LCD_Write4Bits(0x30);
DelayUs(1000);
LCD_Write4Bits(0x30);
DelayUs(1000);
LCD_Write4Bits(0x20);
LCD_Command_I2C(0x28);
LCD_Command_I2C(0x0C);
LCD_Command_I2C(0x06);
LCD_Command_I2C(0x01);
DelayUs(2000);
}
void LCD_Print(char *str)
{
while(*str)
{
LCD_Data_I2C(*str++);
}
}