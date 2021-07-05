/*
 * Basic SMU GUI, D. Erickson
 *  9/26/2020 Got triggers working, multiple pages, basic GUI, basic plots
 *  9/28/2020 addt working for plots
 *  9/30/2020 tried basic menu lists for range selection
 * 11/14 2020 Sending to objects while on wrong page causes page change commands to be missed.  Added "if (nPage==Main)" 
 * Copyright (c) 2020 Athanasios Seitanis < seithagta@gmail.com >.
 * https://www.seithan.com 

  }
*/

//int mode = 0;
//int inc = 10000;
int sel = 4;

void NexSetup(void){
  setIndex = 0;
  myNex.begin(38400);            // Begin Nextion object with baud rate
  pinMode(LED_BUILTIN, OUTPUT);   // LED initialized as output     
  digitalWrite(LED_BUILTIN, LOW); // Set LED to LOW = off 
  myNex.writeNum("vf.val", setVal[setIndex].lSet);  
}  // End of NexSetup()

void dispRange(void){     // Update range display on Page Main
  int range;
  if (nPage == Main){
      range = instr.vRange;
      myNex.writeStr("vr.txt", Range[range].RangeLabel);
      range = instr.iRange;
      myNex.writeStr("ir.txt", Range[range].RangeLabel);
  }        
}

void dispForce(long val){   // Update VF
  int range;
  if (nPage == Main){
    if (instr.mode == FV){     // Display Force V, Main page
      range = instr.vRange;
      myNex.writeNum("vf.val", val); 
      myNex.writeNum("vf.vvs1", Range[range].ForceDP);
      myNex.writeStr("vfu.txt", Range[range].RangeUnit);
      myNex.writeStr("vfl.txt", "VF:");
    }
    if (instr.mode == FI){     // Display Force I
      range = instr.iRange;
      myNex.writeNum("vf.val", val); 
      myNex.writeNum("vf.vvs1", Range[range].ForceDP);
      myNex.writeStr("vfu.txt", Range[range].RangeUnit);
      myNex.writeStr("vfl.txt", "IF:");
    }        
  }
  setCtrlReg();
}

void dispMeas(void){      // Display Measure, Main page, Main display
  int range;
  long val;
  if (nPage == Main){
    if (instr.mode == FV){    // Measure I
      range = instr.iRange;
      val = (long)(instr.mi * Range[range].M_ItoF_Scale / Range[range].Mfull);
      myNex.writeNum("im.vvs1", Range[range].MeasDP);
      myNex.writeNum("im.val", val);
      myNex.writeStr("imu.txt", Range[range].RangeUnit);
      myNex.writeStr("iml.txt", "IM:");
    }
    if (instr.mode == FI){
      range = instr.vRange;   // Measure V
      val = (long)(instr.mv * Range[instr.vRange].M_ItoF_Scale);
      //Serial.println(val);
      myNex.writeNum("im.vvs1", Range[range].MeasDP);
      myNex.writeNum("im.val", val); 
      myNex.writeStr("imu.txt", Range[range].RangeUnit);
      myNex.writeStr("iml.txt", "VM:");
    } 
  }
}

void dispOnOff(void){      // Display ON OFF status, Main page
  if (nPage == Main){
    if (instr.onOff == ON){
      myNex.writeNum("b1.bco", 63488); // Set button b0 background color to RED 
      myNex.writeStr("b1.txt", "ON"); // Set button b0 text to "ON"
    }
    else {
      myNex.writeNum("b1.bco", 50712); // Set button b0 background color to grey 
      myNex.writeStr("b1.txt", "OFF"); // Set button b0 text to "ON"
    } 
  }
}

// Secondary measure display: Display Measure V if FV, I if FI, Main page
// Needa a HACK: do not update this display if the Current Range buttons are active. 
// It interferes with the current range display. Needs a trigger on current range button. 
void dispMeas2(void){   
  int range;
  long val;
  if (noDisp2 == 0){  
    if (nPage == Main){
      if (instr.mode == FV) {    // Measure V
        range = instr.vRange;
        val = (long)(instr.mv * Range[range].M_ItoF_Scale);              // Scale and convert to int for display
        myNex.writeNum("vm.vvs1", Range[range].MeasDP);
        myNex.writeNum("vm.val", val); 
        myNex.writeStr("vmu.txt", Range[range].RangeUnit);
        myNex.writeStr("vml.txt", "VM:");
      }
      if (instr.mode == FI) {    // Else MI
        range = instr.iRange;
        val = (long)(instr.mi * Range[range].M_ItoF_Scale / Range[range].Mfull);
        myNex.writeNum("vm.vvs1", Range[range].MeasDP);
        myNex.writeNum("vm.val", val);
        myNex.writeStr("vmu.txt", Range[range].RangeUnit);
        myNex.writeStr("vml.txt", "IM:");
      }
    }
  }
}

