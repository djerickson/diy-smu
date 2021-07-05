/*
  DIY-SMU
  D. Erickson, Erickson Engineering
  See www.djerickson.com/diy_smu
  
  9/6/20    Started from 18b DAC project, LeoLed processor and OLED display
  9/7/20    16b DACs working
  9/11/20   ADC working
  9/13/20   Added I Measure
  9/29/20   Started Nextion display
  10/5/20   Nextion Working
  10/10/20  Optimized ADC speed and removed unneded delay()'s to make loop() faster 
            Should make ADC read non-blocking to speed loop further
            Should also make serial read, read all data. See serial read stuff.   
  10/19/20  Added FIMV. Lots of GUI hacking to change modes
  10/21/20  Added Clamp function. 
  11/21/20  Fixed Nextion missing codes: Errors caused by sending object to inactive pages
            Fixed scaling, clamps. Need to fix clamp formatting 
  01/17/21  Started cleanup 
  02/07/21  Teensy added, changed timer, pinouts. Compiles OK. Runs on bare board, sends serial, Times out OK
  02/14/21  Running 100% on new CPU board with Teensy 3.2 CPU
  04/11/21  Refactor instrument control code. Add instr.ino. Remove setCtrlReg() from loop()
            Fixed Current Range button overlap bug. It's caused by the order of .1d's in Nextion. Higher numbers install later, get priority. 
            Use the Up and Down arrows in Nextion editor to move priorities. Crude but it works. 
  04/12/21  Functions for renges. Moved calibration factors to cal.ino. 
  04/25/21  SCPI Working!! Source, measure
  04/26/21  Refactored ON/OFF. Now working on SCPI
  
  
*/

#include <Wire.h>
#include <stdio.h>
#include <EEPROM.h>
#include "EasyNextionLibrary.h"  // Include EasyNextionLibrary
#include "Vrekrer_scpi_parser.h"

EasyNex myNex(Serial1); // Create object of EasyNex class with name myNex. Set Serial port.

//Defines for processor-specific code: Timer setup and timed interrupt handler. Steal a Teensy timed interrupt from the Polysynth Project. 
#define TEENSY
//#define ATMEGA

#ifdef TEENSY
#include <TimerOne.h>
#endif // TEENSY

#ifdef ATMEGA
// Pin Definitions
#define TESTPIN 0       // Just for test point
// Encoder
#define ENCA 4
#define ENCB 7
//#define S1 A4   // Reading S1 doesn't work with D17, uses nonstandard pin
// Switches
//#define S1 17
#define S2 11     // RIGHT
#define S3 12     // LEFT
#define S4 5
#define S5 13
// Pins for SPI to Main Board, Arduino ATMEGA
#define SPI_CK    A5
#define SPI_MOSI  A4
#define SPI_MISO  A3
#define SPI_SS0   A2    // DAC SS/
#define SPI_SS1   A1    // ADC SS/
#define SPI_SS2   A0    // REG SS/
#endif

#ifdef TEENSY
#define TESTPIN 2       // Just for test point
// Encoder
#define ENCA 16
#define ENCB 17
// Switches
#define S1 18
#define S2 20     // RIGHT
#define S3 21     // LEFT
#define S4 23
#define S5 22
#define S6 7
#define S7 6
#define S8 5
// Pins for SPI to Main Board, Teensy 3.2
#define SPI_CK    13
#define SPI_MOSI  11
#define SPI_MISO  12
#define SPI_SS0   10    // DAC SS/
#define SPI_SS1   9    // ADC SS/
#define SPI_SS2   8    // REG SS/
#endif

// 16b DAC scaling: Needs to be signed
#define DAC_REF 5.0
#define DAC_MAX 65535L

// AD5686 DAC settings
#define SETREG_CMD 0x01L
#define SETDAC_CMD 0x03L
#define SETPWR_CMD 0x04L
#define SETREF_CMD 0x07L

// DAC Channels
#define DAC_FORCE 0x01L
#define DAC_CLN   0x02L
#define DAC_CLP   0x04L
#define DAC_SPARE 0x08L


// Display Force and Clamp limit and init set values 
#define FDISPVMAX  150000L
#define FDISPVMIN -150000L
#define FDISPVINIT      0L 

#define FDISPIMAX  105000L
#define FDISPIMIN -105000L
#define FDISPIINIT      0L 

#define CDISPVMAX    15000L
#define CDISPVMIN   -15000L
#define CDISPVINITP   5000L
#define CDISPVINITN - 5000L

