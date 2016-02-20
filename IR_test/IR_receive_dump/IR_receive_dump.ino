#include <IRLib.h>

//Create a receiver object to listen on pin 11
IRrecv My_Receiver(2);

//Create a decoder object
IRdecode My_Decoder;

void setup()
{
  Serial.begin(9600);
  My_Receiver.enableIRIn(); // Start the receiver
}

void loop() {
//Continuously look for results. When you have them pass them to the decoder
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();		//Decode the data
    My_Decoder.DumpResults();	//Show the results on serial monitor
    My_Receiver.resume(); 		//Restart the receiver
  }
}

