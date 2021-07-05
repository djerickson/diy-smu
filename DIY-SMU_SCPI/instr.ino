

void reset(void) {
  setVal[DFORCE].lSet = 0L;  
  setOutput(OFF);
  setVRange(VRange_15);
  setIRange(IRange_1mA);
  setRemoteSense(OFF);
  dispRange();

}

// Set Output ON or OFF
void setOutput(char on_off){
    instr.onOff = on_off;
    setCtrlReg();  
    dispOnOff();
 }

// Set remote sense ON or OFF
void setRemoteSense(char remote){
    instr.remSense = remote;
    setCtrlReg();
 }

// Note Nextion updates the range display itself, but SCPI has to add dispRange() 
void setVRange (byte range){
    instr.vRange = range;
    setCtrlReg();
 }

void setIRange (byte range){
    instr.iRange = range;
    setCtrlReg();
 }

void setMode (byte mode){
    instr.mode = mode;
    setCtrlReg();
    if (instr.mode == FV) myNex.writeStr("b11.txt", "FVMI"); 
    if (instr.mode == FI) myNex.writeStr("b11.txt", "FIMV");  
 }

void getVMeasure(void){    // Get VM
  adcV = (float)getAdc(0) * Range[instr.vRange].Mfull * MV_NOM / 8388608.0;       // convert counts to volts, nominal
  adcV = (adcV - Range[instr.vRange].MOffsCal) / Range[instr.vRange].MGainCal;    // Offset and gain Cal
  instr.mv = adcV;
//  Serial.print("VM: ");   
//  Serial.println(adcV,6);           // Print Float MV
}

void getIMeasure(void) {   // Get IM
  adcI = (float)getAdc(1) * Range[instr.iRange].Mfull * MI_NOM / 8388608.0;       // convert counts to amps
  adcI = (adcI - Range[instr.iRange].MOffsCal) / Range[instr.iRange].MGainCal;    // Offset and gain Cal
  instr.mi = adcI;  // Store real value
//  Serial.print("IM: ");
//  Serial.println(adcI, 9);    
}

/* Set Force DAC 
  Integer setting setVal[setIndex].lSet 
  Convert to Float (scale by range),   Apply cal offset and gain factors
  Convert to DAC, Clip to DAC limits, Send to DAC
 */
void setForce(void){
  long DacBits;       
  
  if (instr.mode == FV){
    if (setVal[setIndex].lSet > FDISPVMAX) setVal[setIndex].lSet = FDISPVMAX;    // Clip Iset 
    if (setVal[setIndex].lSet < FDISPVMIN) setVal[setIndex].lSet = FDISPVMIN; 
    dispForce(setVal[setIndex].lSet);
    fIset = (float)setVal[setIndex].lSet  / (Range[instr.vRange].F_ItoF_Scale);     // Convert to Float, scale to FP to create real voltage
    fIset = (fIset - Range[instr.vRange].FOffsCal) / (FV_NOM * Range[instr.vRange].FGainCal) ; // nominal and Cal Force voltage DAC
    // Scale DAC, add rounding.
    DacBits = (long)((fIset * ((float)DAC_MAX/2.0) * Range[instr.vRange].F_ItoF_Scale / FDISPVMAX) + 32767.0 + 0.5); 
    // Clip DAC BITS
    if (DacBits < 0) {
      DacBits = 0 ;
    }
  }
  else{         // FI: Force current 
    if (setVal[setIndex].lSet > FDISPIMAX) setVal[setIndex].lSet = FDISPIMAX;    // Clip Iset 
    if (setVal[setIndex].lSet < FDISPIMIN) setVal[setIndex].lSet = FDISPIMIN;    
    dispForce(setVal[setIndex].lSet);
    fIset = (float)setVal[setIndex].lSet  / (Range[instr.iRange].F_ItoF_Scale );     // Convert to Float, scale to FP to create real voltage
    fIset = (fIset - Range[instr.iRange].FOffsCal) / (FI_NOM * Range[instr.iRange].FGainCal) ; // nominal and Cal Force voltage DAC
    // Scale DAC, add rounding.
    DacBits = (long)((fIset * ((float)DAC_MAX/2.0) * Range[instr.iRange].F_ItoF_Scale / FDISPIMAX) + 32767.0 + 0.5); 
  }
  // Clip DAC BITS
  if (DacBits < 0) DacBits = 0 ;
  if (DacBits > DAC_MAX) DacBits = DAC_MAX;
  setDac(DAC_FORCE, DacBits);   // Set Force DAC
}


