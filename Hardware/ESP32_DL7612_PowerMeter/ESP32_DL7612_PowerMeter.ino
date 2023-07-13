#include <Arduino.h>
#include <Stream.h>
#include <HardwareSerial.h>
#include <stdlib.h>
long strtol (const char *__nptr, char **__endptr, int __base);

#define TX1_pin  16
#define RX1_pin  17
#define TX2_pin  18
#define RX2_pin  19

TaskHandle_t Task1;
TaskHandle_t Task2;
HardwareSerial myserial2(1);
HardwareSerial meserial(2);

bool send_sucress = false;
bool start_send = false;
bool repeat_enable = false;

String Voltage_str = "";
String Current_str = "";
String Power_str = "";
String K_WattHours_str = "";
String P_Factor_str = "";
String Freq_str = "";

double Voltage = 0;
double Current = 0;
double Power = 0;
double K_WattHours = 0;
double P_Factor = 0;
double Freq = 0;

uint8_t command[8] = {0x01, 0x03, 0x00, 0x48, 0x00, 0x08, 0xC4, 0x1A};

String str = "";
String payload = "";

const int ledPin = 2;

unsigned long previousMillis1 = 0;
const long interval1 = 300000;

unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
const long interval = 30000;

unsigned long me_previousMillis = 0;
unsigned long me_currentMillis = millis();
const long me_interval = 120000;

void setup()
{
  Serial.begin(115200);
  myserial2.begin(115200, SERIAL_8N1, RX1_pin, TX1_pin);
  meserial.begin(4800, SERIAL_8N1, RX2_pin, TX2_pin);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);


  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    Task1code,   /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(10);

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
    Task2code,   /* Task function. */
    "Task2",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task2,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */
  delay(10);

}
//Task1code: blinks an LED every 1000 ms
void Task1code( void * pvParameters ) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {

    // Pause the task for 1 ms
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    unsigned long me_currentMillis = millis();
    if (me_currentMillis - me_previousMillis >= me_interval)
    {
      me_previousMillis = me_currentMillis;
      meserial.write(command, 8);
      meserial.flush(); // tried with and without this line

    }
    if (meserial.available() > 0)
    {
      String Out_data = "";
      char buffer[100];
      while (meserial.available())
      {
        byte my_data = meserial.read();
        sprintf(buffer, "%02x", my_data);// Serial.print(buffer);
        Out_data = Out_data + String(buffer);
      }
      Serial.print(Out_data);
      Serial.print(" : " + String(Out_data.length()));
      String data2 = Out_data.substring(0, 70);
      String crc = Out_data.substring(70);
      //Serial.print(data2);
      //Serial.println(" : " + String(data2.length()));
      String calculated_crc = "";
      calculated_crc = ModRTU_CRC(data2); //Modbus message without crc code. Reply the calculated crc code
      //Serial.println("---------------------------------");
      //Serial.println("Modbus CRC Calculated: " + calculated_crc);
      crc.toUpperCase();
      if (calculated_crc.equals(crc))
      {
        Serial.println(" Data OK ...");
        Voltage_str = Out_data.substring(6, 14);
        Current_str = Out_data.substring(14, 22);
        Power_str = Out_data.substring(22, 30);
        K_WattHours_str = Out_data.substring(30, 38);
        P_Factor_str = Out_data.substring(38, 46);
        Freq_str = Out_data.substring(62, 70);

        //  Serial.println(Voltage_str);
        //  Serial.println(Current_str);
        //  Serial.println(Power_str);
        //  Serial.println(K_WattHours_str);
        //  Serial.println(P_Factor_str);
        //  Serial.println(Freq_str);

        Voltage = (strtol(Voltage_str.c_str(), NULL, 16)) / 10000.00;
        Current = (strtol(Current_str.c_str(), NULL, 16)) / 10000.00;
        Power = (strtol(Power_str.c_str(), NULL, 16)) / 10000.00;
        K_WattHours = (strtol(K_WattHours_str.c_str(), NULL, 16)) / 10000.00;
        P_Factor = (strtol(P_Factor_str.c_str(), NULL, 16)) / 1000.00;
        Freq = (strtol(Freq_str.c_str(), NULL, 16)) / 100.00;

        Serial.println("Voltage : " + String(Voltage, 4));
        Serial.println("Current : " + String(Current, 4));
        Serial.println("Power : " + String(Power, 4));
        Serial.println("K_Watt Hours : " + String(K_WattHours, 4));
        Serial.println("Power Factor : " + String(P_Factor, 4));
        Serial.println("Freq : " + String(Freq, 4));

      }
      else Serial.println(" Data Fault ...");
    }
    // Pause the task for 1 ms
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
void loop()
{
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= interval1)
  {
    previousMillis1 = currentMillis1;
    str = payload;
    start_Senddata();
  }
}
void start_Senddata()
{
  start_send = true;
  send_sucress = false;
}
void Send_data()
{
  if (start_send == true)
  {
    start_send = false;
    myserial2.println("AT+NCMGS=" + String(str.length()) + "," + str);
    repeat_enable = true;
    currentMillis = millis();
    previousMillis = currentMillis;
  }
  if (repeat_enable == true)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;
      if (send_sucress == false)
      {
        myserial2.println("AT+NCMGS=" + String(str.length()) + "," + str);
      }
    }
  }
}
void Read_serial()
{
  if (myserial2.available())
  {
    while (myserial2.available() > 0)
    {
      String text = myserial2.readStringUntil('\n');
      Serial.println(text);
      if (text.indexOf(F("+NNMI:3")) != -1)
      {
        send_sucress = true;
        repeat_enable = false;
        Serial.println(HexString2ASCIIString(getValue(text, ',', 1)));
        Serial.println("Send Sucress...");
      }
      if (text.indexOf(F("+ACK")) != -1)
      {
        send_sucress = true;
        repeat_enable = false;
        Serial.println("Send Sucress...");
      }
    }
  }
}
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
String  str2HexStr(String strin)
{
  int lenuse = strin.length();
  char charBuf[lenuse * 2 - 1];
  char strBuf[lenuse * 2 - 1];
  String strout = "";
  strin.toCharArray(charBuf, lenuse * 2) ;
  for (int i = 0; i < lenuse; i++)
  {
    sprintf(strBuf, "%02X", charBuf[i]);

    if (String(strBuf) != F("00") )
    {
      strout += strBuf;
    }
  }

  return strout;
}