#define CDISPIMAX    10000L
#define CDISPIMIN   -10000L
#define CDISPIINITP  10000L
#define CDISPIINITN -10000L

/* Encoder action codes */
#define NADA  0
#define INCR  1
#define DECR  2
#define ILL   3

/* Set to the appropriate encoder used */
#define ENC4PPCLICK
//#define ENC1PPCLICK

// Key state, to debounce, etc. one for each key. Also KeyFlag, set by interrupts
#define NUMKEYS   4
#define KEYIDLE   0
#define KEYVALID  20
#define KEYWAIT   KEYVALID + 1

#define NUM_DISP 4
#define MAX_DISP NUM_DISP -1

#define DIGIT_MAX 5
#define DIGIT_MIN 0

// cirBuf Struct
//typedef struct {
//  char  *buf;           // Buffer[]  
//  int bufSiz;
//  int   wp;             // Write pointer
//  int   rp;             // Read pointer
//  } cirBufTypeDef;
//cirBufTypeDef fpBuf; 


// Range Struct        
typedef struct {
  char *RangeUnit;     // Range units string
  char *RangeLabel;    // Range label string  
  float FOffsCal;       // Force Cal
  float FGainCal;
  float MOffsCal;       // Measure Cal
  float MGainCal;
  float COffsPCal;      // Clamp + Cal
  float CGainPCal;
  float COffsNCal;      // Clamp - Cal
  float CGainNCal;
  float Mfull;          // Measure Full scale range
//  long  FDispMax;       // Max force and clamp display in integers
//  long  FDispMin;       // Min display in integers
//  long  FDispInit;      // Initial force display       
  long long  F_ItoF_Scale;   // Scale to convert Iset to float for force
  long  M_ItoF_Scale;   // Scale to convert Iset to float for force
  byte  ForceDP;        // Decimal point 0-5
  byte  MeasDP;         // Decimal point 0-5
  byte  ClampDP;        // Decimal point 0-5
  } rangeTypeDef;

#define NUM_V_RANGES 3
#define NUM_I_RANGES 6
#define NUM_RANGES NUM_V_RANGES+NUM_I_RANGES
#define VRange_1_5 0
#define VRange_15  1
#define VRange_150 2
#define IRange_1uA 3
#define IRange_10uA 4
#define IRange_100uA 5
#define IRange_1mA 6
#define IRange_10mA 7
#define IRange_100mA 8

// Control bit definitions
#define CTL_VMODE  0x0001 
#define CTL_IMODE  0x0002
#define CTL_REMOT  0x0004 
#define CTL_ON     0x0008
#define CTL_1_5V   0x0040        // Low V 1.5V, off  Bit 6 HI for 
#define CTL_15V    0x0000        // Mid V 15V, Bits 6,7 LO
#define CTL_150V   0x0080        // High V 150V, Bit 7 HI
// Current ranges. All bits are active low
#define CTL_GE_10MA   0x4000     // Bit 6 high for >= 10mA ranges
#define CTL_I_1UA     0x3F00     // All HI except bit 6
#define CTL_I_10UA    0x3E00     // Bit 0, 6 LO
#define CTL_I_100UA   0x3D00     // Bit 1, 6 LO
#define CTL_I_1MA     0x3B00     // Bit 2, 6 LO
#define CTL_I_10MA    0x3700 | CTL_GE_10MA   // Bit 3 LO
#define CTL_I_100MA   0x2f00 | CTL_GE_10MA   // Bit 4 LO
#define CTL_I_1A      0x1F00 | CTL_GE_10MA   // Bit 5 LO

rangeTypeDef Range[NUM_RANGES];   // Create structs for each range. Should  make separate I and V ranges ?

#define FV_NOM 1.10  // Nominal force V DAC gain is +1.075: 21.5K / 20K, plus VM scaling
#define FI_NOM 1.075 // Nominal force I DAC gain is +1.075: 21.5K / 20K, Not sure why it is about 5% low

#define CL_NOM 1.10  // for 21.5K clamp DAC gain resistors
#define MV_NOM 1.125 // Nominal measure ADC range is +8%
#define MI_NOM 1.125 // Nominal measure ADC range is +8%


