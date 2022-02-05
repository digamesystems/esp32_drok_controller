/*
 * DROK Temperature Controller Interface
 * A little program to control/log data from the DROK temperature
 * controller module using an ESP32. (see: http://www.Droking.com)
 * 
 * I got mine on Amazon: 
 * https://www.amazon.com/dp/B07GQPT9VG?psc=1&ref=ppx_yo2_dt_b_product_details
 * 
 * DROK Model: XY-T01
 * 
 * The program sets up two serial ports on an ESP32  - 
 * one to talk to the DROK, and one to report to the serial monitor in the Arduino IDE.
 * 
 * After initializing the ports, setup() sends a "start" message to the controller 
 * to begin logging temperature and relay state. 
 * 
 * You can use either the serial monitor or serial plotter to look at the data. 
 * 
 * Commands typed in to the serial monitor will be routed to the DROK, so you can 
 * use this app to setup the controller as you wish. 
 * 
 * In playing with it, I find the following commands seem to work 
 * (NB: commands are case-sensitive!):
 * 
 * 'start' Turns on reporting. Temperature and Relay State are reported once a second.
 * 'stop' Turns off reporting. 
 * 'read' Gives a summary of the current parameters. (See datasheet)
 * 'S:XX' Sets the setpoint in deg. C.  
 *        I haven't yet figured out how to set tenths of a degree...
 * 'ALA:XX.X' Sets alarm temperature. 
 * 
 * Other commands might work as well. Feel free to explore. 
 * 
 */

#define debugUART Serial
#define drokUART Serial2
 
String temperature;      // Temp in deg. C
String relay;            // State of the relay 
bool   logging = false;  // When logging, the Drok periodically reports T and Relay state. 


// Declares
String drokCommand(String command); // Send a command to the Drok and wait for a reply.
String getValue(String data, char separator, int index); // String tokenizer function.
void   updateTemperatureAndRelay(String drokMessage); // Message cleanup for data reported while logging.


/*******************************************************************/
void setup() 
{
  // Serial Monitor UART
  debugUART.begin(9600); 

  // Second Serial port to communicate w/the DROK
  // TX=Pin33, RX=Pin25 - Pick what works for your hardware
  drokUART.begin(9600, SERIAL_8N1, 33, 25);                                          
  
  delay(1000); // Wait a bit for the ports to initialize

  debugUART.println("---------------------------------------------");
  debugUART.println("Drok Digital Thermostat Controller");
  debugUART.println("Version 1.0");
  debugUART.println("Released to the public under the MIT License");
  debugUART.println("                           -- Digame Systems");
  debugUART.println("                       www.digamesystems.com");
  debugUART.println("---------------------------------------------");
  
  drokCommand("on");    // Enable relay control
  drokCommand("start"); // Launch the DROK
  
  logging = true;
}



/*******************************************************************/
void loop() 
{ 
  String s="";

  // Route messages on the debugUART to the DROK.
  if (debugUART.available()){ 
    s = debugUART.readStringUntil('\n');
    s.trim();
    if(s=="start") logging = true;
    if(s=="stop")  logging = false;
    debugUART.println(drokCommand(s));
  }

  // Read and report Async DROK responses.
  if (drokUART.available()){    
    s = drokUART.readStringUntil('\n');
    if (logging){
      updateTemperatureAndRelay(s);
      debugUART.println(temperature + "," + 10*(relay == "OP"));
    } else {
      debugUART.println(s);
    }
  }

}


/*******************************************************************
* A little string tokenizer function to pull values our of the 
* Drok's reports
 */
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
      if (data.charAt(i) == separator || i == maxIndex) {
          found++;
          strIndex[0] = strIndex[1] + 1;
          strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
  }

  String s;
  if (found>index) {
    s = data.substring(strIndex[0], strIndex[1]);
    s.trim();
  } else {
    s = "";
  } 
  
  return s;
}


/*******************************************************************
 *  When reporting the temperature and relay state, the Drok puts 
 *  in some garbage characters after the Temperature reading. 
 *  This takes care of that.
 */
void updateTemperatureAndRelay(String drokMessage)
{
    temperature = getValue(drokMessage,',',0);
    
    int len1 = temperature.length();
    if (len1>=2){ // Remove garbage characters from controller message. 
      temperature = temperature.substring(0,len1-2);
    }
    
    relay = getValue(drokMessage,',',1); 
}


/*******************************************************************
 * Send a command to the Drok and wait for a reply.
 */
String drokCommand(String command)
{
  String response="";
  
  drokUART.println(command);
  response = drokUART.readStringUntil(10);
  delay(500); // The Drok controller is SLOW. 
              // If you send multiple commands too quickly, 
              // he gets confused.
  return response;  
}
