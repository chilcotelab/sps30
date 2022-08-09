/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
//including headers
#include <stdio.h>
#include "sensirion_uart.h"
#include "sps30.h"

//main
int main(void) {
    //declarations
    struct sps30_measurement m;
    char serial[SPS30_MAX_SERIAL_LEN];
    const uint8_t AUTO_CLEAN_DAYS = 5;
    int16_t ret;

   //opening UART
    while (sensirion_uart_open() != 0) {
        printf("UART init failed\n");
        sensirion_sleep_usec(1000000); // sleep for 1s
    }

    //Busy loop for initialization, because the main loop does not work 
    //without a sensor.
    while (sps30_probe() != 0) {
        printf("SPS30 sensor probing failed\n");
        sensirion_sleep_usec(1000000); // sleep for 1s
    }
    printf("SPS30 sensor probing successful\n");

    //reading device information
    struct sps30_version_information version_information;
    ret = sps30_read_version(&version_information);
    if (ret) {
        printf("error %d reading version information\n", ret);
    } else {
        printf("Firmware: %u.%u Hardware: %u, SHDLC: %u.%u\n",
               version_information.firmware_major,
               version_information.firmware_minor,
               version_information.hardware_revision,
               version_information.shdlc_major,
               version_information.shdlc_minor);
    }

    //reading device serial
    ret = sps30_get_serial(serial);
    if (ret)
        printf("error %d reading serial\n", ret);
    else
        printf("SPS30 Serial: %s\n", serial);

    //setting auto cleaning interval
    ret = sps30_set_fan_auto_cleaning_interval_days(AUTO_CLEAN_DAYS);
    if (ret)
        printf("error %d setting the auto-clean interval\n", ret);

    //starting measurement
    while (1) {
        ret = sps30_start_measurement();
         if (ret < 0) {
            printf("error starting measurement\n");
        }

        printf("measurements started\n");
        
        //creating and opening a .csv file in the current directory 
        FILE *fpt;
        
        fpt = fopen("sps30_data.csv", "w+");
      
        //printing particle number counts (#/cm^3)
        fprintf(fpt, "nc0.5, nc1.0, nc2.5, nc4.5, nc10.0\n");
        
        //setting how many seconds to measure for (1 measurement per second)
        for (int i = 0; i < 180; ++i) {
            //reading measurements
            ret = sps30_read_measurement(&m);
            if (ret < 0) {
                printf("error reading measurement\n");
                fprintf(fpt, "\t, \t, \t, \t, \t\n");
            } else {
                if (SPS30_IS_ERR_STATE(ret)) {
                    printf(
                        "Chip state: %u - measurements may not be accurate\n",
                        SPS30_GET_ERR_STATE(ret));
                }
                //printing measurements to .csv
                fprintf(fpt, "%0.2f, %0.2f, %0.2f, %0.2f, %0.2f\n",
                    m.nc_0p5, m.nc_1p0, m.nc_2p5, m.nc_4p0, m.nc_10p0);
            }
            sensirion_sleep_usec(1000000); //sleep for 1s
        }
        //closing .csv
        fclose(fpt);

        //Stopping measurement to preserve power. Also enter sleep mode 
        //if the firmware version is >=2.0.
        ret = sps30_stop_measurement();
        if (ret) {
            printf("Stopping measurement failed\n");
        }

        if (version_information.firmware_major >= 2) {
            ret = sps30_sleep();
            if (ret) {
                printf("Entering sleep failed\n");
            }
        }

        printf("Stopping measurements...\n");
        sensirion_sleep_usec(1000000 * 30); //setting how many seconds to pause
        
        printf("Turning back on\n");
        if (version_information.firmware_major >= 2) {
            ret = sps30_wake_up();
            if (ret) {
                printf("Error %i waking up sensor\n", ret);
            }
        }
    }
    //closing UART
    if (sensirion_uart_close() != 0)
        printf("failed to close UART\n");

    return 0;
}