void nDispClamps(){   // Clip, Display, and set DACs
#define CLAMPDIFF 1000
// Clip to display limits
  if (setVal[DCLIP].lSet > CDISPIMAX) setVal[DCLIP].lSet = CDISPIMAX;
  if (setVal[DCLIP].lSet < CDISPIMIN) setVal[DCLIP].lSet = CDISPIMIN;
  
  if (setVal[DCLIN].lSet > CDISPIMAX) setVal[DCLIN].lSet = CDISPIMAX;
  if (setVal[DCLIN].lSet < CDISPIMIN) setVal[DCLIN].lSet = CDISPIMIN;
  
  if (setVal[DCLVP].lSet > CDISPVMAX) setVal[DCLVP].lSet = CDISPVMAX;
  if (setVal[DCLVP].lSet < CDISPVMIN) setVal[DCLVP].lSet = CDISPVMIN;
  
  if (setVal[DCLVN].lSet > CDISPVMAX) setVal[DCLVN].lSet = CDISPVMAX;
  if (setVal[DCLVN].lSet < CDISPVMIN) setVal[DCLVN].lSet = CDISPVMIN;

  // + Clamp must be > - clamp by some margin. Prevent + from getting too close to -
  if (setVal[DCLIP].lSet - setVal[DCLIN].lSet < CLAMPDIFF)  setVal[DCLIP].lSet =  setVal[DCLIN].lSet + CLAMPDIFF;
  if (setVal[DCLVP].lSet - setVal[DCLVN].lSet < CLAMPDIFF)  setVal[DCLVP].lSet =  setVal[DCLVN].lSet + CLAMPDIFF;
  
  if (nPage == Clamp){ 
    myNex.writeNum("clip.val", setVal[DCLIP].lSet);
    myNex.writeNum("clin.val", setVal[DCLIN].lSet);
    myNex.writeNum("clvp.val", setVal[DCLVP].lSet);
    myNex.writeNum("clvn.val", setVal[DCLVN].lSet);
  }
  setClamps();
}