String HexString2ASCIIString(String hexstring)
{
  String temp = "", sub = "", result;
  char buf[3];
  for (int i = 0; i < hexstring.length(); i += 2)
  {
    sub = hexstring.substring(i, i + 2);
    sub.toCharArray(buf, 3);
    char b = (char)strtol(buf, 0, 16);
    if (b == '\0')
      break;
    temp += b;
  }
  return temp;
}

// Compute the MODBUS RTU CRC
String ModRTU_CRC(String raw_msg_data)
{
  //Calc raw_msg_data length
  byte raw_msg_data_byte[raw_msg_data.length() / 2];
  //Convert the raw_msg_data to a byte array raw_msg_data
  for (int i = 0; i < raw_msg_data.length() / 2; i++)
  {
    raw_msg_data_byte[i] = StrtoByte(raw_msg_data.substring(2 * i, 2 * i + 2));
  }

  //Calc the raw_msg_data_byte CRC code
  uint16_t crc = 0xFFFF;
  String crc_string = "";
  for (int pos = 0; pos < raw_msg_data.length() / 2; pos++)
  {
    crc ^= (uint16_t)raw_msg_data_byte[pos];          // XOR byte into least sig. byte of crc
    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)

  //Become crc byte to a capital letter String
  crc_string = String(crc, HEX);
  crc_string.toUpperCase();

  //The crc should be like XXYY. Add zeros if need it
  if (crc_string.length() == 1)
  {
    crc_string = "000" + crc_string;
  }
  else if (crc_string.length() == 2)
  {
    crc_string = "00" + crc_string;
  }
  else if (crc_string.length() == 3)
  {
    crc_string = "0" + crc_string;
  }
  else
  {
    //OK
  }

  //Invert the byte positions
  crc_string = crc_string.substring(2, 4) + crc_string.substring(0, 2);
  return crc_string;
}

//String to byte --> Example: String = "C4" --> byte = {0xC4}
byte StrtoByte (String str_value)
{
  char char_buff[3];
  str_value.toCharArray(char_buff, 3);
  byte byte_value = strtoul(char_buff, NULL, 16);
  return byte_value;
}