// System top level Struct
typedef struct {
  byte  mode;           // system mode: FVMI, FIMV, VM, IM, RM....
  byte  iRange;
  byte  vRange;
  float mi;             // I Measure
  float mv;             // V Measure  
  float fi;             // I Force
  float fv;             // V Force  
  float clp;            // Pos Clamp 
  float cln;            // Neg Clamp 
  char  onOff;          // unit on / off
  char  remSense;       // remote sense
  } topDef;

topDef instr;       // Create single top level struct

// Top level instrument state
#define FV 0
#define FI 1
#define MV 2
#define MI 3
#define MR 4

#define OFF 0     
#define ON 1

#define LOCAL 0 
#define REMOTE 1

// Display Struct These are the integer value settings for Force and Clamp values
//enum setVal {DFORCE=0, DCLIP=1, DCLIN=2, DCLVP=3, DCLVN=4} sDisp=DFORCE ; // Set Displays
#define DFORCE 0
#define DCLIP 1
#define DCLIN 2
#define DCLVP 3
#define DCLVN 4

/* Struct contains the values to be set by user and displayed by the UI. 
 * These are set as integers, then converted to floats to be scaled and calibrated, 
 * then converted back to int's to be set to the DACs. 
 */
typedef struct {        // Scaling, DP, units are from range struct
  long  lSet;           // Set Value, long integer
  float fSet;           // Not currently used
    } setDispDef;

setDispDef setVal[5];  // Currently 5 values to set by the UI. 0: Force, 1: CLV+, 2: CLV-, 3: CLI+, 4: CLI- 

/***********  Global Variables ********************/
int setIndex;    // setVal[] Index, defines which value is being set
unsigned int count = 1;
char chan;        // ADC Channel
float fIset;      // Force setting, float 
long im, vm;
float adcV, adcI;
char noDisp2 = 0;

SCPI_Parser my_instrument;

/* Encoder and Keypad stuff. Messy, should be a struct */
static unsigned char keyState[NUMKEYS];
static unsigned char keyFlag[NUMKEYS];
char command;
int mainDigit, clampDigit, clampSel;    // Cursor variables. clampSel is which clamp being set, 0-3
int16_t encoder_count = 0;      /* Number of encoder counts since last read */
int16_t old_encoder = 0;        /* Previous value */
uint8_t encoder_state;
enum Page {Main=0, Clamp=1, Plot=2, Splash=3} nPage=Main ; // Nextion Pages


/****************************** setup() *******************************/
void setup() {
  int i;
  // Initialize serial communications
  Serial.begin(115200);
  encoder_state = 0X0F;   // Set all bits high, the idle state for encoder. 

  mainDigit = 4;     // Start with 10s digit 
  clampDigit = 4;     // Start with 10s digit 
  clampSel = 0;

  NexSetup();     // Nextion Setup

  //  myNex.writeNum("page", 0);
  NsetPage(Splash);     // Display Splash screen, then delay
  delay(1500);         
  NsetPage(Main);  
  nPage = Main;
//  delay(2000);        // For USB COM port to start
//while (!Serial) {}    // wait for USB serial port to connect. This causes HW reset to hang!

  //Serial.println("DIY-SMU By D. Erickson");

/* Setup for 1mS interrupt.
   If Arduino, user Timer0 1ms interrupt. Default time is 16MHz/256/64 = 976.6 Hz
   Timer0 is already used for millis() - we'll interrupt about the middle and call the
   "Compare A" function below. OCR0A is not used by Arduino timers*/
#ifdef ATMEGA
  OCR0A = 128;           // Set to 128 out of 255 or 50%
  TIMSK0 |= _BV(OCIE0A);  // Set interrupt enable
#endif
/* Setup timer 1 for Teensy */
#ifdef TEENSY
#define F_SAMPLE 1000
  Timer1.initialize(1000000 / F_SAMPLE); // in uS
  Timer1.attachInterrupt(timedIntRoutine); // Stuff to do on 1mS interrupt
#endif

  pinMode(TESTPIN, OUTPUT);
  pinMode(ENCA, INPUT_PULLUP);  // Two encoder pins
  pinMode(ENCB, INPUT_PULLUP);
  pinMode(S2, INPUT_PULLUP);    // Switches
  pinMode(S3, INPUT_PULLUP);
  pinMode(S4, INPUT_PULLUP);
  pinMode(SPI_CK, OUTPUT);      // Set up SPI pins
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO, INPUT_PULLUP);
  pinMode(SPI_SS0, OUTPUT);
  pinMode(SPI_SS1, OUTPUT);
  pinMode(SPI_SS2, OUTPUT);
  digitalWrite(SPI_CK, 1);      // Set SPI to idle state
  digitalWrite(SPI_MOSI, 0);
  digitalWrite(SPI_SS0, 1);
  digitalWrite(SPI_SS1, 1);
  digitalWrite(SPI_SS2, 1);
  
  initDac();
  initAdc(0);
  
  for (i = 0; i < NUMKEYS; i ++) keyFlag[i] = 0; // Set up keypad flags
  
  initInstr();
  setIndex = DFORCE;
