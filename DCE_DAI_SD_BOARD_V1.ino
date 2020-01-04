/********************************************************************************
  DCE_DAI_SD_BOARD_V1

  By............: Pierre Durant
  Date..........: 01/08/2019
  Version.......: 1.0
  Modification : Full implementation of DAI's LOAD and SAVE routines + various DOS routines
  -------------------------------------------------------------------------------
  Wiring Layout
  -------------

  DCE Port Output               Arduino Input
  --------------------               -------------
  Name      Dir.   Pin                Name    Pin
  ----      ----   ---                ----    ---
  DATA BYTE A >        ......................22-23-24-25-26-27-28-29 connect to DCE 16-14-12-10-9-11-13-15
  DATA BYTE B <        ......................37-36-35-34-33-32-31-30 connect to DCE 30-31-32-25-24-23-22-21 !!! connect to a network of resistor 10k on the 5v pin 1 of DCE connector !!!
  ORDER       >        ......................49-48-47-46 connect to DCE 26-27-28-29
  ANSWER      <        ......................45-44-43-42 connect to DCE 20-19-18-17 !!! connect to a network of resistor 10k on the 5v pin 1 of DCE connector !!!
  -------------------------------------------------------------------------------
  LED RGB A:
  RED pin 10
  GREEN pin 11
  BLUE pin 12
  LED RGB B:
  RED pin 13
  GREEN pin 14
  BLUE pin 15
  -------------------------------------------------------------------------------
  SD-CARD on the standard arduino PIN for MEGA
  with CS connected on pin 53

********************************************************************************/

//***************************************
// VERSION 1
//***************************************


#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
SdFat sd;

//Sd2Card card;
//SdVolume volume;

File myFile;
File FIndex;

int DataA0   = 22;
int DataA1   = 23;
int DataA2   = 24;
int DataA3   = 25;
int DataA4   = 26;
int DataA5   = 27;
int DataA6   = 28;
int DataA7   = 29;

int DataB0   = 37;
int DataB1   = 36;
int DataB2   = 35;
int DataB3   = 34;
int DataB4   = 33;
int DataB5   = 32;
int DataB6   = 31;
int DataB7   = 30;

int DataC0   = 49;
int DataC1   = 48;
int DataC2   = 47;
int DataC3   = 46;
int DataC4   = 45;
int DataC5   = 44;
int DataC6   = 43;
int DataC7   = 42;


int ANSWER0    = DataC4;
int ANSWER1    = DataC5;
int ANSWER2    = DataC6;
int ANSWER3    = DataC7;

int ORDER0   = DataC0;
int ORDER1   = DataC1;
int ORDER2   = DataC2;
int ORDER3   = DataC3;

int BLUE = 12;
int GREEN = 11;
int RED = 10;
int BBLUE = 15;
int BGREEN = 14;
int BRED = 13;

bool BYTE_TRAITE = false;
bool BYTE_ENVOYE = false;
bool EOT = false;

int Ordre_envoye;
int Index;

const int chipSelect = 53;

const int SAVE = 3;
const int FINSAVE = 14;
const int LOAD = 4;
const int FINLOAD = 14;
const int DIR = 1;
const int DELETE = 2;
const int COPY = 6;
const int RENAME = 7;
const int ABOUT = 9;
const int FINPAR = 14;
const int SETCOMMAND1 = 12;
const int INDXAUTO = 5;

unsigned long StartTime;
unsigned long CurrentTime;
unsigned long ElapsedTime;

char file_nameC[255] = "";
String file_name = "";
String parameter = "";

enum States {
  READY,
  BUSY,
  ACK
} State;

//************************************************************************************************* SETUP *******

