/*  Contains VRekrer SCPI functions
 *   Callbacks for DIY-SMU
*/

#include "Vrekrer_scpi_parser.h"

void initScpi(void)
{
  // Set up SCPI
  my_instrument.RegisterCommand(F("*IDN?"), &Identify);           // F() specifies Flash strings
  my_instrument.RegisterCommand(F("*RST"), &SetReset);
  my_instrument.SetCommandTreeBase(F("MEASure"));
    my_instrument.RegisterCommand(F(":CURRent?"), &GetCurr);
    my_instrument.RegisterCommand(F(":VOLTage?"), &GetVolt);
  my_instrument.SetCommandTreeBase(F("SOURce"));
    my_instrument.RegisterCommand(F(":VOLTage"), &SetVolt);
    my_instrument.RegisterCommand(F(":VOLTage?"), &GetSourceVolt);  //Get Voltage Setting
    my_instrument.RegisterCommand(F(":VOLTage:RANGe"), &SetVoltRange);    
    my_instrument.RegisterCommand(F(":CURRent"), &SetCurr);
    my_instrument.RegisterCommand(F(":CURRent:RANGe"), &SetCurrRange);
  my_instrument.SetCommandTreeBase(F("OUTput"));
    my_instrument.RegisterCommand(F(":STATe"), &SetState);  // ON/OFF
    my_instrument.RegisterCommand(F(":RSENse"), &SetRemoteSense);  // INT/EXT
  my_instrument.SetCommandTreeBase(F("FUNCtion"));
    my_instrument.RegisterCommand(F(":VOLTage"), &SetFuncVolt);  // Force Voltage Mode
    my_instrument.RegisterCommand(F(":CURRent"), &SetFuncCurr);  // Force Current Mode  
  my_instrument.SetCommandTreeBase(F("CALibrate"));
    my_instrument.RegisterCommand(F(":VOLTage"), &CalVolt);     // Cal Voltage 
    my_instrument.RegisterCommand(F(":CURRent"), &CalVolt);     // Cal Current
    my_instrument.RegisterCommand(F(":STATe"), &SetCalState);   // Cal factors ON/OFF
}

// SCPI Commands
/*
 * Respond to *IDN?
 */
void Identify(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println(F("Erickson Engineering,DIY-SMU1,#01,v1.0"));
}

void SetReset(SCPI_C commands, SCPI_P parameters, Stream& interface) {
//  interface.println(F("Doing a reset"));
  reset();
}


/**
 * Read the DAC Set voltage: Iset
 */
void GetSourceVolt(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.print((float)setVal[DFORCE].lSet, 6);
  //Serial.println(" V");
}

/* These 2 functions need to be properly synchronized or trigger the ADC. 
 *  Currently the SCPI MEAS: command requires one to just wait one full loop 
 *  (2 ADC conversions + loop time) or more  
 */
void GetCurr(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  //getIMeasure();    // Can't do this here and in the main loop. 
  interface.println(String(instr.mi, 12));
}

void GetVolt(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  //getVMeasure();    // Can't do this here and in the main loop. 
  interface.println(String(instr.mv, 9));
}

void SetVolt(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  float voltage;
  // No out of range or bad parameter check is done. 
  if (parameters.Size() > 0) {
    voltage = String(parameters[0]).toFloat();
    voltage = constrain(voltage, -150.0, 150.0);
    setVal[DFORCE].lSet = voltage * Range[instr.vRange].F_ItoF_Scale;
    setForce();         // This clips to the range maximumse
  }
}

void SetCurr(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  float curr = 0.0;
  // For simplicity no bad parameter check is done.
  if (parameters.Size() > 0) {
    curr = String(parameters[0]).toFloat();
    //interface.println(curr, 6);
    setVal[DFORCE].lSet = curr * Range[instr.iRange].F_ItoF_Scale;
  }
}

void SetFuncVolt(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  setMode(FV);

}