// Init voltage display
  setVal[DFORCE].lSet = FDISPVINIT;
// Instrument initial settings
  instr.mode = FV;
  instr.iRange = IRange_1mA;
  instr.vRange = VRange_15;
  instr.onOff = OFF;            // unit off
  instr.remSense = LOCAL;       // local sense
  
  initScpi();
  
  setCtrlReg();

/* EEPROM Test: Write */
//  float f = 123.4567;
//  EEPROM.put(0, f);
//  f = 42.4242;
//  EEPROM.put(4, f );
  
}  /* End of setup() */


/************************ loop() runs forever ***********************/
void loop()
{
//  EEPROM basic test: Read
//  float f;
//  
//  EEPROM.get(0, f);
//  Serial.print("EEPROM Data: ");
//  Serial.print(f, 6);
//  EEPROM.get(4, f);
//  Serial.print(" and: ");
//  Serial.println(f, 6);
  
  char key;
//  char incomingByte;

  my_instrument.ProcessInput(Serial, "\n");

  myNex.NextionListen(); // This function must be called repeatedly to response touch events
                         // from Nextion touch panel. Place in loop()

command = 0;
  // Get Encoder changes.
  
  if (encoder_count != old_encoder)
  {
    if (encoder_count > old_encoder)
    {
      command = 'U';
      count = encoder_count - old_encoder;
      old_encoder = encoder_count;
      //Serial.println("Enc: U");
    }
    if (encoder_count < old_encoder)
    {
      command = 'D';
      count =  old_encoder - encoder_count ;
      old_encoder = encoder_count;
      //Serial.println("Enc: D");
    }
  }

  key = getKeypad();
  if (key)
  {
    command = key;
//    Serial.print("Key: ");
//    ln(command);
  }
  
  switch (command)
  {
    //     case 'a':      //a n  nnnnn Set DAC n in counts
    //       i = Serial.parseInt();    // Get DAC number
    //       dac[i] = Serial.parseInt();  //Careful, with ParseInt() if there is no
    //               //non-numeric character after int,  serial waits for a timeout (1s default)
    //       break;

    case 'h':        // Help
      //Serial.println("a c, nnnnn Set Dac c to value nnnn in mV:");
      break;
    
    case 'U':    //U and D increment and decrement the current digit 
      digitUp();
      //Serial.println("U");
      break;
    case 'D':
      digitDown();
      //Serial.println("D");
      break;   
    
    case 'u':      // Add a bunch
      setVal[setIndex].lSet += (long)count * 1000L;
      //Serial.println("u");
      break;
    case 'd':    // Subtract 
      setVal[setIndex].lSet -= (long)count * 1000L;
      //Serial.println("d");
      break;
    
    case 'l':    // move digit left
      digitLeft();
      break;
    case 'r':
      digitRight();
      break;
    
    default:
      break;
  }
  command = 0;

  setForce();

//  Serial.print("fIset = ");
//  Serial.println (fIset, 6);

  getVMeasure();  // Measure Voltage
  getIMeasure();  // Measure Current

  dispMeas();
  dispMeas2();

  getPage();


// Test code for I and V range control bit setting
//  spi16W(CTL_VMODE | CTL_ON | CTL_REMOT | CTL_I_10MA);
  setCtrlReg();   // Mode, ON, SEN, Vr, Ir


  
// delay(20);        // delay sets display and loop() update rate, No need since ADC is slow
}
/*************************** End of loop() ********************/

// Select the timed interrupt control if ATMEGA or TEENSY
#ifdef ATMEGA
// This uses the Arduino ATMEGA interrupt. Interrupt is called about once per millisecond. Uses Arduino timer 0 
SIGNAL(TIMER0_COMPA_vect)
{
  timedIntRoutine();
}
#endif

// Timed Interrupt routine, every 1mS, needs to be fairly fast 
void timedIntRoutine(){  
  // Test by pulsing TESTPIN High
  digitalWrite(TESTPIN, 1);
  getEncoder();
  getKeys();
  digitalWrite(TESTPIN, 0);
}  


