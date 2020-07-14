
//Extended Model

//Extra Features

//Usage
// #GO displays can messages on serial
// #ST stops displaying messages on serial
// #HW BAUD 0 sets baud to 9600
// #HW BAUD 1 sets baud to 38400
// #SM 01EF 4 30 32 34 36 sends a 4 byte can message to the bus
      //e.g Message ID: 01EF, Number of data bytes: 4, Byte Contents: 30 32 34 36
// #SM 00EF 8 E0 E1 E2 E3 E4 E5 E6 E7 sends a 8 byte message to the bus
      //e.g Message ID: 00EF, Number of data bytes: 8, Byte Contents: E0 E1 E2 E3 E4 E5 E6 E7

//Dynamic Baud Rate Change -> to verify
//Start/Stop CAN messages -> To verify
//Request CAN settings -> in progress
//Send/Receive CAN message in 11 bit format -> to verify
//Send/Receive CAN message in 29 bit format -> to do

//29 bits are represented with 2^5(32), so 5 bytes

#include <stdlib.h>
#include <string.h>
#include "mcp_can.h"
#include <SPI.h>
MCP_CAN CAN0(10); //setting CS pin to pin 10    

#define MAX_CMD_LENGTH 60

#define CR "\n\r"
#define CRCR "\n\r\n\r"
unsigned char stmp[8] = {0x30, 0x31, 0x32, 0x33, 0x34,0x35, 0x36, 0x37};//Array of memory addresses to send CAN message bytes to

//runs on boot or reset states
//notation is that e.g 9600 baud translates to 960 bytes per second, 
//for those who need to deal with word timing,here's a guide: bit_time = 1 / baud_rate
void setup() {
  //Set baud rate for interface
  Serial.begin(9600);
  Serial.println("System boot ok");

  //run initialization procedure
  if(CAN0.begin(CAN_250KBPS)== CAN_OK)
    Serial.println("CAN Controller Initiation complete. \n\r\n\r");
    else
      Serial.println("CAN Controller Initialization failed. Reset host. \n\r");
  
}//setup

//Program entry point

void loop() {

 SubCheckCANMessage();
 SubSerialMonitorCommand();

  }//loop

void SubCheckCANMessage(void){

  //Message ID: 01EF, Number of data bytes: 4, Byte Contents: 30 32 34 36
  //or 
  //Message ID: 00EF, Number of data bytes: 8, Byte Contents: E0 E1 E2 E3 E4 E5 E6 E7
    byte nMsgLen = 0;//from 0 to 8 bytes
    byte nMsgBuffer[8];//Byte Contents: E0 E1 E2 E3 E4 E5 E6 E7
    char sString[4];//Message ID: 01EF

   if(CAN0.checkReceive() == CAN_MSGAVAIL){

    //Read buffer
    CAN0.readMsgBuf(&nMsgLen, &nMsgBuffer[0]);//read length of message and contents of messge
    unsigned long nMsgId = CAN0.getCanId();//get can id of message

    //Print message on Serial
    Serial.print("Message ID: 0x");
     
     if(nMsgId <16)
      Serial.print("0");
     
     Serial.print(itoa(nMsgId, sString, 16));
     Serial.print("\n\r");

     //Print data to serial monitor
     Serial.print("Data: ");
     for(int nIndex; nIndex < nMsgLen; nIndex++){
      Serial.print("0x");
      
       if(nMsgBuffer[nIndex] < 16)
        Serial.print("0");

      Serial.print(itoa(nMsgBuffer[nIndex], sString, 16));
      Serial.print(CRCR);
      
    }//for
   }//if
  }//SubCheckCANMessage