/* List of Nextion Button Triggers:
 *    1:    ON/OFF button
 *    2:    digitUp()
 *    3:    digit Down()
 *    4: 
 *    5: 
 *    6-8:  V ranges, 3
 *    9:    Set V range
 *    10:   Set I range
 *    11:   Mode button, scrolls thru the modes
 *    12:
 *    13:
 *    14:
 *    15:
 *    16-21: I ranges: 6  
 */

void trigger1(){
  /* ON/OFF button handler, toggles instr.onOff and LED.  printh 23 02 54 01 */
  if (instr.onOff == 0){     
    setOutput(1);
  }
  else{
    setOutput(0);
    //Serial.println("Trigger 1 OFF");
  }
  dispOnOff();
  digitalWrite(LED_BUILTIN, instr.onOff);
}

void trigger11(){
  /* MODE button. printh 23 02 54 11  Scrolls through the mode setings */
  //Serial.println("Trigger 11");
  instr.mode++;
  if (instr.mode > FI) instr.mode = FV;
  if (instr.mode == FV) myNex.writeStr("b11.txt", "FVMI"); 
  if (instr.mode == FI) myNex.writeStr("b11.txt", "FIMV");  
}

void trigger2(){
  /* UP button. printh 23 02 54 02  */
  //Serial.println("Trigger 2");
  digitUp();
  myNex.writeNum("vf.val", setVal[setIndex].lSet); 
}

void trigger3(){
  /* DOWN button. printh 23 02 54 03  */
  //Serial.println("Trigger 3");
  digitDown();
  myNex.writeNum("vf.val", setVal[setIndex].lSet); 
}

void trigger4(){  
  /* Left, same as "l" */
  //Serial.println("Trigger 4");
  digitLeft();
}

void trigger5(){  
  /* Right, same as "r" */
  //Serial.println("Trigger 5");
  digitRight();
}

void plot2(){
#define PLOTLEN 400
  unsigned char data[PLOTLEN];
  int i;
  int waveCount=0;
  
  myNex.writeStr("s0.vscope", "global");  // Set to global to make wave persistent
//for (j =0; j< 5; j++){
    for(i=0; i<PLOTLEN; i++){
      data[i] = (unsigned char)(100.0 + 50.0* sin((float)i / ((float)waveCount + 2.0)));
    }
    waveCount ++;
    myNex.writeNum("n0.val", waveCount); 
    plot(1, 0, PLOTLEN, data);  
//}
}

void trigger9(){
  // I Range button. 
  noDisp2 = 1;
  //Serial.println("Trig 9");
  delay (100);
  myNex.writeStr("ref ir3");
  myNex.writeStr("ref ir2");
 // myNex.writeStr("ref", "ir3");
 // myNex.writeStr("ref", "ir5");
}

void trigger10(){
  // V Range button.  
  noDisp2 = 1;
  //Serial.println("Trig 10");
}

// Page change triggers. 
// Had to move page changes to Arduino to be relaible. Don't know why. Still not perfect
void trigger12(){
  // Main: Next
  nPage = Clamp;
  NsetPage(nPage);  
  //Serial.print("Trig12 ");
  //Serial.println(nPage);
}
void trigger13(){ 
  // Clamp: Next
  nPage = Plot;
  NsetPage(nPage);  
  //Serial.print("Trig 13 ");
  //Serial.println(nPage);
//  plot2(); //Set plot data global, turn off refresh during plot update */
}

void trigger14(){
  // Plot: Next  
  nPage = Main;
  NsetPage(nPage);  
  //Serial.print("Trig 13 ");
  //Serial.println(nPage);  
}

// Trigger 6-8 set V range
void trigger6(){
  noDisp2 = 0;
  setVRange (VRange_1_5);
  //Serial.println("VR = 1.5");
}
void trigger7(){
  noDisp2 = 0;
  setVRange (VRange_15);
//  Serial.println("VR = 15");
}
void trigger8(){
  noDisp2 = 0;
  setVRange (VRange_150);
//    Serial.println("VR = 150");  
}

// Trigger 16-21 set I range
void trigger16(){
    noDisp2 = 0;
    setIRange (IRange_1uA);
//    Serial.println("IR = 1uA"); 
}
void trigger17(){
    noDisp2 = 0;
    setIRange (IRange_10uA);
//    Serial.println("IR = 10uA");  
}
void trigger18(){
    noDisp2 = 0;
    setIRange (IRange_100uA);
//    Serial.println("IR = 100uA");  
}
void trigger19(){
    noDisp2 = 0;
    setIRange (IRange_1mA);   
//    Serial.println("IR = 1mA");  
}
void trigger20(){
    noDisp2 = 0;
    setIRange (IRange_10mA);  
//    Serial.println("IR = 10mA");  
}
void trigger21(){
    noDisp2 = 0;
    setIRange (IRange_100mA);      
//    Serial.println("IR = 100mA");  
}