void setup()
{
  // Configure pins

  pinMode(DataA0, INPUT_PULLUP);
  pinMode(DataA1, INPUT_PULLUP);
  pinMode(DataA2, INPUT_PULLUP);
  pinMode(DataA3, INPUT_PULLUP);
  pinMode(DataA4, INPUT_PULLUP);
  pinMode(DataA5, INPUT_PULLUP);
  pinMode(DataA6, INPUT_PULLUP);
  pinMode(DataA7, INPUT_PULLUP);

  pinMode(DataB0, OUTPUT);
  pinMode(DataB1, OUTPUT);
  pinMode(DataB2, OUTPUT);
  pinMode(DataB3, OUTPUT);
  pinMode(DataB4, OUTPUT);
  pinMode(DataB5, OUTPUT);
  pinMode(DataB6, OUTPUT);
  pinMode(DataB7, OUTPUT);

  pinMode(ANSWER0, OUTPUT);
  pinMode(ANSWER1, OUTPUT);
  pinMode(ANSWER2, OUTPUT);
  pinMode(ANSWER3, OUTPUT);

  pinMode(ORDER0, INPUT_PULLUP);
  pinMode(ORDER1, INPUT_PULLUP);
  pinMode(ORDER2, INPUT_PULLUP);
  pinMode(ORDER3, INPUT_PULLUP);

  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);

  Serial.begin(500000);
  while (!Serial) {
    ;
  }

  digitalWrite(RED, 0);
  digitalWrite(GREEN, 0);
  digitalWrite(BLUE, 0);

  Serial.println("Initializing SD card...");

  if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
    digitalWrite(RED, 1);
    return;
  }

  digitalWrite(GREEN, 1);
  Send_Answer(0);
  Send_Char(0);

  // wait the SD-DOS has been activated on the DAI
  while (Decode_Order() != 0) {
    delay (1);
  }

}

//************************************************************************************************** SAVE *******

void Save()
{
  const int ENVOI = ORDER0;
  const int BIENRECU  = ANSWER0;
  bool FIN = false;
  byte car;

  myFile = sd.open(file_nameC, FILE_WRITE);
  myFile.seek(0);

  if (myFile) {
    //    Serial.println("Opening file OK");
  } else {
    //    Serial.println("Error opening file");
    LED_Error();
    return;
  }

  LED_Begin_Activity();

  State = READY;
  StartTime = millis();

  while (!FIN)
  {
    switch (State) {
      case READY:
        if (digitalRead(ENVOI) == 0)
          BYTE_TRAITE = false;
        digitalWrite(BIENRECU, LOW);
        if (digitalRead(ENVOI) == 1)
        {
          State = BUSY;
        }
        if (Decode_Order() == FINSAVE) {
          FIN = true;
        }
        break;

      case BUSY:
        car = ProcessChar();
        State = ACK;
        break;

      case ACK:
        digitalWrite(BIENRECU, HIGH);
        State = READY;
        break;
    }
  }
  myFile.close();
  CurrentTime = millis();
  if (Index > 0) {
    FIndex = sd.open("INDXSAVE", FILE_WRITE);
    if (FIndex) {
      FIndex.seek(0);
      FIndex.write(Index);
      FIndex.close();
    }
  }
  LED_End_Activity();
  //Serial.print("Elapsed Time: ");
  //Serial.println(CurrentTime - StartTime);
}

//***************************************************************************************** PROCESSCHAR *********

byte ProcessChar()
{
  byte Char;

  if (BYTE_TRAITE == false) {
    Char = digitalRead(DataA0) +
           (digitalRead(DataA1) << 1) +
           (digitalRead(DataA2) << 2) +
           (digitalRead(DataA3) << 3) +
           (digitalRead(DataA4) << 4) +
           (digitalRead(DataA5) << 5) +
           (digitalRead(DataA6) << 6) +
           (digitalRead(DataA7) << 7);
    myFile.write(Char);
    BYTE_TRAITE = true;
    return Char;
  }
}

//******************************************************************************************* GETNAME ***********

