/********************************************************
 Name          : main.c
 Author        : asle
 Copyright     : 
 Description   : STK600 RCU3L0 template
 **********************************************************/

// Include Files
#include "board.h"

#if UC3L
// Target DFLL0 frequency
#define EXAMPLE_FDFLL_HZ              (100000000ULL)  // 100MHz
#define EXAMPLE_FDFLL_KHZ             (100000UL)
#define EXAMPLE_MCUCLK_HZ             (25000000UL)    // 25MHz
// Note: for UC3L devices, the clock configurations are handled by the SCIF module
// and the synchronous clocks used to clock the main digital logic are handled
// by the PM module.
#include "power_clocks_lib.h"
#endif

#include "max7219.h"

static void fcpu_fpba_configure()
{
#if UC3L
    static scif_gclk_opt_t gc_dfllif_ref_opt = { SCIF_GCCTRL_SLOWCLOCK, 0, OFF };
    static pcl_freq_param_t pcl_dfll_freq_param =
    {
      .main_clk_src = PCL_MC_DFLL0,
      .cpu_f        = EXAMPLE_MCUCLK_HZ,
      .pba_f        = EXAMPLE_MCUCLK_HZ,
      .pbb_f        = EXAMPLE_MCUCLK_HZ,
      .dfll_f       = EXAMPLE_FDFLL_HZ,
      .pextra_params = &gc_dfllif_ref_opt
    };
    // Implementation for UC3L
    // Note: on the AT32UC3L-EK board, there is no crystal/external clock connected
    // to the OSC0 pinout XIN0/XOUT0. We shall then program the DFLL and switch the
    // main clock source to the DFLL.
    pcl_configure_clocks(&pcl_dfll_freq_param);
#endif // UC3L
}

/* Add software framework include drivers below for desired peripherals */
//#include "gpio.h"
//#include "adc.h"

int main(void) {

	// Initialize domain clocks (CPU, HSB, PBA and PBB) to the max frequency available
	// without flash wait states.
	// Some of the registers in the GPIO module are mapped onto the CPU local bus.
	// To ensure maximum transfer speed and cycle determinism, any slaves being
	// addressed by the CPU on the local bus must be able to receive and transmit
	// data on the bus at CPU clock speeds. The consequences of this is that the
	// GPIO module has to run at the CPU clock frequency when local bus transfers
	// are being performed => we want fPBA = fCPU.
	fcpu_fpba_configure();

	MAX7219Init();
	MAX7219Write(REG_SCAN_LIMIT, 5);       // 5 digit scan

	// Loop forever and display all segments with brightness of
	// 3 and 15 alternatively.
	unsigned char brightness_levels[2] = {3,15};
	unsigned char index = 0;
	int count = 0;
	while (1) {
      // No button pressed.  Light up everything on the display.
	  count++;
	  if (count >= 32767) {
	    index = index ^ 1;
	    count = 0;
	  }
	  MAX7219SetBrightness(brightness_levels[index]);
      MAX7219DisplayChar(1, 'A', 0x80);
	  MAX7219DisplayChar(2, 'B', 0x80);
	  MAX7219DisplayL123(L1 | L2 | L3);
	  MAX7219DisplayChar(4, 'C', 0x80);
	  MAX7219DisplayChar(5, 'D', 0x80);
	}
	return 0;
}
