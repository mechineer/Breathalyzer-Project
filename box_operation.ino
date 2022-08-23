#include <Servo.h>

#include "DHT.h"

#define DHTPIN 6 // Digital pin connectddd to the DHT sensor

#define DHTTYPE DHT11 //DHT11
// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
//declaring data printing function
void printValues(bool p, bool a, float temp, float humid, float alcohol, unsigned long Ttaken);



///servo inits
Servo serv0; //creates the servo object

int pos = 0;
////////

///timer inits
unsigned long Stime = 0.0, Etime = 60000, Ctime = 0.0;
////////

///mq-3 calibration code
// Set Vo to 5 Volts
float Vo = 5.0;
// Declare all variables
float Vd,  Vfixed,  Ro, a_percent;
//coefficients from calibration
float a0 = 1.1999;
//these values are for choosing the range of LEDS displayed for the alcohol level detected

float low = 0.95, mid = 3.1935, high = 3.4;//4.1875;//most likely will change them, here for placeholder values
///////////////////////////////////

///defining the variables used by DHT11 in global scope so I can access them for data retrieval
float f = 0.0, h = 0.0, t = 0.0;
//////

///creating the array for the LED pins
int led[] = {41,40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 43, 42};


////
//these boolean variables will be used to note whether a person was detected, and whether the box was opened
//true means yes, false means no
bool person;
bool access;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);                           // for everything
  serv0.attach(9);                              //attaches the servo to pin 9
  serv0.write(48);                              //sets the servo to the locked position
  dht.begin();
  lcd.begin(18, 2);//from 16
  // Print a message to the LCD.
  lcd.print("system init");
  delay(1500);
  lcd.begin(18, 2);
  lcd.print("press top button");
  lcd.setCursor(0, 1);
  lcd.print("when ready");

                                                // sets the pinmode for all the led pins
  for(int i = 0; i < 20; i ++){
    pinMode(led[i], OUTPUT);
  }  



  
}