/******************* Controls: Encoder, buttons... ******************/

/* Encoder */
/* For encoders that generate 4 transitions per click such as most mechanical encoders.
   Note that mechanical encoders need pull up resistors and should also have R-C filters
*/
const  uint8_t encoder_logic[] = {
  /*old new      action
    ---------------------- */
  /* 00 00 */    NADA,
  /* 00 01 */    NADA,
  /* 00 10 */    DECR,
  /* 00 11 */    NADA,
  /* 01 00 */    NADA,
  /* 01 01 */    NADA,
  /* 01 10 */    NADA,
  /* 01 11 */    NADA,

  /* 10 00 */    INCR,
  /* 10 01 */    NADA,
  /* 10 10 */    NADA,
  /* 10 11 */    NADA,
  /* 11 00 */    NADA,
  /* 11 01 */    NADA,
  /* 11 10 */    NADA,
  /* 11 11 */    NADA
};

/* getEncoder()
   This is the interrupt routine for the encoder.
   The encoder is an optical encoder knob that outputs A-B quadrature signals. The knob is
   rotated relatively slowly, so once every 1 mS the 2 bits are polled. These are compared
   to the previous readings and depending on the sequence, either the count value is
   incremented, decremented or not action (NADA).
   The logic is contained in encoder_logic[] a table.
   Here's an example:
    old new action
    32  10
   --------------------
    00  00 no action
    00  01 increment
    ...
   The encoder state is 4 bits. The 2 LSBs are the new bits. The MSBs [3:2] are the previous readings.
*/
void getEncoder(void)
{
  uint8_t action;
  
  /* read 2 LSBs, shift up the previous bits */
  encoder_state = (encoder_state << 2) | (digitalRead(ENCB) << 1) | digitalRead(ENCA);
  action = encoder_logic[encoder_state & 0x0F];   /* Look up action: 4 LSBs only */
  if (action == INCR)
    encoder_count++;
  if (action == DECR)
    encoder_count--;
}


/* Interrupt to debounce keys. If the key is down, increment keyState. If not, reset to KEYIDLE, 0
    This contains the mapping between pins and keyState[]
    Set flag if count resches KEYVALID, thn hold until key is released.
*/

void getKeys (void)
{
  int keyIndex;
  // Read all the keys. Increment or reset keyState[]
  
  //  if (digitalRead(S1) == 0) keyState[0]++;
  //  else keyState[0] = KEYIDLE;

  if (digitalRead(S2) == 0) keyState[1]++;
  else keyState[1] = KEYIDLE;

  if (digitalRead(S3) == 0) keyState[2]++;
  else keyState[2] = KEYIDLE;

//  if (digitalRead(S5) == 0) keyState[3]++;
//  else keyState[3] = KEYIDLE;
  
  for (keyIndex = 0; keyIndex < NUMKEYS; keyIndex++)
  {
    if (keyState[keyIndex] >= KEYWAIT)
    {
      keyState[keyIndex] = KEYWAIT;  // Hold in KEYWAIT
    }
    if (keyState[keyIndex] == KEYVALID)             // Generate a key flag.
    {
      keyFlag[keyIndex] = 1;
      keyState[keyIndex] = KEYWAIT;
    }
  }
}


/* Read the key flags set by interrupts
   Map to the key codes
   keyFlag[0] = 't'
   keyFlag[2]S3
   Put the key in inKey[] and set the flag */
char getKeypad(void)
{
  char key2 = 0;
  
  if (keyFlag[0])
  {
    key2 = 't';
    keyFlag[0] = 0;
    //Serial.println("S1 Pressed");
  }
  if (keyFlag[1])
  {
    key2 = 'r';
    keyFlag[1] = 0;
    //Serial.println("S2 Pressed");
  }
  if (keyFlag[2])
  {
    key2 = 'l';
    keyFlag[2] = 0;
    //Serial.println("S3 Pressed");
  }
  if (keyFlag[3])
  {
    key2 = 's';
    keyFlag[3] = 0;
    //Serial.println("S5 Pressed");
  }
  //  Serial.println(keyState[0], DEC);
  //  Serial.println(keyState[1], DEC);
  //  Serial.println(keyState[2], DEC);
  //  Serial.print("In getKeypad() key2 = ");
  //  Serial.println(key2, DEC);
  return key2;
}
