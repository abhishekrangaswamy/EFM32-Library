/**************************************************************************//**
 * @file
 * @brief Demo for energy mode current consumption testing.
 * @version 3.20.5
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include <stdio.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_pcnt.h"

#include "display.h"
#include "textdisplay.h"
#include "retargettextdisplay.h"

#define NO_OF_EMODE_TESTS            (9) /* Number of energy modes.     */

/* Frequency of RTC (COMP0) pulses on PRS channel 2. */
#define RTC_PULSE_FREQUENCY    (LS013B7DH03_POLARITY_INVERSION_FREQUENCY)

static volatile int      eMode;          /* Selected energy mode.            */
static volatile bool     startTest;      /* Start selected energy mode test. */
static volatile bool     displayEnabled; /* Status of LCD display.           */
static volatile uint32_t seconds = 0;     /* Seconds elapsed since reset.    */
static DISPLAY_Device_t  displayDevice;  /* Display device handle.           */

static void GpioSetup(void);
static void PcntInit(void);
static void SelectClock( CMU_Select_TypeDef hfClockSelect,
                         uint32_t clockDisableMask );
static void EnterEMode( int mode, uint32_t secs );

/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
  int currentEMode;

  /* Chip errata */
  CHIP_Init();

  /* Setup GPIO for pushbuttons. */
  GpioSetup();

  /* Initialize the display module. */
  displayEnabled = true;
  DISPLAY_Init();

  /* Retrieve the properties of the display. */
  if ( DISPLAY_DeviceGet( 0, &displayDevice ) != DISPLAY_EMSTATUS_OK )
  {
    /* Unable to get display handle. */
    while( 1 );
  }

  /* Retarget stdio to the display. */
  if ( TEXTDISPLAY_EMSTATUS_OK != RETARGET_TextDisplayInit() )
  {
    /* Text display initialization failed. */
    while( 1 );
  }

  /* Set PCNT to generate an interrupt every second. */
  PcntInit();

  printf( "\n\n\n Push PB1 to\n"
          " cycle through\n"
          " the energy mode\n"
          " tests available"
          "\n\n Push PB0 to\n"
          " start test.\n\n\n" );

  startTest    = false;
  eMode        = 0;
  currentEMode = -1;

  /* Disable LFB clock tree. */
  CMU->LFCLKSEL &= ~(_CMU_LFCLKSEL_LFB_MASK);

  while ( 1 )
  {
    /* Mode change ? If yes, update the display. */
    if ( eMode != currentEMode )
    {
      currentEMode = eMode;
      switch ( eMode )
      {
        case 0:
          printf("\r    EM0 24MHz\n"
                   "  (primes calc)");
          break;

        case 1:
          printf("\r               " );
          printf( TEXTDISPLAY_ESC_SEQ_CURSOR_HOME_VT100 );
          printf("\n\n\n\n\n\n\n\n\n\n\n\n    EM1 24MHz");
          break;

        case 2:
          printf("\r    EM2 32kHz");
          break;

        case 3:
          printf("\r       EM3   ");
          break;

        case 4:
          printf("\r       EM4");
          break;

        case 5:
          printf("\r     EM2+RTC");
          break;

        case 6:
          printf("\r   EM2+RTC+LCD");
          break;

        case 7:
          printf("\r   EM3+RTC+LCD");
          break;

        case 8:
          printf("\r      USER    ");
          break;
      }
    }

    if ( startTest )
    {
      /* Get ready to start the energy mode test. Turn off everything we     */
      /* don't need. Use energyAware Profiler to observe energy consumption. */

      /* Disable GPIO. */
      NVIC_DisableIRQ(GPIO_EVEN_IRQn);
      NVIC_DisableIRQ(GPIO_ODD_IRQn);
      GPIO_PinModeSet(gpioPortC, 8, gpioModeDisabled, 1);
      GPIO_PinModeSet(gpioPortC, 9, gpioModeDisabled, 1);

      /* Clear LCD display. */
      printf("\f");

      switch (eMode)
      {
        case 0:           /* EM0 24MHz (primes) */
        case 1:           /* EM1 24MHz */
        case 2:           /* EM2 32kHz */
        case 3:           /* EM3 */
        case 4:           /* EM4 */
          /* Power down LCD display and disable the RTC. */
          displayEnabled = false;
          NVIC_DisableIRQ(RTC_IRQn);
          NVIC_DisableIRQ(PCNT0_IRQn);
          displayDevice.pDisplayPowerOn( &displayDevice, false );
          break;

        case 5:           /* EM2+RTC */
          /* Power down LCD display. */
          displayEnabled = false;
          displayDevice.pDisplayPowerOn( &displayDevice, false );
          break;

        case 6:           /* EM2+RTC+LCD */
        case 7:           /* EM3+RTC+LCD */
          break;

        case 8:           /* USER */
          break;
      }

      /* Do the slected energy mode test. */
      switch (eMode)
      {
        case 0:  /* EM0 24MHz (primes) */
          SelectClock( cmuSelect_HFXO,               /* HF clock           */
                       CMU_OSCENCMD_HFRCODIS |       /* Clock disable mask */
                       CMU_OSCENCMD_LFXODIS  |
                       CMU_OSCENCMD_LFRCODIS );
          {
            #define PRIM_NUMS  (64)
            uint32_t i, d, n;
            uint32_t primes[PRIM_NUMS];

            /* Find prime numbers forever */
            while (1)
            {
              primes[0] = 1;
              for (i = 1; i < PRIM_NUMS;)
              {
                for (n = primes[i - 1] + 1;; n++)
                {
                  for (d = 2; d <= n; d++)
                  {
                    if (n == d)
                    {
                      primes[i] = n;
                      goto nexti;
                    }
                    if (n % d == 0) break;
                  }
                }
              nexti:
                i++;
              }
            }
          }
          /*break;*/

        case 1:  /* EM1 24MHz */
          SelectClock( cmuSelect_HFXO,               /* HF clock           */
                       CMU_OSCENCMD_HFRCODIS |       /* Clock disable mask */
                       CMU_OSCENCMD_LFXODIS  |
                       CMU_OSCENCMD_LFRCODIS );
          EnterEMode(1, 1);
          break;

        case 2:  /* EM2 32kHz */
          SelectClock( cmuSelect_LFRCO,              /* HF clock           */
                       CMU_OSCENCMD_HFXODIS  |       /* Clock disable mask */
                       CMU_OSCENCMD_HFRCODIS |
                       CMU_OSCENCMD_LFXODIS  );
          EnterEMode(2, 1);
          break;

        case 3:  /* EM3 */
          EnterEMode(3, 1);
          break;

        case 4:  /* EM4 */
          EnterEMode(4, 1);
          break;

        case 5:  /* EM2+RTC */
          /* Wake up on each PCNT interrupt. */
          while (1)
          {
            EnterEMode(2, 1);
            /* This loop will be visible in eAProfiler. */
            { volatile int i; for( i=0; i<10000; i++ ); }
          }
          /*break;*/

        case 6:  /* EM2+RTC+LCD */
          /* Wake up on each PCNT interrupt. */
          printf("\n\n\n\n\n\n\n\n");
          while (1)
          {
            EnterEMode(2, 1);
            printf("\r  EM2+RTC+LCD -");
            EnterEMode(2, 1);
            printf("\r  EM2+RTC+LCD \\");
            EnterEMode(2, 1);
            printf("\r  EM2+RTC+LCD |");
            EnterEMode(2, 1);
            printf("\r  EM2+RTC+LCD /");
          }
          /*break;*/

        case 7:  /* EM3+RTC+LCD */
          /* Wake up on each PCNT interrupt. */
          printf("\n\n\n\n\n\n\n\n");
          while (1)
          {
            /* Disable LFB clock select */
            EnterEMode(3, 1);
            printf("\r  EM3+RTC+LCD -");
            EnterEMode(3, 1);
            printf("\r  EM3+RTC+LCD \\");
            EnterEMode(3, 1);
            printf("\r  EM3+RTC+LCD |");
            EnterEMode(3, 1);
            printf("\r  EM3+RTC+LCD /");
          }
          /*break;*/

        case 8:           /* USER */
          for (;;);
          /*break;*/
      }

      /* We should never end up here ! */
      EFM_ASSERT( false );
    }
  }
}