void initInstr(void){
  
/* Initialize the data structures, calibration defaults and settings*/
// Init common struct values
//  for (i = VRange_1_5; i < (VRange_150 + 1); i++){    // for all V ranges, set min, max, init
//    Range[i].FDispMax = 150000L;       // Max display in integers
//    Range[i].FDispMin = -150000L;      // Min display in integers
//    Range[i].FDispInit = 0L;           // Init Value
//  }
//  for (i = IRange_1uA; i < (IRange_100mA + 1); i++){    // for all I ranges, set min, max, init
//    Range[i].FDispMax = 105000L;       // Max display in integers
//    Range[i].FDispMin = -105000L;      // Min display in integers
//    Range[i].FDispInit = 0L;           // Init Value
//  }

  setCal(ON);         // Set board calibration factors
  
  // Initialize the Force display Scale factors. Some are over 32 bits, so long long
  // All I and V Force ranges display 5.5 digits
  Range[VRange_1_5].F_ItoF_Scale    = 100000LL;
  Range[VRange_15].F_ItoF_Scale     = 10000LL;
  Range[VRange_150].F_ItoF_Scale    = 1000LL;
  Range[IRange_1uA].F_ItoF_Scale    = 100000000000LL;
  Range[IRange_10uA].F_ItoF_Scale   = 10000000000LL; // 10.0000 = 10uA
  Range[IRange_100uA].F_ItoF_Scale  = 1000000000LL;  // 100.000 = 100uA
  Range[IRange_1mA].F_ItoF_Scale    = 100000000LL;   // 1.00000 - 1mA
  Range[IRange_10mA].F_ItoF_Scale   = 10000000LL;    // 10.0000 = .01A
  Range[IRange_100mA].F_ItoF_Scale  = 1000000LL;     // 100.000 = .1A
  // Initialize  the Measure display Scale factors
  Range[VRange_1_5].M_ItoF_Scale    = 100000L;
  Range[VRange_15].M_ItoF_Scale     = 10000L;
  Range[VRange_150].M_ItoF_Scale    = 1000L;
  Range[IRange_1uA].M_ItoF_Scale    = 100000L; //All current ranges display 5.5 digits
  Range[IRange_10uA].M_ItoF_Scale   = 100000L;
  Range[IRange_100uA].M_ItoF_Scale  = 100000L;
  Range[IRange_1mA].M_ItoF_Scale    = 100000L;
  Range[IRange_10mA].M_ItoF_Scale   = 100000L;
  Range[IRange_100mA].M_ItoF_Scale  = 100000L;
  // Initialize  the Force display decimal point
  Range[VRange_1_5].ForceDP   = 5;
  Range[VRange_15].ForceDP    = 4;
  Range[VRange_150].ForceDP   = 3;
  Range[IRange_1uA].ForceDP   = 5;
  Range[IRange_10uA].ForceDP  = 4;
  Range[IRange_100uA].ForceDP = 3;
  Range[IRange_1mA].ForceDP   = 5;
  Range[IRange_10mA].ForceDP  = 4;
  Range[IRange_100mA].ForceDP = 3;
  // Initialize  the Measure display decimal point: one more digit
  Range[VRange_1_5].MeasDP    = 5;
  Range[VRange_15].MeasDP     = 4;
  Range[VRange_150].MeasDP    = 3;
  Range[IRange_1uA].MeasDP    = 5;
  Range[IRange_10uA].MeasDP   = 4;
  Range[IRange_100uA].MeasDP  = 3;
  Range[IRange_1mA].MeasDP    = 5;
  Range[IRange_10mA].MeasDP   = 4;
  Range[IRange_100mA].MeasDP  = 3;
  // Initialize  the Measure display units
  Range[VRange_1_5].RangeUnit    = (char*)"V";
  Range[VRange_15].RangeUnit     = (char*)"V";
  Range[VRange_150].RangeUnit    = (char*)"V";
  Range[IRange_1uA].RangeUnit    = (char*)"uA";
  Range[IRange_10uA].RangeUnit   = (char*)"uA";
  Range[IRange_100uA].RangeUnit  = (char*)"uA";
  Range[IRange_1mA].RangeUnit    = (char*)"mA";
  Range[IRange_10mA].RangeUnit   = (char*)"mA";
  Range[IRange_100mA].RangeUnit  = (char*)"mA";
  // Initialize  the Range Labels
  Range[VRange_1_5].RangeLabel   = (char*)"1.5V";
  Range[VRange_15].RangeLabel    = (char*)"15V";
  Range[VRange_150].RangeLabel   = (char*)"150V";
  Range[IRange_1uA].RangeLabel   = (char*)"1uA";
  Range[IRange_10uA].RangeLabel  = (char*)"10uA";
  Range[IRange_100uA].RangeLabel = (char*)"100uA";
  Range[IRange_1mA].RangeLabel   = (char*)"1mA";
  Range[IRange_10mA].RangeLabel  = (char*)"10mA";
  Range[IRange_100mA].RangeLabel = (char*)"100mA";
  // Initialize the Measure full scale range
  Range[VRange_1_5].Mfull        = 15.0;        // Same as 15V: no ADC scaling
  Range[VRange_15].Mfull         = 15.0;
  Range[VRange_150].Mfull        = 150.0;
  Range[IRange_1uA].Mfull        = 0.000001;
  Range[IRange_10uA].Mfull       = 0.00001;
  Range[IRange_100uA].Mfull      = 0.0001;
  Range[IRange_1mA].Mfull        = 0.001;
  Range[IRange_10mA].Mfull       = 0.01;;
  Range[IRange_100mA].Mfull      = 0.1;

  // Initialize the Clamp values
  setVal[DCLIP].lSet = CDISPIINITP;
  setVal[DCLIN].lSet = CDISPIINITN;
  setVal[DCLVP].lSet = CDISPVINITP;
  setVal[DCLVN].lSet = CDISPVINITN;
  nDispClamps();
}