void plot(char id, char chan, int len, unsigned char data[]){
//  int i;
//  Nref_stop();
//  for(i=0; i<len; i++){
//    Nadd(id, chan, data[i]); 
//  }  
//  Nref_star();
  Naddt(id, chan, len, data);
}

// Add 1 point to plot using 'add'
void Nadd(char id,char chan, unsigned char data ){
  char cstr[8];
  itoa(data, cstr, 10);
  Serial1.write("add 1,0,"); 
  Serial1.write(cstr); 
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  Serial1.write(0xff); 
}

// Send array of data to plot using 'addt' 1,0,len,0xff 0xff 0xff then 5ms delay then raw data... 
// Note some errors, haven't found out why yet.
void Naddt(char id,char chan, int len, unsigned char data[] ){
  char cstr[8];
  int i;
  itoa(len, cstr, 10);
  Serial1.write("addt 1,0,"); 
  Serial1.write(cstr);            // send length
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  delay(5);
  for(i=0; i<len; i++){ 
    Serial1.write(data[i]);       // send data, binary
//    delay (1);
  }   
}

// Change display page
void NsetPage(char pageNum){
  char cstr[8];
  itoa(pageNum, cstr, 10);
  Serial1.write("page ");  
  Serial1.write(cstr);            // send page
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  Serial1.write(0xff);  
}

// Waveform controls
// Turn off waveform refresh
void Nref_stop(void){
  Serial1.write("ref_stop");  
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  Serial1.write(0xff);  
}
// Turn on waveform refresh
void Nref_star(void){
  Serial1.write("ref_star"); 
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  Serial1.write(0xff);
} 
// Clear waveform
void Ncle(void){
  Serial1.write("cle 1,0"); 
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  Serial1.write(0xff);
} 

// Draw a line line x1,y1,x2,y2,color
void nLine(int x1, int y1, int x2, int y2, int color){
  char cstr[8];
  itoa(x1, cstr, 10);
  Serial1.write("line "); 
  Serial1.write(cstr);
  Serial1.write(","); 
  itoa(y1, cstr, 10);
  Serial1.write(cstr);
  Serial1.write(",");
  itoa(x2, cstr, 10);
  Serial1.write(cstr);
  Serial1.write(",");
  itoa(y2, cstr, 10);
  Serial1.write(cstr);
  Serial1.write(",");
  itoa(color, cstr, 10);
  Serial1.write(cstr);  
  Serial1.write(0xff); 
  Serial1.write(0xff); 
  Serial1.write(0xff);
}

/*  Draw digit select cursor as a line under digit, taking into account decimal point
    But need to delete the old one first. 
    This is getting a bit ugly with 4 Clamp displays */
void nCursor(){
  unsigned char dw;        // Digit width
  unsigned char dpw;       // Decimal point width
  unsigned char h;         // Height of cursor in pixels
  int vpos;                // Vertical Pos
  int digPosH;
  int dp=0;                 // To avoid warning.
  int i,j;
  int hend, len, hpos;      // For Clamp display calculations
  int disp = 2;             // Which Clamp display, 0 based
  int dispVs = 40;          // Clamp display vertical spacing
  
  if (nPage == Main){  // Big display on Page 1
    dw = 30;
    dpw = 20;
    h = 3;
    vpos = 70;
    if (instr.mode == FV) dp = Range[instr.vRange].ForceDP;
    if (instr.mode == FI) dp = Range[instr.iRange].ForceDP;
    
    if (mainDigit < dp) digPosH = mainDigit * dw;
    if (mainDigit >= dp) digPosH = mainDigit * dw + dpw;
    for (i=0; i< h; i++){
      // Erase old cursor
      nLine(50, vpos+i, 300, vpos+i, 0);
      // Draw new one
      nLine(295-digPosH, vpos+i, 295-digPosH-dw, vpos+i, 65535);
    }  
  }
  if (nPage == Clamp){  // 4 displays on Page 2, smaller font
    dw = 15;
    dpw = 10;
    h = 3;
    disp = clampSel;
    
    hpos = 85;               // Left-most digit xpos + Width
    len = 150;
    hend = hpos + len - 5;
    dp = 4;
    if (clampDigit < dp) digPosH = clampDigit * dw;
    if (clampDigit >= dp) digPosH = clampDigit * dw + dpw;
    for (j=0; j<4; j++){              // Erase all old cursors. lazy
      vpos = 70 + j * dispVs;
      for (i=0; i< h; i++){
        nLine(hpos, vpos+i, hend, vpos+i, 0);
      }
    }
    vpos = 70 + disp * dispVs;
    for (i=0; i< h; i++){             // Draw new cursor
      nLine(hend-digPosH, vpos+i, hend-digPosH-dw, vpos+i, 65535);
    }
      
  }
}

