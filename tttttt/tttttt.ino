#include <SoftwareSerial.h>
#include <EEPROM.h>
const int totalPhoneNo = 2;
String phoneNo[totalPhoneNo] = {"",""};
int offsetPhone[totalPhoneNo] = {0,13};
String tempPhone = "";
//GSM Module RX pin to Arduino 8
//GSM Module TX pin to Arduino 7
#define rxPin 7
#define txPin 8



SoftwareSerial sim900(rxPin,txPin);
String smsStatus,senderNumber,receivedDate,msg;
boolean isReply = false;

boolean DEBUG_MODE = 1;

void setup() {
  Serial.begin(19200);
  Serial.println("Arduino serial initialize");
  sim900.begin(19200);
  Serial.println("sim900 software serial initialize");
  smsStatus = "";
  senderNumber="";
  receivedDate="";
  msg="";
  Serial.println("List of Registered Phone Numbers");
  for (int i = 0; i < totalPhoneNo; i++){
    phoneNo[i] = readFromEEPROM(offsetPhone[i]);
    if(phoneNo[i].length() != 14)
      {phoneNo[i] = "";Serial.println(String(i+1)+": empty");}
    else
      {Serial.println(String(i+1)+": "+phoneNo[i]);}
  }
  delay(2000);
  sim900.print("AT+CMGF=1\r"); //SMS text mode
  delay(1000);
  //delete all sms
  sim900.println("AT+CMGD=1,4");
  delay(1000);
  sim900.println("AT+CMGDA= \"DEL ALL\"");
  delay(1000);

}

void loop() {
while(sim900.available()){
  parseData(sim900.readString());
}
while(Serial.available())  {
  sim900.println(Serial.readString());
}
} //main loop ends

void parseData(String buff){
  Serial.println(buff);

  unsigned int len, index;
  //Remove sent "AT Command" from the response string.
  index = buff.indexOf("\r");
  buff.remove(0, index+2);
  buff.trim();
  
  if(buff != "OK"){
    index = buff.indexOf(":");
    String cmd = buff.substring(0, index);
    cmd.trim();
    
    buff.remove(0, index+2);
    
    if(cmd == "+CMTI"){
      //get newly arrived memory location and store it in temp
      index = buff.indexOf(",");
      String temp = buff.substring(index+1, buff.length()); 
      temp = "AT+CMGR=" + temp + "\r"; 
      //get the message stored at memory location "temp"
      sim900.println(temp); 
    }
    else if(cmd == "+CMGR"){
      extractSms(buff);
      
      //----------------------------------------------------------------------------
      if(msg.equals("r") && phoneNo[0].length() != 14) {
        writeToEEPROM(offsetPhone[0],senderNumber);
        phoneNo[0] = senderNumber;
        String text = "Number is Registered: ";
        text = text + senderNumber;
        debugPrint(text);
        Reply("Number is Registered", senderNumber);
      }
      //----------------------------------------------------------------------------
      if(comparePhone(senderNumber)){
        doAction(senderNumber);
        //delete all sms
        sim900.println("AT+CMGD=1,4");
        delay(1000);
        sim900.println("AT+CMGDA= \"DEL ALL\"");
        delay(1000);
      }
      //----------------------------------------------------------------------------
    }
  }
  else{
  //The result of AT Command is "OK"
  }
}

void extractSms(String buff){
   unsigned int index;
   
    index = buff.indexOf(",");
    smsStatus = buff.substring(1, index-1); 
    buff.remove(0, index+2);
    
    senderNumber = buff.substring(3, 14);
    buff.remove(0,19);
   
    receivedDate = buff.substring(0, 20);
    buff.remove(0,buff.indexOf("\r"));
    buff.trim();
    
    index =buff.indexOf("\n\r");
    buff = buff.substring(0, index);
    buff.trim();
    msg = buff;
    buff = "";
    msg.toLowerCase();

    String tempcmd = msg.substring(0, 3);
    if(tempcmd.equals("r1=") || tempcmd.equals("r2=")){
        
        tempPhone = msg.substring(3, 16);
        msg = tempcmd;
        //debugPrint(msg);
        //debugPrint(tempPhone);
    }
}
void doAction(String phoneNumber){
  if(msg == "r2="){
    writeToEEPROM(offsetPhone[1],tempPhone);
    phoneNo[1] = tempPhone;
    String text = "Phone2 is Registered: ";
    text = text + tempPhone;
    debugPrint(text);
    Reply(text, phoneNumber); //phoneNumber
  }
  else if(msg == "list"){
      String text = "";
      if(phoneNo[0])
        text = text + phoneNo[0]+"\r\n";
      debugPrint("List of Registered Phone Numbers: \r\n"+text);
      Reply(text, phoneNumber);
  }
  if(msg == "del=all"){  
      writeToEEPROM(offsetPhone[0],"");
      phoneNo[0] = "";
      debugPrint("All phone numbers are deleted.");
      Reply("All phone numbers are deleted.", phoneNumber);
  }

  smsStatus = "";
  senderNumber="";
  receivedDate="";
  msg="";
  tempPhone = "";
}
void Reply(String text, String Phone)
{
    sim900.print("AT+CMGF=1\r");
    delay(1000);
    sim900.print("AT+CMGS=\""+Phone+"\"\r");
    delay(1000);
    sim900.print(text);
    delay(100);
    sim900.write(0x1A); //ascii code for ctrl-26 //sim900.println((char)26); //ascii code for ctrl-26
    delay(1000);
    Serial.println("SMS Sent Successfully.");
}

void writeToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = 13;
  strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + i, strToWrite[i]);
  }
}

String readFromEEPROM(int addrOffset){
  int len = 13;
  char data[len + 1];
  for (int i = 0; i < len; i++)
  {
    data[i] = EEPROM.read(addrOffset + i);
  }
  data[len] = '\0';
  return String(data);

}

boolean comparePhone(String number){
  boolean flag = 0;
  //--------------------------------------------------
  for (int i = 0; i < totalPhoneNo; i++){
    phoneNo[i] = readFromEEPROM(offsetPhone[i]);
    if(phoneNo[i].equals(number)){
      flag = 1;
      break;
    }
  }
  //--------------------------------------------------
  return flag;
}

 void debugPrint(String text){
  if(DEBUG_MODE == 1)
    Serial.println(text);
 }