void Get_Name()
{
  const int ENVOI = ORDER0;
  const int BIENRECU  = ANSWER0;
  const int NOMFINI = ANSWER1;
  bool BYTE_TRAITE = false;
  bool FIN = false;
  byte car;
  int car_counter = 0;
  int long_nom = 0;
  int file_type = 0;
  int ordre;

  State = READY;
  Serial.println("GETNAME");

  file_name = String();

  while (!FIN)
  {
    switch (State) {
      case READY:
        if (digitalRead(ENVOI) == 0)
          BYTE_TRAITE = false;
        digitalWrite(BIENRECU, LOW);
        if (digitalRead(ENVOI) == 1)
        {
          State = BUSY;
        }
        ordre  = Decode_Order();
        if (ordre == FINSAVE) {
          FIN = true;
        }
        break;

      case BUSY:
        if (BYTE_TRAITE == false) {
          car = digitalRead(DataA0) +
                (digitalRead(DataA1) << 1) +
                (digitalRead(DataA2) << 2) +
                (digitalRead(DataA3) << 3) +
                (digitalRead(DataA4) << 4) +
                (digitalRead(DataA5) << 5) +
                (digitalRead(DataA6) << 6) +
                (digitalRead(DataA7) << 7);
          if ( (car_counter > 3) and (car_counter <= (3 + long_nom)) ) {
            file_name += char(car);
          }
          else if (car_counter == 2) {
            long_nom = car;
          }
          else if (car_counter == 0) {
            file_type = car;
          }
          car_counter += 1;
          BYTE_TRAITE = true;
        }
        State = ACK;
        break;

      case ACK:
        digitalWrite(BIENRECU, HIGH);
        State = READY;
        break;
    }
  }
  if (file_name == "") {
    Get_name_auto();
  }
  switch (file_type) {
    case 48:
      file_name += ".BAS";
      break;
    case 49:
      file_name += ".BIN";
      break;
    case 50:
      file_name += ".TAB";
      break;
    default:
      file_name += ".BAD";
      break;
  }
  file_name.toCharArray(file_nameC, 255);
  Serial.print("File Name: ");
  Serial.println(file_nameC);
  digitalWrite(NOMFINI, HIGH);
}

//***************************************************************************************************************

void Get_name_auto() {
  if (Ordre_envoye == SAVE) {
    FIndex = sd.open("INDXSAVE");
  }
  else {
    FIndex = sd.open("INDXLOAD");
  }
  if (FIndex) {
    Index = FIndex.read();
    FIndex.close();
  }
  else {
    Index = 0;
  }
  file_name = "AUTO";
  file_name.concat(Index);
  Index += 1;
}

//***************************************************************************************************************

void LED_Error() {
  digitalWrite(RED, 1);
  digitalWrite(GREEN, 0);
  digitalWrite(BLUE, 0);
  delay(2000);
  digitalWrite(RED, 0);
  digitalWrite(GREEN, 1);
  digitalWrite(BLUE, 0);
}

void LED_Final_Error() {
  digitalWrite(RED, 1);
  digitalWrite(GREEN, 0);
  digitalWrite(BLUE, 0);
}

void LED_Begin_Activity() {
  digitalWrite(RED, 0);
  digitalWrite(GREEN, 0);
  digitalWrite(BLUE, 1);
}

void LED_End_Activity() {
  digitalWrite(RED, 0);
  digitalWrite(GREEN, 1);
  digitalWrite(BLUE, 0);
}

//***************************************************************************************************************

void Load()
{
  const int ENVOI = ANSWER0;
  const int LERROR = ANSWER3;
  const int BIENRECU  = ORDER0;
  bool FIN = false;
  int rec;

  myFile = sd.open(file_nameC, FILE_READ);

  if (myFile) {
    //Serial.println("Opening file OK");
  } else {
    //Serial.println("Error opening file");
    digitalWrite(LERROR, HIGH);
    digitalWrite(ENVOI, HIGH);
    LED_Error();
    return;
  }

  LED_Begin_Activity();

  State = BUSY;
  StartTime = millis();

  while (!FIN)
  {
    switch (State) {
      case BUSY:
        //Serial.println("BUSY");
        if (digitalRead(BIENRECU) == LOW) {
          rec = myFile.read();
          if (rec != -1) {
            Send_Char(rec);
            State = ACK;
          }
          else {
            FIN = true;
            Send_Answer(FINLOAD);
            delay(50);
          }
        }
        break;

      case ACK:
        //Serial.println("ACK");
        digitalWrite(ENVOI, HIGH);
        State = READY;
        break;

      case READY:
        //Serial.println("READY");
        if (digitalRead(BIENRECU) == HIGH) {
          digitalWrite(ENVOI, LOW);
          State = BUSY;
        }
        break;
    }
  }
  myFile.close();
  CurrentTime = millis();
  LED_End_Activity();
  if (Index > 0) {
    FIndex = sd.open("INDXLOAD", FILE_WRITE);
    if (FIndex) {
      FIndex.seek(0);
      FIndex.write(Index);
      FIndex.close();
    }
  }
  Serial.print("Elapsed Time: ");
  Serial.println(CurrentTime - StartTime);
}

