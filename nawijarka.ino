// nawijarka do cewek
#include <ModbusMaster.h>               //Library for using ModbusMaster

// instantiate ModbusMaster object
ModbusMaster node;

// deklarcja pinów
const int PUL1_pin = 13; //Pulse dla silnika wału
const int DIR1_pin = 12; //Dir dla silnika wału
const int PUL2_pin = 11; //Pulse dla silnika wozka
const int DIR2_pin = 10; //Dir dla silnika wozka
const int P1_pin = 3; // Przycisk awaryjny
const int P2_pin = 2; // Przycisk resetu

#define MAX485_DE      5
#define MAX485_RE_NEG  4
int warstwy;
int reset;
int calosc;
int jedna_warstwa;
int licznik;

// deklaracja startowych wartosci
uint8_t result;
int value=0;
bool step_wozek = false;
long STEPS_WOZEK, STEPS_WAL;  // ilosc krokow silnikiem wozka oraz walu
const int COEF_ACCEL = 200;  
long pozycja;
int timesDelay = 40;   // krotność opóźnienia 
int setDelay = 10;    // zadana krotność opóźnienia
int kt = 0;           // do spowolnienia zmiany prędkości 
long x, y, y_cal;
int predkosc, zatwierdz, kierunek_walu, kierunek_wozka, start_proces, opoznienie, stop_proces;
uint16_t dlugosc1, dlugosc2;
uint16_t  stopnie1, stopnie2;

//funkcja resetu
void(* resetFunc) (void) = 0;

