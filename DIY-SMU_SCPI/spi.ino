/*  Contains SPI and SPI devices
  SPI is bit-bang for now
  AD7190 24 bit 4 channel ADC
  AD5686 Quad 16b DAC
  initDac() External ref, x2 gain, on....
  setDac(int chan, int value)
  Control Register 2x74HC595. Send 16b data. Latches on rising edge of /SS2
  
*/

//#define ADCTIM 42       // Between 1 and 1023. ADC Convert time is .8ms * ADCSPD.  21 is 1 PLC: 16.6ms
#define ADCTIM 42       // If large > 100, need to increase ADC time-out. 

// AD7190 ADC channel input select. Can use multiple bits, but need to read out all. 
#define AD7190_AIN1_AIN2  0x01
#define AD7190_AIN3_AIN4  0x02
#define AD7190_AIN1_COM   0x10
#define AD7190_AIN2_COM   0x20
#define AD7190_AIN3_COM   0x40
#define AD7190_AIN4_COM   0x80
// AD7190 Registers
#define AD7190_MODE_REG_W 0x08
#define AD7190_MODE_REG_R 0x48
#define AD7190_CONF_REG_W 0x10
#define AD7190_CONF_REG_R 0x50
#define AD7190_DATA_REG_R 0x58

/* Assemble and set 16 control bits from instrument settings. No need to pass parameters. */
void setCtrlReg(void){
  unsigned int idata = CTL_I_1MA;
  unsigned int vdata = CTL_15V;
  unsigned int mdata = CTL_VMODE;
  unsigned int ondata = 0;
  unsigned int remdata = 0;

  if(instr.vRange == VRange_1_5) vdata = CTL_1_5V;
  if(instr.vRange == VRange_15)  vdata = CTL_15V;
  if(instr.vRange == VRange_150) vdata = CTL_150V;
  
  if(instr.iRange == IRange_1uA) idata = CTL_I_1UA;
  if(instr.iRange == IRange_10uA) idata = CTL_I_10UA;
  if(instr.iRange == IRange_100uA) idata = CTL_I_100UA;
  if(instr.iRange == IRange_1mA) idata = CTL_I_1MA;
  if(instr.iRange == IRange_10mA) idata = CTL_I_10MA;
  if(instr.iRange == IRange_100mA) idata = CTL_I_100MA;   
  
  if(instr.mode == FV) mdata = CTL_VMODE;           // Set the mode control bits
  if(instr.mode == FI) mdata = CTL_IMODE;
  
  if(instr.onOff == 1) {                            // Remote sense only if Output is ON
    ondata = CTL_ON;
    if(instr.remSense == 1) remdata = CTL_REMOT;
  }
  else remdata = 0;
  
  
  spi16W(mdata | ondata | remdata | vdata | idata | mdata);       // Send to control register
}

/*  Low-level Bit-bang 16 bit SPI Write for 16 bit 74HC595 control register
    Data is clocked in on + edge of CK\. MSB first. Assert SS2/, clock out 16 bits, de-assert /SS2
*/
void spi16W(unsigned int data){ //For Control Register
  int i;
  digitalWrite(SPI_CK, 0);    // May or may not be in this state
  digitalWrite(SPI_MOSI, 0);
  digitalWrite(SPI_SS2, 1);
  digitalWrite(SPI_SS2, 0);
  
  for (i = 0; i < 16; i++)
  {
    if (data & 0x8000) digitalWrite(SPI_MOSI, 1);  // MOSI = 1
    else digitalWrite(SPI_MOSI, 0);                 // MOSI = 0
    digitalWrite(SPI_CK, 1);
    digitalWrite(SPI_CK, 0);
    data = data << 1;                               // Shift up
  }
  digitalWrite(SPI_SS2, 1);
}

/* Set AD5686 DAC: 4 bits COMMAND, 4 bits Channel, 16 bits data 
    24 bits, clocked in on falling edge of SCK - clock default HI
    4 bits Command, 4 bits address, 16 bits data */
void initDac(void){
  spi24W(SETPWR_CMD << 20 ); // 0 to enable DACs 
  spi24W(SETREF_CMD << 20 | 0L << 16 | 1L); // 1 is external reference
//  spi24W(SETREF_CMD << 20 | 0L << 16 | 0L); // 1 is external reference
}

void setDac(long chan, long code){
  spi24W((SETDAC_CMD << 20) | (chan << 16) | code);
}


/* Set clamps to values in display: SetDisp[]
 *  This clips to DAC limits. Clipping to display limits is one level up. 
 *  Scaling to DAC nominal 
 *  no calibration yet, will add Force cal values which compensate for all but DAC and Error amp errors
 *  Needs to be run when mode is changed since DAC function is changed 
 *  Note Rev1 has Clamp DACs swapped. Fixed by swapping #defines for force DACs 
 *  Need to correct Schematic (label only) and PCB TP labels   
 */
