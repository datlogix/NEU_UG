// Import libraries
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MLX90614.h>
#include <DHT.h>

// Pin Definitions
#define   redLED        2              
#define   yellowLED     4
#define   greenLED      3
#define   reedSwitch    11
#define   heaterRelay   37              
#define   fanRelay      35
#define   buzzerPin     5
#define   DHTpin        6
#define   btnLeft       53
#define   btnCentre     51
#define   btnRight      49

#define   testPot       A8

// I2C Addresses
#define IR_Temp_Sensor_Addr     0x5A
#define LCD_Addr                0x27

// Other defines
#define DHTTYPE DHT22

#define errorMargin             2
#define controlMargin           1
#define targetTempMax           38
#define targetTempMin           30

// Create the fan
// Enumeration for the various states of the cooling fan
enum class FanSpeed {off, half, full};

// Create objects
LiquidCrystal_I2C lcd(LCD_Addr, 20, 4);       // Create the LCD object. LCD size is 20x4
Adafruit_MLX90614 IRTemp = Adafruit_MLX90614();
DHT dht(DHTpin, DHTTYPE);

// Project variables
float skinTemp;
float incubatorTemp;
float incubatorHumid;

bool heaterIsOn       = true;
FanSpeed fanState     = FanSpeed::full;           // fanState variable based off the fanSpeed enum
bool btnCentre_state;                             // State of the centre push button 
bool btnRight_state;                              // State of the right push button
bool btnLeft_state;                               // State of the left push button



// Unimplemented sensor values
float targetTemp      = 34.0;

void setup() {
  // put your setup code here, to run once:

  // Set the pin modes
  pinMode (redLED, OUTPUT);
  pinMode (yellowLED, OUTPUT);
  pinMode (greenLED, OUTPUT);
  pinMode (buzzerPin, OUTPUT);
  pinMode (heaterRelay, OUTPUT);
  pinMode (fanRelay, OUTPUT);
  pinMode (reedSwitch, INPUT);
  pinMode (btnLeft, INPUT_PULLUP);
  pinMode (btnCentre, INPUT_PULLUP);
  pinMode (btnRight, INPUT_PULLUP);
  
  pinMode (testPot, INPUT);         // Potentiometer for temperature varying tests

  // Set the initial states  digitalWrite (redLED, HIGH);
  digitalWrite (redLED, HIGH);
  digitalWrite (yellowLED, LOW);
  digitalWrite (greenLED, LOW);
  digitalWrite (buzzerPin, LOW);
  switchHeater(false);              // Ensure that heater is off upon boot
  switchFan(false);                 // Ensure that fan is off upon boot

  // initialize the lcd object
  lcd.init();
  lcd.setBacklight(true);           // Turn on the backlight

  // initialize the MLX sensor
  if (!IRTemp.begin()) {
    lcd.setCursor(5,0); lcd.print("Error");
    while (1);
  };

   // initialize the DHT sensor
  dht.begin();

  // Display welcome message
  lcd.clear();
  lcd.setCursor(2,0);               // Go to first line
  lcd.print("Initializing");
  lcd.setCursor(5,1); lcd.print("System");
  delay(3000);

  lcd.clear();
  lcd.setCursor(1,0); lcd.print("Initialization");
  lcd.setCursor(6,1); lcd.print("Done");
  // Beep buzzer twice
  for (int i=0; i<2; i++) {
    digitalWrite (buzzerPin, HIGH);
    delay(300);
    digitalWrite (buzzerPin, LOW);
    delay(200);
  }
  delay (1000);
  
}

