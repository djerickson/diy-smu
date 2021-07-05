
// Set known Cal factors for each board
// Curently these are done manually

/* CAL Factor Storage in EEPROM
Use SCPI CAL:VOLT  INDEX, VALUE
INDEX is Cal factor, a 4 byte float
Value is the cal value
Offsets shold be in the mV range, Gains should be 1.0 +/- 5%

1.5V range CAL:
  0 is VF_GAIN, 1 is VF_OFFSET
  2 is VM_GAIN, 3 is VM_OFFSET
15V range CAL:
  4 is VF_GAIN, 5 is VF_OFFSET
  6 is VM_GAIN, 7 is VM_OFFSET
............

Currents start 

*/
#define BOARD2    // Select which board for CAL
#define CAL       // Turn off to disable cal. Or use SCPI CALibrate:STATe OFF

void setCal(byte calOn){
int i;
  // Set default (no) calibration for all ranges
  for(i = 0; i < NUM_RANGES; i++){
    Range[i].FOffsCal = 0.0;
    Range[i].FGainCal = 1.0;
    Range[i].MOffsCal = 0.0;
    Range[i].MGainCal = 1.0;
  }
  if (calOn == ON){
  //Serial.println("Calibration Values ON");
#ifdef CAL
#ifdef BOARD1
// Cal factors are from linear regression of the error, as measured by the DMM. 
// For VF, offset cal is the negative of the offset term. Gain is 1- Gain term 
// For VM, offset cal is the offset term. Gain is 1- Gain term 

  // 1.5 V Range Cal
  Range[ VRange_1_5 ].FOffsCal = -0.00001432 ;
  Range[ VRange_1_5 ].FGainCal =  0.98607569 ;
  Range[ VRange_1_5 ].MOffsCal = -0.00177517 ;
  Range[ VRange_1_5 ].MGainCal =  1.00218660 ;
  // 15 V Range Cal
  Range[ VRange_15 ].FOffsCal = -0.00199495 ;
  Range[ VRange_15 ].FGainCal =  0.99282880 ;
  Range[ VRange_15 ].MOffsCal = -0.00176334 ;
  Range[ VRange_15 ].MGainCal =  1.00221653 ;
  // 150 V Range Cal
  Range[ VRange_150 ].FOffsCal = -0.02144985 ;
  Range[ VRange_150 ].FGainCal =  1.01228148 ;
  Range[ VRange_150 ].MOffsCal = -0.01642679 ;
  Range[ VRange_150 ].MGainCal =  0.98295755 ;


  // I Measure
  Range[IRange_100mA].MOffsCal = -0.000062;       // 100mA range
  Range[IRange_100mA].MGainCal =  0.97540;        // using 34401 DMM, I range
  Range[IRange_10mA]. MOffsCal = -0.0000062;      // 10mA range
  Range[IRange_10mA]. MGainCal =  0.97307;        // using 1K load
  Range[IRange_1mA].  MOffsCal = -0.00000063;     // 1mA range
  Range[IRange_1mA].  MGainCal =  0.93867;        // using precision 10K load
  Range[IRange_100uA].MOffsCal = -0.000000063;    // 100uA range
  Range[IRange_100uA].MGainCal =  0.96352;        // using 100K load
  Range[IRange_10uA]. MOffsCal = -0.000000007;    // 10uA range
  Range[IRange_10uA]. MGainCal =  0.96470;        // using 1MK load
  Range[IRange_1uA].  MOffsCal = -0.000000001;    // 1uA range
  Range[IRange_1uA].  MGainCal =  0.971;          // using 10M load (DMM)
  // I Force
  Range[IRange_100uA].FOffsCal = -0.000012;
  Range[IRange_100uA].FGainCal = 0.964528;
//  Range[IRange_1mA].  FOffsCal =  -0.000000112;
//  Range[IRange_1mA].  FGainCal = 0.964528;
//  Range[IRange_1mA].  FGainCal = 1.0931;
  Range[IRange_10mA]. FOffsCal = -0.00;
  Range[IRange_10mA]. FGainCal = 0.93036;
  Range[IRange_100mA].FOffsCal = -0.000012;
  Range[IRange_100mA].FGainCal = 0.92880;         // using 34401 DMM, I range
#endif // BOARD1

#ifdef BOARD2
  // Calibrated at 2021-05-30 11:15:03.479662
  // 1.5 V Range Cal
  Range[VRange_1_5].FGainCal =  0.98927955 ;
  Range[VRange_1_5].FOffsCal = -0.00046439 ;
  Range[VRange_1_5].MGainCal =  0.99851444 ;
  Range[VRange_1_5].MOffsCal = -0.00056178 ;
  // 15 V Range Cal
  Range[VRange_15 ].FGainCal =  0.99631107 ;
  Range[VRange_15 ].FOffsCal = -0.00236850 ;
  Range[VRange_15 ].MGainCal =  0.99851692 ;
  Range[VRange_15 ].MOffsCal = -0.00061017 ;
  // 150 V Range Cal
  Range[VRange_150].FGainCal =  1.01251234 ;
  Range[VRange_150].FOffsCal = -0.02451412 ;
  Range[VRange_150].MGainCal =  0.98254268 ;
  Range[VRange_150].MOffsCal = -0.00566919 ;

  // I Measure
  Range[IRange_10mA ].MOffsCal =  0.0000010;     // 10mA range
  Range[IRange_10mA ].MGainCal =  1.015360;      // using 34401 DMM, I range
  Range[IRange_100mA].MOffsCal =  -0.000008;
  Range[IRange_100mA].MGainCal =  1.02327;       // using 34401 DMM, I range
  Range[IRange_1mA  ].MOffsCal = -0.00000009;
  Range[IRange_1mA  ].MGainCal =  1.019894;      // using 34401 DMM, I range
  Range[IRange_100uA].MOffsCal = -0.000000009;
  Range[IRange_100uA].MGainCal =  0.9988;        // using 100K load, VM
  Range[IRange_10uA ].MOffsCal = -0.0000000009;
  Range[IRange_10uA ].MGainCal =  1.0;      // using 34401 DMM, I range
  // I Force
  Range[IRange_100mA].FOffsCal = +0.000014;
  Range[IRange_100mA].FGainCal = 0.95105;
  Range[IRange_10mA]. FOffsCal = -0.0000014;
  Range[IRange_10mA]. FGainCal = 0.95441;
  Range[IRange_1mA].  FOffsCal = -0.00000014;
  Range[IRange_1mA].  FGainCal = 0.95020;

// Calibrated at 2021-06-01 13:26:09.218018
// 0.001 V Range Cal
Range[IRange_1mA].FGainCal =  0.94893619 ;
Range[IRange_1mA].FOffsCal = -0.00000012 ;
Range[IRange_1mA].MGainCal =  1.02142671 ;
Range[IRange_1mA].MOffsCal =  0.00000003 ;
// Calibrated at 2021-06-01 14:08:42.481757
// 0.001 V Range Cal
Range[IRange_1mA].FGainCal =  0.94892843 ;
Range[IRange_1mA].FOffsCal = -0.00000012 ;
Range[IRange_1mA].MGainCal =  1.02143732 ;
Range[IRange_1mA].MOffsCal =  0.00000003 ;
#endif // BOARD2

#endif // CAL
  }
  else {
    //Serial.println("Calibration Values OFF");
  }
}