//ustawienie zmiany kierunku transmisji rs485
void preTransmission(){
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

//Ustawienie parametrow startowych
void setup(){
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  // Modbus communication runs at 115200 baud
  Serial.begin(115200);

  // Modbus slave ID 1
  node.begin(1, Serial);
  // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  pinMode(PUL1_pin, OUTPUT); 
  pinMode(DIR1_pin, OUTPUT); 
  pinMode(PUL2_pin, OUTPUT); 
  pinMode(DIR2_pin, OUTPUT); 
  pinMode(P1_pin, INPUT_PULLUP);
  pinMode(P2_pin, INPUT_PULLUP);
  digitalWrite(DIR1_pin, HIGH);
  digitalWrite(DIR2_pin, HIGH);
  attachInterrupt(digitalPinToInterrupt(P1_pin), stop, FALLING);
  attachInterrupt(digitalPinToInterrupt(P2_pin), reset_fun, FALLING);
  //zatwierdzenie parametrów
  do{
    result = node.readCoils(0x00003, 1);
    if(result == node.ku8MBSuccess){
      zatwierdz = node.getResponseBuffer(0x00);  // rozpoczęcie procesu
    }
    result=0;
  }while(zatwierdz<=0);

  //Zczytanie wartosci z wyswietlacza (informacje odnosnie ilosci zwojow)
  result = node.readHoldingRegisters(0x40003, 4);
  if(result == node.ku8MBSuccess){
    licznik = node.getResponseBuffer(0x00);  
    warstwy = node.getResponseBuffer(0x01);  
    calosc = node.getResponseBuffer(0x02); 
    jedna_warstwa = node.getResponseBuffer(0x03);  
  }
  node.writeSingleCoil(0x00000, 0);

  //wybor kierunku walu
  result = node.readCoils(0x00004, 1);
  if(result == node.ku8MBSuccess){
    kierunek_walu = node.getResponseBuffer(0x00);
  }
  if(kierunek_walu <= 0){
    digitalWrite(DIR1_pin, LOW);  
  }
  else if(kierunek_walu >= 1){
    digitalWrite(DIR1_pin, HIGH); 
  }
        
  //wybor kierunku wozka
  result = node.readCoils(0x00005, 1);
  if(result == node.ku8MBSuccess){
    kierunek_wozka = node.getResponseBuffer(0x00);
  }
  if(kierunek_wozka <= 0){
    digitalWrite(DIR2_pin, LOW); 
  }
  else if(kierunek_wozka >= 1){
    digitalWrite(DIR2_pin, HIGH);
  }

  //Wybór parametrów
  result = node.readHoldingRegisters(0x40000, 3);
  if(result == node.ku8MBSuccess){
    stopnie1 = node.getResponseBuffer(0x02);  // zapisanie ilosci obrotow walu (w stopniach)  
    dlugosc1 = node.getResponseBuffer(0x01);  // zapisanie dlugosci nawijanej cewki (w milimetrach)
    opoznienie = node.getResponseBuffer(0x00); // zapisanie predkosci nawijania
  }

  //przypisywanie wartosci
  double wozek = 335.23*(double(dlugosc1)/100); //wyliczenie ilosci krokow wozka na podstawie dlugosci cewki
  double wal = 711.1111111111*(double(stopnie1)/10)*360; //wyliczenie ilosci krokow walu na podstawie ilosci stopni
  STEPS_WOZEK = long(wozek); 
  STEPS_WAL = long(wal); 
  predkosc = opoznienie;
  x = STEPS_WOZEK;
  y = STEPS_WAL;
  y_cal=y;
  pozycja = 2 * STEPS_WOZEK + STEPS_WAL;

  //funkcja rozpoczęcia procesu
  do{
  result = node.readCoils(0x00001, 1);
    if(result == node.ku8MBSuccess){
      start_proces = node.getResponseBuffer(0x00);  // rozpoczęcie procesu
      result=0;
    }
  }while(start_proces<=0);
}
    
//Glowna funkcja programu
void loop(){

  //zakonczenie programu (pozycja osiągnięta)
  while((x == 0) && (y == 0)){    
    node.writeSingleCoil(0x00000, 1); // wpisanie diody konca procesu
    //wpisywanie ilosci warstw
    if((digitalRead(DIR1_pin)==LOW) && (licznik==jedna_warstwa)){
      warstwy++;
      licznik = 0;
      node.writeSingleRegister(0x40003, licznik);
      node.writeSingleRegister(0x40004, warstwy);  
    }
    else if((digitalRead(DIR1_pin)==HIGH) && (licznik==0)){
      warstwy--;
      licznik=jedna_warstwa;
      node.writeSingleRegister(0x40003, licznik);
      node.writeSingleRegister(0x40004, warstwy);  
    } 
  resetFunc(); //reset programu
  }
  
  // wybor ruchu po x lub po y
  step_wozek = false;
  if (pozycja > 0){
    pozycja -= 2 * STEPS_WAL; 
    // step X
    step_wozek = true;
    if(x>0){
      x--;
    }
  }
  // step Y
  pozycja += 2 * STEPS_WOZEK; 
  step(step_wozek);
  if(y>0){
    y--;
  }

  // blok opóźnienia
  kt++;
  if (kt >= COEF_ACCEL){
    if((x>=0.9*STEPS_WOZEK) && (y>=0.9*STEPS_WAL)){
      if (timesDelay!=0){
        timesDelay--;
      }
    }
    if((x<=0.1*STEPS_WOZEK) && (y<=0.1*STEPS_WAL)){
      if (timesDelay!=setDelay){
        timesDelay++;
      }
    }
    kt = 0;
  }
  
  // pętla opóźniająca
  int t = timesDelay;
  do{
    delayMicroseconds(predkosc);   // minimalna długość przerwy między krokami
    t--;
  }while(t > 0);  

  //wpisywanie aktualnej ilości zwojów
  if(y%260000==0){
    if(digitalRead(DIR1_pin)==LOW){
      licznik++;
    calosc++;
    }
    else if(digitalRead(DIR1_pin)==HIGH){
      licznik--;
      calosc--;
    }
    if(licznik<0){
      licznik=jedna_warstwa-1;
      warstwy--;
      node.writeSingleRegister(0x40004, warstwy);  
    }
  node.writeSingleRegister(0x40003, licznik);
  node.writeSingleRegister(0x40005, calosc);
  }
}
 
// funkcja ruchu silnikow krokowch
void step(bool step_wozek) {
  if(step_wozek){
    digitalWrite(PUL2_pin, HIGH);
  }
  digitalWrite(PUL1_pin, HIGH);
  delayMicroseconds(1);     // długość impulsu step           
  digitalWrite(PUL1_pin, LOW);  
  digitalWrite(PUL2_pin, LOW);    
}

//przerwanie procesu (przycisk awaryjny)
void stop(){
  boolean zmienna = 0;
  int licznik1=0;
  while(licznik1!=100){
    licznik1++;
  }
  while(zmienna!=1){
    if(digitalRead(P1_pin)==LOW){
      zmienna=!zmienna;
    }
  }
  int licznik2=0;
  while(licznik2!=100){
    licznik2++;
  }
}

//funkcja resetu
void reset_fun(){
  resetFunc();
}