//***************************************************************************************************************

void Send_Char(int byt)
{
  if (bitRead(byt, 0) == 0) {
    digitalWrite(DataB0, LOW);
  }
  else {
    digitalWrite(DataB0, HIGH);
  }
  if (bitRead(byt, 1) == 0) {
    digitalWrite(DataB1, LOW);
  }
  else {
    digitalWrite(DataB1, HIGH);
  }
  if (bitRead(byt, 2) == 0) {
    digitalWrite(DataB2, LOW);
  }
  else {
    digitalWrite(DataB2, HIGH);
  }
  if (bitRead(byt, 3) == 0) {
    digitalWrite(DataB3, LOW);
  }
  else {
    digitalWrite(DataB3, HIGH);
  }
  if (bitRead(byt, 4) == 0) {
    digitalWrite(DataB4, LOW);
  }
  else {
    digitalWrite(DataB4, HIGH);
  }
  if (bitRead(byt, 5) == 0) {
    digitalWrite(DataB5, LOW);
  }
  else {
    digitalWrite(DataB5, HIGH);
  }
  if (bitRead(byt, 6) == 0) {
    digitalWrite(DataB6, LOW);
  }
  else {
    digitalWrite(DataB6, HIGH);
  }
  if (bitRead(byt, 7) == 0) {
    digitalWrite(DataB7, LOW);
  }
  else {
    digitalWrite(DataB7, HIGH);
  }
}

//***************************************************************************************************************

int Decode_Order()
{
  return ( digitalRead(ORDER0) +
           (digitalRead(ORDER1) << 1) +
           (digitalRead(ORDER2) << 2) +
           (digitalRead(ORDER3) << 3) );
}

//***************************************************************************************************************

void Send_Answer(int answer)
{
  if (bitRead(answer, 0) == 0) {
    digitalWrite(ANSWER0, LOW);
  }
  else {
    digitalWrite(ANSWER0, HIGH);
  }
  if (bitRead(answer, 1) == 0) {
    digitalWrite(ANSWER1, LOW);
  }
  else {
    digitalWrite(ANSWER1, HIGH);
  }
  if (bitRead(answer, 2) == 0) {
    digitalWrite(ANSWER2, LOW);
  }
  else {
    digitalWrite(ANSWER2, HIGH);
  }
  if (bitRead(answer, 3) == 0) {
    digitalWrite(ANSWER3, LOW);
  }
  else {
    digitalWrite(ANSWER3, HIGH);
  }
}

//***************************************************************************************************************

void printDirectory(SdFile dir) {
  const int ENVOIFIN = ANSWER2;
  char fnom[50];
  char ligneDirC[60];
  String ligneDir = "";
  SdFile lfile;

  Send_Message_Line(" ");

  bool OK = dir.getName(fnom, 50);
  if (sd.exists(fnom) and OK) {
    LED_Begin_Activity();
    while (true) {
      ligneDir = String();
      if (! lfile.openNext(&dir, O_RDONLY)) {
        //no more files
        break;
      }
      if (lfile.isDir()) {
        if (!lfile.getName(fnom, 50)) {
          //No Name !
        }
        ligneDir = fnom;
        ligneDir.concat("/  DIR");
        ligneDir.toCharArray(ligneDirC, 60);
        Send_Message_Line(ligneDirC);
      } else {
        // files have sizes, directories do not
        if (lfile.getName(fnom, 50)) {
          if (!lfile.isHidden())  {
            ligneDir = fnom;
            ligneDir.concat("  ");
            ligneDir.concat(lfile.fileSize());
            Send_Message_Line(ligneDir);
          }
        }
      }
      lfile.close();
    }
    //LED_End_Activity();
  } else {
    LED_Final_Error();
    Send_Message_Line("Directory don't exist !");
  }

  digitalWrite(ENVOIFIN, HIGH);
  delay(100);
  LED_End_Activity();
}