void setClamps(void) {
  long dacp=0, dacn=0;
  
  if (instr.mode == FV){   // FV, Clamp I
    dacp = (long)(setVal[DCLIP].lSet * (DAC_MAX/2) / (CL_NOM * CDISPIMAX)) + (DAC_MAX/2);  // +CDISPIMAX maps to FFFF  
    dacn = (long)(setVal[DCLIN].lSet * (DAC_MAX/2) / (CL_NOM * CDISPIMAX)) + (DAC_MAX/2);  
  }
  if (instr.mode == FI){
    dacp = (long)(setVal[DCLVP].lSet * (DAC_MAX/2) / (CL_NOM * CDISPVMAX)) + (DAC_MAX/2);  
    dacn = (long)(setVal[DCLVN].lSet * (DAC_MAX/2) / (CL_NOM * CDISPVMAX)) + (DAC_MAX/2);
  } 
  setDac(DAC_CLP, dacClip(dacp));     // Clip and set DACs
  setDac(DAC_CLN, dacClip(dacn)); 
}

long dacClip(long dac){
  if (dac > DAC_MAX) dac = DAC_MAX;
  if (dac < 0) dac = 0;
  return dac;
}
  
/*  Low-level Bit-bang 24 bit SPI Write for AD5686 quad 16b DAC.
    Data is clocked in on - edge of CK\
    Assert SS/, clock out 24 bits, de-assert /SS
*/
void spi24W(long data){      //For AD5686 DAC
  int i;
  digitalWrite(SPI_CK, 1);    // May or may not be in this state
  digitalWrite(SPI_MOSI, 0);
  digitalWrite(SPI_SS0, 1);
  digitalWrite(SPI_SS0, 0);
  for (i = 0; i < 24; i++)
  {
    if (data & 0x800000l) digitalWrite(SPI_MOSI, 1);  // MOSI = 1
    else digitalWrite(SPI_MOSI, 0);                 // MOSI = 0
    digitalWrite(SPI_CK, 0);
    digitalWrite(SPI_CK, 1);
    data = data << 1;                               // Shift up
  }
  digitalWrite(SPI_SS0, 1);
}

void initAdc (char chan){
  //Serial.println("initAdc()");
  AD7190Res();                                   // HW reset
  // Set up Config and Mode register channels
  if (chan == 1)    chan = AD7190_AIN2_COM;
  else chan = AD7190_AIN1_COM;
//spi8W24W(AD7190_CONF_REG_W, 0x000000L | (chan <<8), 1);     //Config register Ain1, Ref1, x1, Buf OFF 
  spi8W24W(AD7190_CONF_REG_W, 0x000010L | (chan <<8), 1);     //Buf ON Bit 4 0x000010,  Chop bit 23 0x800000
  spi8W24W(AD7190_MODE_REG_W, 0x280000L | (ADCTIM & 0x3FF), 1);   //Mode command, No status (24b), Single, int clock
  spiRData(AD7190_DATA_REG_R);           // Read data from previous register set. 
  //Serial.print("Init ADC: ");
  //Serial.println(rData, HEX);   
}

long getAdc (char chan){          // 0 is VM, 1 is IM. 2-3 unused
  long rData;
  if (chan == 1)    chan = AD7190_AIN2_COM;
  else chan = AD7190_AIN1_COM;
//spi8W24W(AD7190_CONF_REG_W, 0x000000L | (chan << 8), 1);     //Config register Ain1, Buf Off
  spi8W24W(AD7190_CONF_REG_W, 0x000010L | (chan << 8), 1);     //Config register Ain1,Buf ON
  spi8W24W(AD7190_MODE_REG_W, 0x280000L | (ADCTIM & 0x3FF), 1);      
  rData = spiRData(AD7190_DATA_REG_R);              // Read data. 
  rData = -(rData - 8388608L);                       // Correct sign and add offset
//  Serial.print("getAdc(): ");
//  Serial.println(rData); 
  return rData;
}

void spi8W24W(char cmd, long data, char term) { // For AD7190 ADC
  int i;
  digitalWrite(SPI_CK, 1);    // May or may not be in this state
  digitalWrite(SPI_MOSI, 0);
  digitalWrite(SPI_SS1, 1);
  digitalWrite(SPI_SS1, 0);
  for (i = 0; i < 8; i++)
  {
    if (cmd & 0x80) digitalWrite(SPI_MOSI, 1);      // MOSI = 1
    else digitalWrite(SPI_MOSI, 0);                 // MOSI = 0
    digitalWrite(SPI_CK, 0);
    digitalWrite(SPI_CK, 1);
    cmd = cmd << 1;                               // Shift up
  }
  for (i = 0; i < 24; i++)
  {
    if (data & 0x00800000L) digitalWrite(SPI_MOSI, 1);  // MOSI = 1
    else digitalWrite(SPI_MOSI, 0);                 // MOSI = 0
    digitalWrite(SPI_CK, 0);
    digitalWrite(SPI_CK, 1);
    data = data << 1;                               // Shift up
  }  
  if (term == 1) digitalWrite(SPI_SS1, 1);
}