void digitDown(void){
  int setIndex = 0;
  if (nPage == Clamp){   
    setIndex = clampSel + 1;             // setVal setIndex is 0 for Force, 1-4 for Clamps 
    if (clampDigit == 5) setVal[setIndex].lSet -= (long)count * 100000L;
    if (clampDigit == 4) setVal[setIndex].lSet -= (long)count * 10000L;
    if (clampDigit == 3) setVal[setIndex].lSet -= (long)count * 1000L;
    if (clampDigit == 2) setVal[setIndex].lSet -= (long)count * 100L;
    if (clampDigit == 1) setVal[setIndex].lSet -= (long)count * 10L;
    if (clampDigit == 0) setVal[setIndex].lSet -= (long)count * 1L;
 //   if (setVal[setIndex].lSet < CDISPVMIN) setVal[setIndex].lSet = CDISPVMIN;
    nDispClamps();
  }
  if (nPage == Main){
    setIndex = DFORCE;
    if (mainDigit == 5) setVal[setIndex].lSet -= (long)count * 100000L;
    if (mainDigit == 4) setVal[setIndex].lSet -= (long)count * 10000L;
    if (mainDigit == 3) setVal[setIndex].lSet -= (long)count * 1000L;
    if (mainDigit == 2) setVal[setIndex].lSet -= (long)count * 100L;
    if (mainDigit == 1) setVal[setIndex].lSet -= (long)count * 10L;
    if (mainDigit == 0) setVal[setIndex].lSet -= (long)count * 2L;
  }
}

void digitUp(void){
  int setIndex = 0;
  if (nPage == Clamp){   
    setIndex = clampSel + 1;       
    if (clampDigit == 5) setVal[setIndex].lSet += (long)count * 100000L;
    if (clampDigit == 4) setVal[setIndex].lSet += (long)count * 10000L;
    if (clampDigit == 3) setVal[setIndex].lSet += (long)count * 1000L;
    if (clampDigit == 2) setVal[setIndex].lSet += (long)count * 100L;
    if (clampDigit == 1) setVal[setIndex].lSet += (long)count * 10L;
    if (clampDigit == 0) setVal[setIndex].lSet += (long)count * 1L;
    nDispClamps();
  }
  if (nPage == Main){
    setIndex = DFORCE;
    if (mainDigit == 5) setVal[setIndex].lSet += (long)count * 100000L;
    if (mainDigit == 4) setVal[setIndex].lSet += (long)count * 10000L;
    if (mainDigit == 3) setVal[setIndex].lSet += (long)count * 1000L;
    if (mainDigit == 2) setVal[setIndex].lSet += (long)count * 100L;
    if (mainDigit == 1) setVal[setIndex].lSet += (long)count * 10L;
    if (mainDigit == 0) setVal[setIndex].lSet += (long)count * 2L;
  }
}


void digitLeft(void){
  if (nPage == Main){  // Big display on Page 0
    mainDigit = mainDigit + 1;
    if  (mainDigit > DIGIT_MAX) mainDigit = DIGIT_MAX;
    //Serial.print("l ");
    //Serial.println(mainDigit);
    nCursor(); 
  }
  if (nPage == Clamp){  // 4 clamp displays on Page 1
    clampDigit = clampDigit + 1;
    if  (clampDigit > DIGIT_MAX) {
      clampDigit = DIGIT_MIN;           // Go to next display up
      clampSel = clampSel - 1;
      if (clampSel < 0) clampSel = 3;
    }
    //Serial.print("l ");
    //Serial.println(clampDigit);
    nCursor(); 
  }
}

void digitRight(void){
  if (nPage == Main){  // Big display on Page 0
    mainDigit = mainDigit - 1;
    if  (mainDigit < DIGIT_MIN) mainDigit = DIGIT_MIN;
    //Serial.print("r ");
    //Serial.println(mainDigit);
    nCursor(); 
  }
  if (nPage == Clamp){  // 4 clamp displays on Page 1
    clampDigit = clampDigit - 1;
    if  (clampDigit < DIGIT_MIN){     // Go to next display down
      clampDigit = DIGIT_MAX;
      clampSel = clampSel + 1;
      if (clampSel > 3) clampSel = 0;
    }
    //Serial.print("r ");
    //Serial.println(clampDigit);
    nCursor(); 
  }
}

void getPage(void){
  if (myNex.currentPageId != myNex.lastCurrentPageId){    // Change of page
    nPage = myNex.currentPageId;                                // Store the currentPageId
    //Serial.print("Page = ");
    //Serial.println(nPage);  
    nCursor();                                       // Update cursor on page change
    myNex.lastCurrentPageId = nPage;
  }
}