void loop() {
  // put your main code here, to run repeatedly:
  person = false;
  access = false;
  
  if(digitalRead(44) == HIGH){                   //reads input from button 1
    
    if(serv0.read() != 48){                       //checks the position of the servo to see if the box is locked
      
        for (pos = serv0.read(); pos >= 48; pos -= 1) { // moves the servo from current position to locked at 48 degrees
                                                        // in steps of 1 degree
                                                        //Serial.println(myservo.read());
        serv0.write(pos);                       // tell servo to go to position in variable 'pos'
        delay(15);
      }
      
    }
    Ctime = 0.0;
    unsigned long timeLeft = Etime;
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("After the timer");
    lcd.setCursor(0,1);
    lcd.print("starts ");
    delay(1500);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Blow in the box");
    delay(1500);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Time left:");
    
    
    Stime = millis();
  
    while(Ctime < Etime){                       //loop runs for Etime before stopping and telling the user to try again.
      
      if(digitalRead(45) == HIGH){              //checks for the killswitch (button 2)
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Operation");
          lcd.setCursor(0,1);
          lcd.print("Terminated");
          //this for loop opened the box after pressing the killswitch
            delay(1000);                        //delays the loop so that maintenance mode isnt activated with every press
        break;                                  //if its pressed, the box unlocks, this is for worst case scenarios, 
                                                //debugging, as I would like to have a way to open it without breaking the box 
       }
      lcd.setCursor(10,0);
      lcd.print(float(((timeLeft-Ctime))/1000));//since the sensor requires a 2 second period to get temp and humidity
                                                // time will decrease by 2 seconds per iteration
      Ctime = millis() - Stime;                 // calculates the time spent running the sensors
      //this is where the sensor reading code shall go
      delay(2000);                               //i tried to use shorter delays, but the readings became unreliable

                                                // Reading temperature or humidity takes about 250 milliseconds!
                                                // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      f = dht.readTemperature(true);
    
      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t) || isnan(f)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
    
      // Compute heat index in Fahrenheit (the default)
      float hif = dht.computeHeatIndex(f, h);

      lcd.setCursor(0,1);
      lcd.print("H:");
      lcd.print(h);
      lcd.print("%");
      lcd.setCursor(8, 1);
      lcd.print("T:");
      lcd.print(f);
      lcd.print("F");

                                                //this is where the logic for the dht11 sensor will go, here is where the presence of a person will be detected
                                                // using a value of 90% humidity for, and I will be using the heat index, hif at 85 F

      if((h >= 90.00) && (hif >= 85.00))
      {
        person = true;

        //now for the mq3, and the LED code
        // Read the input on analog pin 0
        Vd = analogRead(0);

         // Change the digital input 0-1023 into a voltage between 0V-5V
        Vfixed = Vd*(Vo/1023);

        a_percent = a0*Vfixed - 0.1;//calculates the percent of alcohol in the air from the excel data
        //Serial.print(a_percent);
 //logic for displaying the LEDs according to detected alcohol levels
        if (a_percent <= low){//here it will just be the green leds'
          access = true;
          for (int i = 0; i < 8; i ++){ 
            digitalWrite(led[i], HIGH);
          }
          delay(1000);
          }
          else if((low < a_percent) && (a_percent <= mid)){         //all green and half of the yellow LED's are lit
          for (int i = 0; i < 12; i ++){ 
            access = true;
            digitalWrite(led[i], HIGH);
          }
          delay(1000);
          }
           else if((mid < a_percent)&&(a_percent <= high)){         //all green and yellow LEDs are lit
            access = true;
          for (int i = 0; i < 16; i ++){ 
            digitalWrite(led[i], HIGH);
          }
          delay(1000);
          }
           else if(high < a_percent){                               // all green, yellow, and red LED's will be lit
            access = false;
          for (int i = 0; i < 20; i ++){ 
            digitalWrite(led[i], HIGH);
          }
          delay(1000);
          }
        break; //exit the while loop
      }

}
    if (Ctime >= Etime){//here the program determines if the time for testing has run out
      printValues(person, access, f, h, a_percent, Ctime);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Timer has ended!");
      lcd.setCursor(0,1);
      lcd.print("Try again");
    }

    
  }
  else if(digitalRead(45) == HIGH){             //this is for opening the box before starting the timers, for debugging and internal checks

    //delay(1500);
        for (pos = serv0.read(); pos <= 168; pos += 1) { // goes from current position to unlocked position
          serv0.write(pos);              // tell servo to go to position in variable 'pos'
          delay(15);                       // waits 15ms for the servo to reach the position
  }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Maintenance Mode");
}



   if(person && access){//checks whether a person is present, and they have been given access, depending on the alcohol detected
    lcd.clear();        //
                        // a person has been detected.
    lcd.setCursor(0,0);
    lcd.print("Entry Granted");
    printValues(person, access, f, h, a_percent, Ctime);
      int wait = 30;
      for (pos = serv0.read(); pos <= 168; pos += 1) { // goes from current position to unlocked position
          serv0.write(pos);              // tell servo to go to position in variable 'pos'
          delay(15);                       // waits 15ms for the servo to reach the position
      }
      
  for(int k = 20; k > 0; k -= 2){
      wait -= 1;
    for(int i = 0; i < k; i++){
      digitalWrite(led[i], HIGH);
      delay(wait);
      //digitalWrite(led[i], LOW);
    }
    //delay(wait);
      for(int j = k; j > 0; j--){
      //digitalWrite(led[j], HIGH);
      delay(wait);
      digitalWrite(led[j], LOW);
    }
    
}
digitalWrite(led[0], LOW);
delay(1000);

lcd.clear();  

                                         
                            ///Person - available- bool person
                            //temp - available- float f
                            //humidity - available float h
                            //alcohol reading - available float a_percent
                            ///time ran, time for reading, etc.
                            ///(allowing key access or not)

                            
  }                    
  else if(person && !access){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Access Denied");
    lcd.setCursor(0,1);
    lcd.print("Get an Uber");
    delay(1000);
    for (int i = 20; i > 0; i--){
      digitalWrite(led[i], LOW);
    }
    digitalWrite(led[0], LOW);
    printValues(person, access, f, h, a_percent, Ctime);
  }

//now for the data saving data printing

}


//printValues(person, access, hif, h, a_percent, Ctime);
void printValues(bool p, bool a, float temp, float humid, float alcohol, unsigned long Ttaken){
//p is for person 
//a is for access 
//temp is for temp read
//humid is for humidity read
//Ttaken is for time from button press to access granted/ denied
Serial.print("Time taken for readings:");
if(Ttaken < Etime){
  Serial.println(float(Ttaken)/1000);//converts time to seconds
}
else{
  Serial.println("Time ran out");
}
Serial.print("Person Detected: ");
if(p){
  Serial.println("true");
}
else if(p == false){
  Serial.println("false");
}
Serial.print("Temperature: ");
if(temp == 0.0){
  Serial.println("No reading");
}
else if(temp != 0.0){
  Serial.println(temp);
}
Serial.print("Humidity: ");
if(humid == 0.0){
  Serial.println("No reading");
}
else if(humid != 0.0){
  Serial.println(humid);
}
Serial.print("Alchol Percent: ");
if(p){
  Serial.println(alcohol);
}
else if(p == false){
  Serial.println("No reading");
}
Serial.print("Access: ");
if(a){
  Serial.println("true");
}
else if (a == false) {
  Serial.println("false");
}
}