// Read 24b data from AD7190 ADC. Send 8 bit read command, wait for MISO low, read 24 bits
// This should be non-blocking since it is the longest delay in loop() and slows down other functions. 
long spiRData(char cmd){
  int i;
  int miso = 1; 
  long rData = 0L;
  int timer = 0;
  digitalWrite(SPI_CK, 1);    // May or may not be in this state
  digitalWrite(SPI_MOSI, 0);
  digitalWrite(SPI_SS1, 1);
  digitalWrite(SPI_SS1, 0);
  #define DELAY 5              // Timeout loop time in mS
  while (miso == 1)            // Wait for conv done. Using timeout 
  {
    delay(DELAY);              // ADC convert time is about 0.8mS * register
    miso = digitalRead(SPI_MISO);
    timer ++;
    if (timer > (400/DELAY)){        // .4 sec is plenty for ADC. 
      miso = 0;                       // Force MISO to 0
      Serial.println("ADC Timeout");
    }
    //Serial.print(".");      // for debug, print one dot per loop
  }
  //Serial.println();         // for debug
  // Serial.print("ADCms:");     // for debug: ADC time in ms
  //Serial.println(timer * DELAY);
  
  for (i = 0; i < 8; i++)   // Send cmd
  {
    if (cmd & 0x80) digitalWrite(SPI_MOSI, 1);  // MOSI = 1
    else digitalWrite(SPI_MOSI, 0);             // MOSI = 0
    digitalWrite(SPI_CK, 0);
    digitalWrite(SPI_CK, 1);
    cmd = cmd << 1;                             // Shift up
  }
   
  digitalWrite(SPI_MOSI, 0);                    // MOSI = 0 for remaining cycles. 
    
  for (i = 0; i < 24; i++)
  {
    digitalWrite(SPI_CK, 0);                    // Data output on CK H-L
    miso = digitalRead(SPI_MISO);               // Read MISO
    rData = rData << 1;                         // Shift first 
    if (miso == 1)                              // OR in a 1
      rData |= 1L;
    
    digitalWrite(SPI_CK, 1);                                       
  }  
  digitalWrite(SPI_SS1, 1);
  return rData;
}

long spi8W24R(char cmd)  // Read 24b register from AD7190 ADC. Send 8 bit read command,  read 24 bits
{
  int i;
  int miso = 1; 
  long rData = 0L;
//  int timer = 0;
//  int command;
//  command = (int)cmd;   //Put aside
  
  digitalWrite(SPI_CK, 1);    // May or may not be in this state
  digitalWrite(SPI_MOSI, 0);
  digitalWrite(SPI_SS1, 1);
  digitalWrite(SPI_SS1, 0);

  for (i = 0; i < 8; i++)   // Send cmd
  {
    if (cmd & 0x80) digitalWrite(SPI_MOSI, 1);  // MOSI = 1
    else digitalWrite(SPI_MOSI, 0);             // MOSI = 0
    digitalWrite(SPI_CK, 0);
    digitalWrite(SPI_CK, 1);
    cmd = cmd << 1;                             // Shift up
  }

  digitalWrite(SPI_MOSI, 0);                    // MOSI = 0 for remaining cycles. 
    
  for (i = 0; i < 24; i++)
  {
    digitalWrite(SPI_CK, 0);                    // Data output on CK H-L
    miso = digitalRead(SPI_MISO);               // Read MISO
    rData = rData << 1;                         // Shift first 
    if (miso == 1)                              // OR in a 1
      rData |= 1L;
    
    digitalWrite(SPI_CK, 1);                                       
  }  
  digitalWrite(SPI_SS1, 1);
  
  //Serial.print("Reg ");
  //Serial.print(command, HEX);
  //Serial.print(" = ");
  Serial.println(rData, HEX);
    
  return rData;
}


void AD7190Res(void)  // Send 40 consecutive 1's to reset 
{
  int i;
  digitalWrite(SPI_CK, 1);    // May or may not be in this state
  digitalWrite(SPI_SS1, 1);
  digitalWrite(SPI_SS1, 0);
  digitalWrite (SPI_MOSI, 1);  // MOSI = 1
  for (i = 0; i < 40; i++)   // Send 40 ones to reset
  {
    digitalWrite(SPI_CK, 0);
    digitalWrite(SPI_CK, 1);
  }
  digitalWrite(SPI_SS1, 1);
  //Serial.println("AD7190 Reset Complete");
  delay(20); // Teensy seems to like this delay, without it reset sometimes fails

}
