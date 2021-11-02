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

// I2C Addresses
#define IR_Temp_Sensor_Addr     0x5A
#define LCD_Addr                0x27

// Other defines
#define DHTTYPE DHT11

#define errorMargin             2
#define controlMargin           1

// Create objects
LiquidCrystal_I2C lcd(LCD_Addr, 16, 2);       // Create the LCD object. LCD size is 16x2
Adafruit_MLX90614 IRTemp = Adafruit_MLX90614();
DHT dht(DHTpin, DHTTYPE);

// Project variables
float skinTemp;
float incubatorTemp;
float incubatorHumid;

// Unimplemented sensor values
float targetTemp      = 34.0;
float ambientTemp     = 30.0;
float ambientHumid    = 70.0;

void setup() {
  // put your setup code here, to run once:

  // Set the pin modes
  pinMode (redLED, OUTPUT);
  pinMode (yellowLED, OUTPUT);
  pinMode (greenLED, OUTPUT);
  pinMode (buzzerPin, OUTPUT);
  pinMode (heaterRelay, OUTPUT);
  pinMode (fanRelay, OUTPUT);
  pinMode(reedSwitch, INPUT);

  // Set the initial states  digitalWrite (redLED, HIGH);
  digitalWrite (redLED, HIGH);
  digitalWrite (yellowLED, LOW);
  digitalWrite (greenLED, LOW);
  digitalWrite (buzzerPin, LOW);
  digitalWrite (heaterRelay, LOW);
  digitalWrite (fanRelay, LOW);

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

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Target Temp: ");
  lcd.setCursor(0,1); lcd.print(targetTemp);  lcd.print(" *C");
  delay(2000);
  
  
}

void loop() {
  // put your main code here, to run repeatedly:

  // Read the values
  skinTemp        = IRTemp.readObjectTempC();
  incubatorTemp   = dht.readTemperature();
  incubatorHumid  = dht.readHumidity();

  // Display infant skin temperature
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Skin Temp: ");
  lcd.setCursor(0,1); lcd.print(skinTemp,1);  lcd.print(" *C");
  delay(2000);

  // Display ambient temperature
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Amb.Temp: ");
  lcd.print(round(ambientTemp));  lcd.print("*C");

  // Display ambient humidity
  lcd.setCursor(0,1); lcd.print("Amb.Hum: ");
  lcd.print(round(ambientHumid));  lcd.print("%");
  delay(2000);

 // Display incubator temperature
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Inc.Temp: ");
  lcd.print(round(incubatorTemp));  lcd.print("*C");

  // Display incubator humidity
  lcd.setCursor(0,1); lcd.print("Inc.Hum: ");
  lcd.print(round(incubatorHumid));  lcd.print("%");
  delay(2000);

  // If the incubator temperature is out of expected range
  if ( (incubatorTemp  > targetTemp + errorMargin) || (incubatorTemp  < targetTemp - errorMargin) ) {
    // Sound alarm
    for (int i=0; i<5; i++) {
      digitalWrite (buzzerPin, HIGH);
      delay(300);
      digitalWrite (buzzerPin, LOW);
      delay(200);
    }

    // If the incubator temperature was higher
    if (incubatorTemp  > targetTemp + errorMargin) {
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Incubator Temp");
      lcd.setCursor(0,1); lcd.print("too high");
      delay(2000);
    }
    else {
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Incubator Temp");
      lcd.setCursor(0,1); lcd.print("too low");
      delay(2000);
    }
  }

  // If the incubator temperature must be controlled
  if ( (incubatorTemp  > targetTemp + controlMargin) || (incubatorTemp  < targetTemp - controlMargin) ) {

    // If the incubator temperature was high
    // We turn on the fan and turn off the heater
    if (incubatorTemp  > targetTemp + controlMargin) {
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Incubator Temp");
      lcd.setCursor(0,1); lcd.print("high");
      
      digitalWrite (heaterRelay, LOW);        // Turn off heater
      digitalWrite (fanRelay, HIGH);          // Turn on the fan
      delay(2000);
    }
    else {    // ie. if the temperature was cooler
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Incubator Temp");
      lcd.setCursor(0,1); lcd.print("low");
      
      digitalWrite (heaterRelay, HIGH);       // Turn on heater
      digitalWrite (fanRelay, LOW);           // Turn off the fan
      delay(2000);
    }
  }

  else { // no need to modify the temperatures
    digitalWrite (heaterRelay, LOW);        // Turn off heater
    digitalWrite (fanRelay, LOW);           // Turn off the fan
  }
}