/**************************************************************************//**
 * @brief   Enter and stay in Energy Mode for a given number of seconds.
 *
 * @param[in] mode  Energy Mode to enter (1..4).
 * @param[in] secs  Time to stay in Energy Mode <mode>.
 *****************************************************************************/
static void EnterEMode( int mode, uint32_t secs )
{
  if ( secs )
  {
    uint32_t startTime = seconds;

    while ((seconds - startTime) < secs)
    {
      switch ( mode )
      {
        case 1: EMU_EnterEM1();         break;
        case 2: EMU_EnterEM2( false );  break;
        case 3: EMU_EnterEM3( false );  break;
        case 4: EMU_EnterEM4();         break;
      default:
        /* Invalid mode. */
        while(1);
      }
    }
  }
}

/**************************************************************************//**
 * @brief Setup GPIO interrupt for pushbuttons.
 *****************************************************************************/
static void GpioSetup(void)
{
  /* Enable GPIO clock */
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Configure PC8 as input and enable interrupt  */
  GPIO_PinModeSet(gpioPortC, 8, gpioModeInputPull, 1);
  GPIO_IntConfig(gpioPortC, 8, false, true, true);

  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);

  /* Configure PC9 as input and enable interrupt */
  GPIO_PinModeSet(gpioPortC, 9, gpioModeInputPull, 1);
  GPIO_IntConfig( gpioPortC, 9, false, true, true);

  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/**************************************************************************//**
 * @brief GPIO Interrupt handler (PB1)
 *        Starts selected energy mode test.
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  /* Acknowledge interrupt */
  GPIO_IntClear(1 << 9);

  eMode = (eMode + 1) % NO_OF_EMODE_TESTS;
}