// ------------------------ Loop Start ------------------------------
void loop() {
  // Read the Centre Button State
  btnCentre_state = !digitalRead(btnCentre);      // HIGH means button pressed, LOW otherwise
  delay (50);                                     // Delay 50ms for debouncing

  // If OK button is pressed, we go to the temperature adjustment menu
  if (btnCentre_state == HIGH) {
    adjustTargetTemp();
  }

  // Read the values
  skinTemp        = IRTemp.readObjectTempC();
  incubatorTemp   = dht.readTemperature();
  int sensorValue = analogRead(testPot);                    // Using the test potentiometer to simulate incubator 
                                                              // temperature for testing
  sensorValue     = map (sensorValue, 0, 1023, 30, 38);
  incubatorTemp   = sensorValue;
  incubatorHumid  = dht.readHumidity();

  // Display infant skin temperature
  //lcd.clear();
  clearLCDLines(0,2);   //Clear top three lines
  lcd.setCursor(0,0); lcd.print("Skin Temp: "); 
  lcd.print(skinTemp,1);  lcd.print("*C");

 // Display incubator temperature
  lcd.setCursor(0,1); lcd.print("Inc.Temp : ");
  lcd.print(round(incubatorTemp));  
  lcd.print("/");
  lcd.print(round(targetTemp)); lcd.print("*C");

  // Display incubator humidity
  lcd.setCursor(0,2); lcd.print("Inc.Hum  : ");
  lcd.print(round(incubatorHumid));  lcd.print("%");
  delay(3000);                                        // Delay a little so we see the values on the LCD

  // If the incubator temperature is out of expected range
  if ( (incubatorTemp  > targetTemp + errorMargin) || (incubatorTemp  < targetTemp - errorMargin) ) {

    // If the incubator temperature was higher
    if (incubatorTemp  > targetTemp + errorMargin) {
      clearLCDLines(0,2);   //Clear top three lines
      lcd.setCursor(0,0); lcd.print("Incubator Temp");
      lcd.setCursor(0,1); lcd.print("too high");
    }
    else {
      clearLCDLines(0,2);   //Clear top three lines
      lcd.setCursor(0,0); lcd.print("Incubator Temp");
      lcd.setCursor(0,1); lcd.print("too low");
    }
    
    // Sound alarm
    for (int i=0; i<5; i++) {
      digitalWrite (buzzerPin, HIGH);
      delay(300);
      digitalWrite (buzzerPin, LOW);
      delay(200);
    }
    delay(1000);

    
  }

  // If the incubator temperature must be controlled
  if ( (incubatorTemp  > targetTemp + controlMargin) || (incubatorTemp  < targetTemp - controlMargin) ) {
    digitalWrite (yellowLED, HIGH);               // Turn on yellow LED
    digitalWrite (greenLED, LOW);                 // Turn off green LED 

    // If the incubator temperature was high
    // We turn on the fan and turn off the heater
    if (incubatorTemp  > targetTemp + controlMargin) {
      if (heaterIsOn) {                         // If heaing system is on, turn it off
        switchHeater(false);                    // Turn off heater
      }
      else {                                    // We need the forced cooling to overheating if fan was off
        switchFan (true);                       // Turn on the fan
      }
    }
    else {    // ie. if the temperature was cooler
      if (fanState != FanSpeed::off) {        // If cooling system is on, turn it off
        switchFan(false);                     // Turn off the fan
      }
      else {                                  // We need the heating to counter cooling if fan was off
        switchHeater(true);                   // Turn on heater
      }
    }
    
  }

  else { // no need to modify the temperatures
    switchHeater(false);                    // Turn off heater
    switchFan(false);                       // Turn off the fan

    clearLCDLines(3,3);                     // Clear the last line (status line)
    digitalWrite (greenLED, HIGH);          // Turn on Green LED
    digitalWrite (yellowLED, LOW);         // Turn off yellow LED
  }

  // delay (120000);                        // Delay 2mins before updating changing the heating/cooling conditions
                                            // This delay must be implemented with Asynchronous Delay to prevnt blocking
                                            // the other parts of the code for a long time
}
// ------------------------ Loop End ------------------------------