//***************************************************************************************************************

void AboutSDDOS() {
  const int ENVOIFIN = ANSWER2;

  LED_Begin_Activity();

  Send_Message_Line(" ");
  Send_Message_Line("SD-DOS for DAI 1.0");
  Send_Message_Line("By Pierre Durant - 2019");
  Send_Message_Line("(https://www.facebook.com/groups/431058947356275/)");
  Send_Message_Line(" ");
  Send_Message_Line("TESTMEM program By Bruno Vivien");
  Send_Message_Line("(//bruno.vivien.pagesperso-orange.fr/DAI/index.htm)");
  Send_Message_Line(" ");

  digitalWrite(ENVOIFIN, HIGH);
  delay(100);
  LED_End_Activity();
}


//***************************************************************************************************************

void Delete(String param)
{
  const int ENVOIMESSAGE = ANSWER2;
  String message = "";
  char paramC[255] = "";

  LED_Begin_Activity();
  param.toCharArray(paramC, 255);

  if (paramC != "") {
    if (!sd.remove(paramC))
    {
      //Serial.println(paramC);
      message = "ERROR, no file deleted";
      LED_Error();
    } else {
      message = "OK, file deleted";
    }
  } else {
    message = "ERROR, missing parameter";
    LED_Error();
  }

  //Serial.println(message);
  digitalWrite(ENVOIMESSAGE, HIGH);
  Send_Message_Line(message);
  digitalWrite(ENVOIMESSAGE, LOW);
  LED_End_Activity();
}

//***************************************************************************************************************

void Copy(String param)
{
  const int ENVOIMESSAGE = ANSWER2;
  String message = "";
  String paramB = "";
  char paramBC[50] = "";
  File myOrigFile;
  File myDestFile;
  int   data;
  bool error = false;

  byte ibuffer[3000];
  int ibufferspace = sizeof(ibuffer);

  LED_Begin_Activity();

  int position_separator = param.indexOf(">");
  if (position_separator != -1) {
    paramB = param.substring(position_separator + 1);
    param = param.substring(0, position_separator);
  }
  paramB.toCharArray(paramBC, 50);

  if (param != "" and paramB != "") {
    if (!(myOrigFile = sd.open(param, FILE_READ))) {
      LED_Error();
      message = "ERROR: opening source file failed";
      error = true;
    }

    if (sd.exists(paramBC)) {
      LED_Error();
      message = "ERROR: a file with the same name already exist";
      error = true;
    } else if (!(myDestFile = sd.open(paramB, FILE_WRITE))) {
      LED_Error();
      message = "ERROR: opening copy file for write failed";
      error = true;
    }

    if (!error) {
      while (myOrigFile.available() > 0)
      {
        int i = myOrigFile.readBytes(ibuffer, 64);
        myDestFile.write(ibuffer, i);
      }
      message = "OK, file copied !";
      if (!myOrigFile.close()) {
        LED_Error();
        message = "ERROR: closing source file failed";
      }
      if (!myDestFile.close()) {
        LED_Error();
        message = "ERROR: closing copy file failed";
      }
    }
  } else {
    LED_Error();
    message = "ERROR, missing parameter(s)";
  }

  //Serial.println(message);
  digitalWrite(ENVOIMESSAGE, HIGH);
  Send_Message_Line(message);
  digitalWrite(ENVOIMESSAGE, LOW);
  LED_End_Activity();
}

//***************************************************************************************************************