/**************************************************************************//**
 * @brief GPIO Interrupt handler (PB0)
 *        Sets next energy mode test number.
 *****************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  /* Acknowledge interrupt */
  GPIO_IntClear(1 << 8);

  startTest = true;
}

/**************************************************************************//**
 * @brief   Set up PCNT to generate an interrupt every second.
 *
 *****************************************************************************/
void PcntInit(void)
{
  PCNT_Init_TypeDef pcntInit = PCNT_INIT_DEFAULT;

  /* Enable PCNT clock */
  CMU_ClockEnable(cmuClock_PCNT0, true);
  /* Set up the PCNT to count RTC_PULSE_FREQUENCY pulses -> one second */
  pcntInit.mode = pcntModeOvsSingle;
  pcntInit.top = RTC_PULSE_FREQUENCY;
  pcntInit.s1CntDir = false;
  pcntInit.s0PRS = pcntPRSCh2;

  PCNT_Init(PCNT0, &pcntInit);

  /* Select PRS as the input for the PCNT */
  PCNT_PRSInputEnable(PCNT0, pcntPRSInputS0, true);

  /* Enable PCNT interrupt every second */
  NVIC_EnableIRQ(PCNT0_IRQn);
  PCNT_IntEnable(PCNT0, PCNT_IF_OF);
}


/**************************************************************************//**
 * @brief   This interrupt is triggered at every second by the PCNT
 *
 *****************************************************************************/
void PCNT0_IRQHandler(void)
{
  PCNT_IntClear(PCNT0, PCNT_IF_OF);

  seconds++;

  return;
}


/**************************************************************************//**
 * @brief   Select a clock source for HF clock, optionally disable other clocks.
 *
 * @param[in] hfClockSelect      The HF clock to select.
 * @param[in] clockDisableMask   Bit masks with clocks to disable.
 *****************************************************************************/
static void SelectClock( CMU_Select_TypeDef hfClockSelect,
                          uint32_t clockDisableMask )
{
  /* Select HF clock. */
  CMU_ClockSelectSet( cmuClock_HF, hfClockSelect );

  /* Disable unwanted clocks. */
  CMU->OSCENCMD     = clockDisableMask;

  /* Turn off clock enables. */
  CMU->HFPERCLKEN0  = 0x00000000;
  CMU->HFCORECLKEN0 = 0x00000000;
  CMU->LFACLKEN0    = 0x00000000;
  CMU->LFBCLKEN0    = 0x00000000;
  CMU->LFCLKSEL     = 0x00000000;
}