void SubSerialMonitorCommand(void){

  //THis function parses the command and checks if it goes to CAN bus or to Hardware layer, #SM and #HW respectivvely. then it deals with it

  //Declarations
   char sString[MAX_CMD_LENGTH+1];//CAN String max length 61 bytes, Serial String length 61 bytes ---THE ENTIRE STRING MAX LENGTH
   bool bError = true;//error flag for error control
   
   int  nNewBaudRate = 0;//used to begin a new baudrate for baudrate switching at runtime
   int nMsgDisplay;//whether to display can msg or not
   int nModeId;//return 11 is standard, return 29 is extended
 
   //unrelated; viscous is sticky, non viscous isn't
   //continue

   unsigned long nMsgId = 0xFFFF;//message id e.g 0x1337 or 0x01EF or ...
   byte nMsgLen = 0;//Message length in bytes
   byte nMsgBuffer[8];//message buffer array of bytes

   //check length of string using the function below
   int nLen = nFctReadSerialMonitorString(sString);////return string length
   
   if (nLen > 0){//if there is a command from the serial, nLen is > 0
    //If you're sending to CAN BUS
    if(strncmp(sString, "#SM", 4) == 0){//compares the string and #SM by looking at at most 4 bytes, pick the first 4 characters
      
      if(strlen(sString) >= 10){//if the CAN string is more than 10 strings long
        //Determine message id and number of data bytes
        nMsgId = lFctCStringLong(&sString[4],4);//convert the four hex-digit numbers of the message id to an unsigned long
        nMsgLen = (byte)nFctCStringInt(&sString[9], 1);//convert the string to an integer which then becomes a byte

        if(nMsgLen >=0 && nMsgLen <=8){//if the message is between 0 and 8 bytes (inclusive) long
         
          int nStrLen = 10+nMsgLen*3; //Check if there are enough data entries
          if(strlen(sString)>= nStrLen){//if the command is larger than the string length
            int nPointer;//create a pointer to navigate
            for(int nIndex = 0; nIndex < nMsgLen; nIndex++){//go through the message bytes
              nPointer = nIndex * 3;//Blank character plus two numbers
              nMsgBuffer[nIndex] = (byte)nFctCStringInt(&sString[nPointer + 11], 2);//insert message into send buffer
            }//end for

            //Reset error flag
            bError = false;//no error 

            //Send the message

            CAN0.sendMsgBuf(nMsgId, CAN_STDID, nMsgLen, nMsgBuffer);//send message to bus

            if(nMsgDisplay == 1){
            Serial.print(sString);//Print on serial monitor
            Serial.print(CRCR);
            }

          }//end if
            }//end if
          }//end if
           
        }//end if
        
        //Check for entry error
        if(bError == true){
        Serial.print("???: ");
        Serial.print(sString);
        Serial.print(CR);
        }//end if
        
     }//end first if


    //Format: #HW BAUD 0, requirement: change baud at runtime thru serial EXPERIMENTAL
    //If you're sending to the hardware layer, i'm using switch statements 
    else if(&sString[1]=="H" && &sString[2]=="W"){  //check if it's going to the hardware layer
         int nMode = nFctCStringInt(&sString[7], 1);//check which mode has been entered. Mode 0 is 9600 Baud, Mode 1 is 38400 Baud

        //TWEAK HERE, mode 1 to get to your desired Baud - leave mode 0 as 9600 for a failsafe
        switch (nMode) {
         case 0:
           nNewBaudRate = 9600;
           break;
         case 1:
           nNewBaudRate = 38400;//115200 gives an overflow warning, future work to get it to work at 1MBps
           break;
         default:
           nNewBaudRate = 9600;
         }

        
        
            //Print on serial monitor
            Serial.print("Baud Rate will be updated. If the output becomes garbage change to the Baud Rate you inputted and it should come back to normal.");
            Serial.print(CRCR);
            delay(2000);//wait 2 secs for user to read
            //Flush serial
            Serial.flush();
            //Send the message
            Serial.begin(nNewBaudRate);
            Serial.print(nNewBaudRate);
            Serial.print(CRCR);
            }//end else if

            else if(&sString[1]=="G" && &sString[2]=="O"){
                nMsgDisplay = 1;
              }

             else if(&sString[1]=="S" && &sString[2]=="T"){
                nMsgDisplay = 0;
              }
             else if(&sString[1]=="C" && &sString[2]=="S"){
                Serial.print(nNewBaudRate);
                Serial.print(CR);
                Serial.print(nModeId);
                Serial.print(CR);
              }
              else

              Serial.print("Accessed Reserved Space");

           }//end SubSerialMonitorCommand
         
         
           
    //Read String from Serial Monitor Input
    byte nFctReadSerialMonitorString(char* sString){
      //declarations
      byte nCount = 0;

      if(Serial.available() > 0){
        Serial.setTimeout(100);
        nCount = Serial.readBytes(sString, MAX_CMD_LENGTH);
        }

        sString[nCount] = 0;

        return nCount;
    }//end nFctReadSerialMonitorString

    int  nFctCStringInt(char *sString, int nLen){
      //declarations
      int nNum;
      int nRetCode = 0;

      //check string length
      if(strlen(sString) < nLen)
        nRetCode = -1;
        else{
        
         //string length ok, convert number
          int nShift =0;
          for(int nIndex = nLen - 1; nIndex >= 0; nIndex--){
            if(sString[nIndex] >= '0' && sString[nIndex] <='9')
              nNum = int(sString[nIndex]- '0');
              
            else if(sString[nIndex] >= 'A' && sString[nIndex] <='F')
              nNum = int(sString[nIndex]- 'A') + 10;

            else 
               return nRetCode;
              
            nNum = nNum << (nShift++ * 4);
            nRetCode = nRetCode + nNum;

          }//end for
    }//end else

      return nRetCode;
      
   }//end nFctCStringInt
            

    unsigned long lFctCStringLong(char *sString, int nLen){
      //declarations
      
      unsigned long nNum;
      unsigned long nRetCode = 0;

      //check string length
      if(strlen(sString) < nLen)
        nRetCode = -1;
        else{
        
         //string length ok, convert number
          int nShift =0;
          for(int nIndex = nLen - 1; nIndex >= 0; nIndex--){
            if(sString[nIndex] >= '0' && sString[nIndex] <='9')
              nNum = int(sString[nIndex]- '0');
              
            else if(sString[nIndex] >= 'A' && sString[nIndex] <='F')
              nNum = int(sString[nIndex]- 'A') + 10;

            else 
               return nRetCode;
              
            nNum = nNum << (nShift++ * 4);
            nRetCode = nRetCode + nNum;

          }//end for
    }//end else

      return nRetCode;
      
   }//end lFctCStringLong

   
    
 