void Rename(String param)
{
  const int ENVOIMESSAGE = ANSWER2;
  String message = "";
  String paramB = "";
  char paramC[255] = "";
  char paramBC[255] = "";

  LED_Begin_Activity();

  int position_separator = param.indexOf(">");
  if (position_separator != -1) {
    paramB = param.substring(position_separator + 1);
    param = param.substring(0, position_separator);
  }
  param.toCharArray(paramC, 255);
  paramB.toCharArray(paramBC, 255);

  if (param != "" and paramB != "") {
    if (sd.exists(paramC)) {
      if (!sd.exists(paramBC)) {
        if (!sd.rename(paramC, paramBC)) {
          LED_Error();
          message = "ERROR: rename failed";
        } else {
          message = "OK, file renamed !";
        }
      } else {
        LED_Error();
        message = "ERROR: a destination file with such name already exist";
      }
    } else {
      LED_Error();
      message = "ERROR, no source file with such name";
    }
  } else {
    LED_Error();
    message = "ERROR, missing parameter(s)";
  }

  //Serial.println(message);
  digitalWrite(ENVOIMESSAGE, HIGH);
  Send_Message_Line(message);
  digitalWrite(ENVOIMESSAGE, LOW);
  LED_End_Activity();
}

//***************************************************************************************************************

void Manage_Auto_Index(String param)
{
  const int ENVOIMESSAGE = ANSWER2;
  String message = "";
  String paramB = "";
  String FileName = "";
  String MessageName = "";
  File Findex;
  int Index, NewIndex;

  LED_Begin_Activity();

  int position_separator = param.indexOf(">");
  if (position_separator != -1) {
    paramB = param.substring(position_separator + 1);
    param = param.substring(0, position_separator);
  }

  if (param == "L") {
    FileName = "INDXLOAD";
    MessageName = "LOAD";
  } else if (param == "S") {
    FileName = "INDXSAVE";
    MessageName = "SAVE";
  } else {
    MessageName = "ERROR";
  }

  if (MessageName != "ERROR") {
    Findex = sd.open(FileName);
    if (Findex) {
      Index = Findex.read();
      Findex.close();
    } else {
      Index = 0;
      Findex = sd.open(FileName, FILE_WRITE);
      Findex.seek(0);
      Findex.write(Index);
      Findex.close();
    }
    message = "INDEX for AUTO "  + MessageName + ": " + String(Index);
  } else {
    LED_Error();
    message = "ERROR: parameter(s) not correct !";
  }

  if (paramB != "") {
    NewIndex = paramB.toInt();
    if ((NewIndex > 0) or (NewIndex == 0 and paramB == "0")) {
      Findex = sd.open(FileName, FILE_WRITE);
      Findex.seek(0);
      Findex.write(NewIndex);
      Findex.close();
      message = "Index for AUTO " + MessageName + " : " + String(Index) + " => " + String(NewIndex);
    } else {
      LED_Error();
      message = "ERROR: second Parameter NOT numerical";
    }
  }

  //Serial.println(message);
  digitalWrite(ENVOIMESSAGE, HIGH);
  Send_Message_Line(message);
  digitalWrite(ENVOIMESSAGE, LOW);
  LED_End_Activity();
}

//***************************************************************************************************************

void Send_Message_Line(String Line)
{
  const int ENVOI = ANSWER0;
  const int BIENRECU  = ORDER0;
  const int FINLIGNE = ANSWER1;

  State = BUSY;

  int lngth = Line.length();
  int i = 0;
  while (i <= lngth) {
    switch (State) {
      case BUSY:
        Serial.println("BUSY");
        if (digitalRead(BIENRECU) == LOW) {
          Send_Char(Line[i]);
          i += 1;
          State = ACK;
        }
        break;

      case ACK:
        Serial.println("ACK");
        digitalWrite(ENVOI, HIGH);
        State = READY;
        break;

      case READY:
        Serial.println("READY");
        if (digitalRead(BIENRECU) == HIGH) {
          digitalWrite(ENVOI, LOW);
          State = BUSY;
        }
        break;
    }
  }
  digitalWrite(FINLIGNE, HIGH);
  delay(10);
  digitalWrite(FINLIGNE, LOW);
}

//***************************************************************************************************************