// Function to manage the turning on and off of the heating system
void switchHeater(bool turnOn) {
  if (turnOn) {         // Request to turn on heater
    if (!heaterIsOn) {
      digitalWrite (heaterRelay, HIGH);       // Turn on heater
      heaterIsOn = true;                      // Set flag to indicate that heater is on

      // Give feedback to user that heating is in progress
      lcd.setCursor(0,3); lcd.print("*Heating Activated*");
      delay(2000);
    }
  }
  else {                // Request to turn off heater
    if (heaterIsOn) {
      digitalWrite (heaterRelay, LOW);        // Turn on heater
      heaterIsOn = false;                     // Set flag to indicate that heater is off

      // Give feedback to user that heating is stopped
      lcd.setCursor(0,3); lcd.print("Heating Deactivated");
      delay(2000);
      // Clear de-activation message
      clearLCDLines(3,3);                   // Clear the last line (status line)
    }
  }
}

// Function to manage the turning on and off of the cooling system
void switchFan(bool turnOn) {
  if (turnOn) {         // Request to turn on fan

    // If fan was off, we turn it on at half speed
    if (fanState == FanSpeed::off) {        
      digitalWrite (fanRelay, HIGH);        // Turn on fan
      fanState = FanSpeed::half;            // Update the state variable that fan is on at half speed

      // Give feedback to user that cooling is in progress
      lcd.setCursor(3,3); lcd.print("*Slow Cooling*");
      delay(2000);
    }
    // If fan was at half speed, we turn it on to maximum speed
    else if (fanState == FanSpeed::half) {
      digitalWrite (fanRelay, HIGH);        // Turn on fan
      fanState = FanSpeed::full;            // Update the state variable that fan is on at max speed

      // Give feedback to user that cooling is in progress
      lcd.setCursor(3,3); lcd.print("*Fast Cooling*");
      delay(2000);
    }
  }
  else {                // Request to turn off fan
    if (fanState == FanSpeed::half || fanState == FanSpeed::full) {
      digitalWrite (fanRelay, LOW);         // Turn on fan
      fanState = FanSpeed::off;            // Update the state variable that fan has been turned off

      // Give feedback to user that cooling is stopped
      lcd.setCursor(0,3); lcd.print("Cooling Deactivated");
      delay(2000);
      // Clear de-activation message
      clearLCDLines(3,3);                   // Clear the last line (status line)
    }
  }
}

// Function to deal with clearing specific lines of the LCD screen
void clearLCDLines(int firstLine, int lastLine) {    // zero-based ie. 1 => line 2
  for (int i=firstLine; i<=lastLine; i++) {
    lcd.setCursor(0,i);                   // Go to the ith line
    lcd.print("                    ");    // Clear with 20 spaces ;-)
  }
}

// Menu interface for adjusting the target temperature
void adjustTargetTemp() {
  clearLCDLines(0,2);   //Clear top three lines
  lcd.setCursor(0,0); lcd.print("Target Temp: ");
  lcd.setCursor(0,1); lcd.print(round(targetTemp));  lcd.print(" *C");
  delay(1000);
  do {
    btnCentre_state = !digitalRead(btnCentre);
    delay(50);

    btnRight_state  = !digitalRead(btnRight);
    btnLeft_state   = !digitalRead(btnLeft);


    // If the increase push button is pressed
    if(btnRight_state == HIGH && targetTemp < targetTempMax) 
    {
      targetTemp += 1;    // Increase the current target temperature by 1
      lcd.setCursor(0,1); lcd.print(round(targetTemp));  lcd.print(" *C");         // Update value on LCD
      delay(250);
    }

    // If the decrease push button is pressed
    if(btnLeft_state == HIGH && targetTemp > targetTempMin) 
    {
      targetTemp -= 1;    // Decrease the current target temperature by 1
      lcd.setCursor(0,1); lcd.print(round(targetTemp));  lcd.print(" *C");         // Update value on LCD
      delay(250);
    }

  } while (btnCentre_state != HIGH);
}