void SetFuncCurr(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  setMode(FI);

}

void SetVoltRange(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  float vrange = 0.0;
  // For simplicity no bad parameter check is done.
  if (parameters.Size() > 0) {
    vrange = String(parameters[0]).toFloat();
    //interface.println(vrange,6);
    if (vrange <= 1.6 ) {
      setVRange(VRange_1_5);
      //interface.println("1.5V Range");
    }
    if (vrange > 1.6 && vrange <= 16.0) {
      setVRange(VRange_15);
      //interface.println("15V Range");
    }
    if (vrange > 16.0 && vrange <= 160.0) {
      setVRange(VRange_150);
      //interface.println("150V Range");
    }
    if (vrange >= 160.0) interface.println("Vrange out of range");
    
    dispRange();
  }
}

void SetCurrRange(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  float irange = 0.0;
  // For simplicity no bad parameter check is done.
  if (parameters.Size() > 0) {
    irange = String(parameters[0]).toFloat();
    //interface.println(irange, 6);
    if (irange <= 1.06e-6) {
      setIRange(IRange_1uA);
      //interface.println("1uA Range");
    }
    if (irange > 1.06e-6 && irange <= 1.06e-5) {
      setIRange(IRange_10uA);
      //interface.println("10uA Range");
    }
    if (irange > 1.06e-5 && irange <= 1.06e-4) {
      setIRange(IRange_100uA);
      //interface.println("100uA Range");
    }
    if (irange > 1.06e-4 && irange <= 1.06e-3) {
      setIRange(IRange_1mA);
      //interface.println("1mA Range");
    }
    if (irange > 1.06e-3 && irange <= 1.06e-2) {
      setIRange(IRange_10mA);
      //interface.println("10mA Range");
    }
    if (irange > 1.06e-2 && irange <= 1.06e-1) {
      setIRange(IRange_100mA);
      //interface.println("100mA Range");
    }
    if (irange > 1.06e-1) interface.println("Irange out of range");
    dispRange();
  } 
}

void SetState(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // For simplicity no bad parameter check is done.
  if (parameters.Size() > 0) {
    String parameter0 = String(parameters[0]);
    parameter0.toUpperCase();
    if ((parameter0 == "ON") || (parameter0 == "1")) {
      setOutput(1);
//      interface.println("ON");
    } 
    else if ((parameter0 == "OFF") || (parameter0 == "0")) {
      setOutput(0);
//      interface.println("OFF");
    }
  }
  dispOnOff();
}

// Accepts one cal factor with 2 parameters: int index and float Value
// Stores in EEPROM
void CalVolt(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  int index;
  float value;

  if (parameters.Size() == 2) {
    index = String(parameters[0]).toInt();
    if (index >=0 && index < 36) {            // 12 for V, 24 for I
      value = String(parameters[1]).toFloat();
      Serial.print("Index: ");
      Serial.print(index);
      Serial.print(" Value: ");
      Serial.println(value);
    }
  }
}


void SetCalState(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // For simplicity no bad parameter check is done.
  if (parameters.Size() > 0) {
    String parameter0 = String(parameters[0]);
    parameter0.toUpperCase();
    if ((parameter0 == "ON") || (parameter0 == "1")) {
      setCal(1);
      //interface.println("CAL ON");
    } 
    else if ((parameter0 == "OFF") || (parameter0 == "0")) {
      setCal(0);
      //interface.println("CAL OFF");
    }
  }
  dispOnOff();
}

void SetRemoteSense(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // For simplicity no bad parameter check is done.
  if (parameters.Size() > 0) {
    String parameter0 = String(parameters[0]);
    parameter0.toUpperCase();
    if ((parameter0 == "ON") || (parameter0 == "1") || (parameter0 == "REM")) {
      interface.println("Remote");
    } 
    else if ((parameter0 == "OFF") || (parameter0 == "0") || (parameter0 == "LOC")) {
      interface.println("Local");
    }
  }
}