void Get_Parameter()
{
  const int ENVOI = ORDER0;
  const int BIENRECU  = ANSWER0;
  const int PARFINI = ANSWER1;
  bool BYTE_TRAITE = false;
  bool FIN = false;
  byte car;

  State = READY;
  //Serial.println("GETPARAM");

  parameter = String();

  delay(100);

  while (!FIN)
  {
    switch (State) {
      case READY:
        if (digitalRead(ENVOI) == 0)
          BYTE_TRAITE = false;
        digitalWrite(BIENRECU, LOW);
        if (digitalRead(ENVOI) == 1)
        {
          State = BUSY;
        }
        if (Decode_Order() == FINPAR) {
          FIN = true;
        }
        break;

      case BUSY:
        if (BYTE_TRAITE == false) {
          car = digitalRead(DataA0) +
                (digitalRead(DataA1) << 1) +
                (digitalRead(DataA2) << 2) +
                (digitalRead(DataA3) << 3) +
                (digitalRead(DataA4) << 4) +
                (digitalRead(DataA5) << 5) +
                (digitalRead(DataA6) << 6) +
                (digitalRead(DataA7) << 7);
          parameter += char(car);
          BYTE_TRAITE = true;
        }
        State = ACK;
        break;

      case ACK:
        digitalWrite(BIENRECU, HIGH);
        State = READY;
        break;
    }
  }
  //parameter.toCharArray(parameterC, 255);
  //Serial.print("Parameter: ");
  //Serial.println(parameter);
  digitalWrite(PARFINI, HIGH);
}

//***************************************************************************************************************

void loop()
{
  // test de présence/confirmité de la SD Card ?

  Index = 0;

  delay(15);
  int command = Decode_Order();
  switch (command) {

    case SAVE:
      Serial.println("SAVE");
      Ordre_envoye = SAVE;
      Send_Answer(1);
      while (Decode_Order() != 1) {
        ;
      }
      Send_Answer(0);
      Get_Name();
      while (Decode_Order() != 0) {
        ;
      }
      Send_Answer(0);
      Save();
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      file_name = "";
      break;

    case LOAD:
      Serial.println("LOAD");
      Ordre_envoye = LOAD;
      Send_Answer(1);
      while (Decode_Order() != 1) {
        ;
      }
      Send_Answer(0);
      Get_Name();
      while (Decode_Order() != 0) {
        ;
      }
      Send_Answer(0);
      Load();
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      file_name = "";
      break;

    case DELETE:
      Serial.println("DELETE");
      Ordre_envoye = DELETE;
      Send_Answer(1);
      while (Decode_Order() != 1) {
        ;
      }
      Send_Answer(0);
      Get_Parameter();
      while (Decode_Order() != 0) {
        ;
      }
      Send_Answer(0);
      Delete(parameter);
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      parameter = "";
      break;

    case RENAME:
      Serial.println("RENAME");
      Ordre_envoye = RENAME;
      Send_Answer(1);
      while (Decode_Order() != 1) {
        ;
      }
      Send_Answer(0);
      Get_Parameter();
      while (Decode_Order() != 0) {
        ;
      }
      Send_Answer(0);
      Rename(parameter);
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      parameter = "";
      break;

    case COPY:
      Serial.println("COPY");
      Ordre_envoye = COPY;
      Send_Answer(1);
      while (Decode_Order() != 1) {
        ;
      }
      Send_Answer(0);
      Get_Parameter();
      while (Decode_Order() != 0) {
        ;
      }
      Send_Answer(0);
      Copy(parameter);
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      parameter = "";
      break;

    case INDXAUTO:
      Serial.println("INDXAUTO");
      Ordre_envoye = INDXAUTO;
      Send_Answer(1);
      while (Decode_Order() != 1) {
        ;
      }
      Send_Answer(0);
      Get_Parameter();
      while (Decode_Order() != 0) {
        ;
      }
      Send_Answer(0);
      Manage_Auto_Index(parameter);
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      parameter = "";
      break;

    case ABOUT:
      Serial.println("ABOUT");
      Ordre_envoye = ABOUT;
      Send_Answer(1);
      AboutSDDOS();
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      break;

    case DIR:
      Serial.println("DIR");
      Ordre_envoye = DIR;
      Send_Answer(1);
      SdFile root;
      root.open("/");
      printDirectory(root);
      Ordre_envoye = 0;
      Send_Answer(0);
      Send_Char(0);
      break;

  }

}
