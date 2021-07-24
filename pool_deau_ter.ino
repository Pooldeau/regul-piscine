/*
  Pooldeau 
  Régulation PH & ORP piscine
  Gestion du temps de filtration
  Mode hivernage actif
*/
#include <EEPROM.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 6
#include <OneWire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);
#include <SPI.h>
#include <Ethernet.h>
#include <ds3231.h>
#include <Wire.h>
//#include <config.h>
//EthernetUDP udp;
#include <time.h>
//en mode hivernage: definition des heures de filtation en fonction de la temmpérature
int heure_hivernage[4] [6]={
{0,6,12,18,22,0},//T°>12
{0,6,14,20,0,0,},//<12>8
{0,10,18,0,0,0,},//<7>0
{0,2,6,12,18,22},//<-1 >-5
// -6°c= 24/24
};
uint8_t drapeau=1;
char jour;
char mois;
char heure;
char minute;
char seconde;
uint8_t bouton_pin_ph=7;
uint8_t bouton_etat_ph=1;
uint8_t bouton_pin_chl = 8;
uint8_t bouton_etat_chl=1;
uint8_t bouton_pin_hiv=3;
uint8_t bouton_etat_hiv=0;
uint8_t flag_ph=0;
uint8_t flag_chl=0;
float tension,valvolt;//Déclaration des variables tension et pression
float pression;
float val_ph;
float val_chl;
float val_bar;
int temp_pisc;
int temp_ext;
uint8_t modulo;//modulo
uint8_t  etat_pompe=0;
uint8_t heure_departpompe=11;//**************************************parametrable ici départ pompe 11h***********************************
//uint8_t tempscycleph;// init du cycle d'injection ph
//uint8_t tempscyclechl;//  init du cycle d'injection chl
uint8_t tempspompe;
uint8_t heurefin;
uint8_t compteur_ph;
uint8_t compteur_chl;
uint8_t debut_ph=20;//*************************************injection si beoin PH toutes les 2 heures impaires parametrable ici à 20mn********************************
uint8_t debut_chl=20;//************************************injection si besoin CHL toutes les 2 heures paires parametrable ici à 20mn******************************
uint8_t tempo_ph;
uint8_t tempo_chl;
uint8_t DELTA_AVEC_UTC;
uint8_t temp_hiver;
float total_ph; 
float bidon_ph;
float total_chl; 
float bidon_chl;
float ph_moins;
float chl_moins;
struct ts t;

// On définit le OneWire
OneWire oneWire(ONE_WIRE_BUS);
// Et on le passe en réference de la librairie Dallas temperature
DallasTemperature sensors(&oneWire);
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 50);
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 for HTTP):
EthernetServer server(80);

void setup() {//*****************************************************setup**************************************
Serial.begin(9600);
// EEPROM.get(20,tempscycleph);
 delay(1);
pinMode(bouton_pin_hiv,INPUT_PULLUP); 
pinMode(7,INPUT_PULLUP);// BP init ph
pinMode(8, INPUT_PULLUP);//BP init ORP
pinMode (2,OUTPUT);//relais pompe
pinMode (4,OUTPUT);//chien de garde
pinMode (9,OUTPUT);//relais chlore
pinMode (5,OUTPUT);//relaisPH
pinMode(A2, OUTPUT);//BUZZER
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  // Check for Ethernet hardware present
if (Ethernet.hardwareStatus() == EthernetNoHardware) {
   //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no pouint8_t running without Ethernet hardware
    }
}
  // start the server
  server.begin();
  //Serial.print("Adresse IP :  ");
  //Serial.println(Ethernet.localIP());
 

 //*******************************************On initialise les ds18b20 ************************
  sensors.begin();
  Wire.begin();
  //*******************************************init RTC************************************************
  DS3231_init(DS3231_INTCN);
  //********************************************init LCD*********************************************
  lcd.init();
  lcd.backlight();
 //********************************************relais  au repos**********************************
  digitalWrite(2, LOW);//relais pompe au repos
  etat_pompe=0;
  digitalWrite(5, LOW);//relais ph au repos
  digitalWrite(9, LOW);//relais chl au repos
  digitalWrite(4, LOW);//sortie chien de garde
 // EEPROM.get(20,tempscycleph);
//*********************************************sauvegarde du bidon ph...*********************************************
if (bidon_ph == 20.00){    
  EEPROM.put(0,bidon_ph);  // mise en memoire e²prom en cas de coupure secteur
  delay(1);
  compteur_ph=0;
  EEPROM.put(4,compteur_ph);
  //Serial.println (bidon_ph);
   }
  EEPROM.get(0,bidon_ph);
  EEPROM.get(4,compteur_ph);
  //*********************************************sauvegarde du bidon chl...*********************************************
if (bidon_chl == 20.00){
  EEPROM.put(8,bidon_chl);   //idem ph
  delay(1);
  compteur_chl=0;
  EEPROM.put(6,compteur_chl);
 // Serial.println (bidon_chl);
  }
  EEPROM.get(8,bidon_chl);
  EEPROM.get(6,compteur_chl);
 // Serial.println (adresse);
  lcd.clear();
  //************************************************T°piscine au depart
/*temp_pisc=(sensors.getTempCByIndex(0),1)-1;
 delay(1);
 EEPROM.put(10,temp_pisc); 
 EEPROM.get(10,temp_pisc);*/
delay(20);

//*************************************sauvegarde temps de filtration************************************
 //EEPROM.put(20,tempspompe); 
EEPROM.get(20,tempspompe); 
}
//**********************************************LOOP  ***********************************************************************************************
void loop() {
chien_de_garde();
//Serial.println("Reset loop"); 
 // sensors.reset ();
 sensors.requestTemperatures();
 
 delay(1);

 temp_pisc=(sensors.getTempCByIndex(0))-1;
 if (t.hour==14 and t.min==5 and t.sec <=2){
 tempspompe=round (0.175*36/12.3*(temp_pisc-8));// 36= volume piscine- 12,3=débit pompe
 //tempspompe=(temp_pisc/2)-3;//Calcul basique du temps de filtration
 delay(1);
 EEPROM.put(20,tempspompe);
 delay(5); 
 EEPROM.get(20,tempspompe);
 delay(5);
 }
 temp_ext=(sensors.getTempCByIndex(1));
 delay(1); 
 
//************************************************ T° BASSE PASSEZ EN MODE HIVERNAGE*******************************
if (temp_pisc < 10 and t.hour>=heure_departpompe){
    lcd.clear();
    lcd.setCursor(2,1);
    lcd.print ("PASSEZ EN MODE");
    lcd.setCursor(5,3);
    lcd.print ("HIVERNAGE");
    delay(3000);
    lcd.clear();
}

  //*****************************************************MODE HIVERNAGE************************************************
  bouton_etat_hiv=digitalRead(bouton_pin_hiv);
 while (bouton_etat_hiv==0){
    digitalWrite(2, LOW);
    etat_pompe=0;
    digitalWrite(5, LOW);
    digitalWrite(9, LOW);
    DS3231_get(&t);
    bouton_etat_hiv=digitalRead(bouton_pin_hiv);
    sensors.requestTemperatures();
    temp_ext=(sensors.getTempCByIndex(1));
  delay(1);
    //Serial.println ("switch");
    if (drapeau==0) {
      lcd.clear();
      drapeau=1;}
  lcd.setCursor(4,0);
  lcd.print(F("MODE HIVERNAGE"));
  lcd.setCursor(4,1);
  lcd.print(F("HEURE:"));lcd.print(t.hour);lcd.print(F("H"));
  if (t.min < 10){ lcd.print(F("0"));}
  lcd.print(t.min);lcd.print(F("        "));
  web();
 //************************************************************1 TEMPERATURE*********************************************
if (temp_ext >= 13 ) {
  temp_hiver=1;
  lcd.setCursor(4,2);
  lcd.print(F("TEMP:"));lcd.print(temp_ext);lcd.write(223);lcd.print(F("C"));lcd.print(F(" 5X1H      "));
  lcd.setCursor(0,3);
  lcd.print(F(" 00H,06H,12H,18H,22H    "));
  uint8_t i;
    for (i = 0; i < 5; i = i + 1) {
    while (heure_hivernage [0] [i]==t.hour ){
      DS3231_get(&t);
      delay(1);
      sensors.requestTemperatures();
      temp_ext=(sensors.getTempCByIndex(1));
      delay(1);
      digitalWrite(2, HIGH);
      chien_de_garde(); 
     // temp_hiver=1;
      web();
      etat_pompe=1;}
    
digitalWrite(2, LOW);
etat_pompe=0;
    }     
 }
  
 //*************************************************************2 TEMPERATURE*********************************************************************************   
if (temp_ext <13 and temp_ext > 8 ){
  temp_hiver=2;               
  lcd.setCursor(4,2);
  lcd.print(F("TEMP:"));lcd.print(temp_ext);lcd.write(223);lcd.print(F("C"));lcd.print(F(" 4X1H  "));
  lcd.setCursor(0,3);
  lcd.print(F("   00H,06H,14H,20H   "));
  uint8_t i;
    for (i = 0; i < 4; i = i + 1) {
    while (heure_hivernage [1] [i]==t.hour ){
      DS3231_get(&t);
      sensors.requestTemperatures();
      temp_ext=(sensors.getTempCByIndex(1));
      delay(1);
      digitalWrite(2, HIGH);
      chien_de_garde(); 
     // temp_hiver=2;
      web();
      etat_pompe=1;}
digitalWrite(2, LOW);
etat_pompe=0;
  }
 } 
       
 //*************************************************************3 TEMPERATURE*********************************************************************************   
    
if (temp_ext <=8 and temp_ext >= 0) {
  temp_hiver=3;
  lcd.setCursor(4,2);
  lcd.print(F("TEMP:"));lcd.print(temp_ext);lcd.write(223);lcd.print(F("C"));lcd.print(F(" 3X1H "));
  lcd.setCursor(0,3);
  lcd.print(F("    00H,10H,18H       "));
    uint8_t i;
    for (i = 0; i < 3; i = i + 1) {
    while (heure_hivernage [2] [i]==t.hour ){
      DS3231_get(&t);
      sensors.requestTemperatures();
      temp_ext=(sensors.getTempCByIndex(1));
      delay(1);
      digitalWrite(2, HIGH);
      chien_de_garde(); 
     // temp_hiver=3;
      web();
      etat_pompe=1;}
digitalWrite(2, LOW);
etat_pompe=0;
    }          
}
  
  //*************************************************************4 TEMPERATURE*********************************************************************************
 
  if (temp_ext <= -1  and temp_ext >= -5) {
    temp_hiver=4;
    digitalWrite(2, LOW);
    lcd.setCursor(4,2);
    lcd.print(F("TEMP:"));lcd.print(temp_ext);lcd.write(223);lcd.print(F("C"));lcd.print(F(" 6X1H  "));
    lcd.setCursor(0,3);
    lcd.print(F("0H,2H,6H,12H,18H,20H"));
      uint8_t i;
    for (i = 0; i < 6; i = i + 1) {
    while (heure_hivernage [3] [i]==t.hour ){
        DS3231_get(&t);
        sensors.requestTemperatures();
        temp_ext=(sensors.getTempCByIndex(1));
        delay(1);
        digitalWrite(2, HIGH);
        chien_de_garde(); 
       // temp_hiver=4;
        web();
        etat_pompe=1;}
  digitalWrite(2, LOW);
  etat_pompe=0;
    }   
     }
   
  //*************************************************************5 TEMPERATURE*********************************************************************************    
bouton_etat_hiv=digitalRead(bouton_pin_hiv);
while (temp_ext <= -6 and bouton_etat_hiv==0)  {
  bouton_etat_hiv=digitalRead(bouton_pin_hiv);
  DS3231_get(&t);
  lcd.print(F("  HEURE:"));lcd.print(t.hour);lcd.print(F("H"));
  /*if (t.min < 10){ lcd.print("0");lcd.print(t.min);lcd.print("        ");}*/
  sensors.requestTemperatures();
  temp_ext=(sensors.getTempCByIndex(1));
  delay(1);
  lcd.setCursor(0,2);
  lcd.print(F("TEMP:"));lcd.print(temp_ext);lcd.write(223);lcd.print(F("C"));lcd.print(F(" 24X1H  "));
  lcd.setCursor(0,7);
  lcd.print(F("       24/24        "));
  etat_pompe=1;
  temp_hiver=5;
  web();
  digitalWrite(2, HIGH);}
  chien_de_garde(); 
 
web();
 }
 
 if (drapeau==1) {
    lcd.clear();
    drapeau=0;}
//*****************************************************changement bidon ph*************************************************************************

bouton_etat_ph=digitalRead(bouton_pin_ph);
while (bouton_etat_ph == 0 ){
  
 bouton_etat_ph=digitalRead(bouton_pin_ph);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("INIT PH : 20.00 L ");
   
  bidon_ph=20.00;// faire bip
  EEPROM.put(0,bidon_ph);
  delay(1);
  
  compteur_ph=0;
  bip();
  delay (3000);
 chien_de_garde(); 
  EEPROM.put(4,compteur_ph);
  delay (1);
  lcd.setCursor(0,0);
  lcd.print("                    ");
}
//********************************************************changement bidon chl*****************************
bouton_etat_chl=digitalRead(8);
//delay(1000);//*****************************************************************************************************************************
while (bouton_etat_chl == 0){
  bouton_etat_chl=digitalRead(bouton_pin_chl);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("INIT ORP : 20.00 L "));
 
  bidon_chl=20.00;// faire bip
  EEPROM.put(8,bidon_chl);
  delay(1);
  compteur_chl=0;
   bip();
   delay (3000);
   chien_de_garde(); 
  EEPROM.put(6,compteur_chl);
  delay (1);
  lcd.setCursor(0,0);
  lcd.print(F("                    "));
} 
 DS3231_get(&t);
  //*************************************************calcul du PH*******************************************
           uint8_t (analogChannel)=1;               
           uint8_t sensorReading;
           total_ph=0.00;
           for (uint8_t i = 1; i<= 100; i++) {
             total_ph=analogRead(analogChannel);
              delay (1);
              val_ph=total_ph+val_ph;
           }
           total_ph=val_ph/100;
           val_ph = ( total_ph/ 73.071);
  //********************************************calcul du chlore*********************************************************
    total_chl=0.0;
    for (uint8_t i = 1; i<= 100; i++) {
    total_chl = analogRead(A0);
    delay (5);
    val_chl=total_chl+val_chl;
     //Serial.println(total_chl);
    }
  total_chl =val_chl/100*5000/1023/1000;
  Serial.println(total_chl);
  val_chl=(((2.5-total_chl)/1.037)*1000);//+0-250

 //val_chl=(1/(total_chl-0.37))*177;
 if (val_chl<340){
  val_chl=val_chl-50;
 }
      
//*******************************************calcul de la pression********************************************************
     tension = analogRead(A3);
     valvolt= ((tension * 5) /1023.0);//Lecture de la tension et conversion en Volts
     if (valvolt <=0.3) {
        valvolt=0.0;
      }
     pression = (valvolt /(1,6*5+0,1)-0.5); //Obtention de la pression en bar
     //Serial.print("pression  ");
     //Serial.println(pression,1);
     delay (1);
     if (pression<0.3){
      pression=0;//****************************************************pour test*****************************************************************************        
     }
   //***************************************affichage DHM sur LCD************************************
    lcd.setCursor(0,0); // positionne le curseur à la premiere colonne de la premiere ligne
    if (t.mday < 10) lcd.print(F("0"));
      lcd.print(t.mday); lcd.print(F("/"));
    if (t.mon < 10) lcd.print(F("0"));
      lcd.print(t.mon); lcd.print(F(" "));
    if (t.hour < 10) lcd.print(F(" "));
      lcd.print(t.hour); lcd.print(F(":"));
    if (t.min < 10) lcd.print(F("0"));
      lcd.print(t.min); lcd.print(F(" "));
    if (t.sec < 10) lcd.print(F("0"));
      lcd.print(t.sec);lcd.print(F(" "));

  if (t.mday < 10){ jour='0';}
      else {jour='\0';}
    if (t.mon <10){ mois= '0';}
      else {mois='\0';}
    
    
    if (t.hour < 10){ heure='0';}
      else {heure='\0';}
    if (t.min < 10){ minute ='0';}
      else {minute='\0';}
    
//   ***********************************affichage température sur LCD*******************************
  lcd.setCursor(0,1);
  lcd.print(F("PISC:"));
  lcd.print(sensors.getTempCByIndex(0),1);// temperature piscine*****************************
  //lcd.print(temp_pisc);
  delay(1);
//   Symbole degré
  lcd.write(223);
  lcd.print(F("C  "));
  lcd.setCursor(12,1);
  lcd.print(F("EXT:"));
  lcd.print(temp_ext);// temperature exterieure*****************************
  lcd.write(223);
  lcd.print(F("C"));

  //************************************************affichage ph et chlore sur LCD********************************************
      lcd.setCursor(0,2);
      lcd.print(F("PH: "));
      lcd.print(val_ph,1);
      lcd.print(F(" ORP: "));
      lcd.print(val_chl,0);lcd.print(F(" MV "));
//******************************************************affichage durée de fonctionnement pompe****************************************
      heurefin=heure_departpompe+tempspompe;
      lcd.setCursor(0,3);
      if (heurefin > t.hour) {
        if (flag_chl==1){
          lcd.setCursor(0,3);
         lcd.print(F("INJ ORP EN COURS    "));
        }
          else if (flag_ph==1){
          lcd.print(F("INJ PH EN COURS     "));
        }
        else if (flag_chl==0 and flag_ph==0){
        lcd.setCursor(0,3);
        lcd.print(F("POMPE:"));
        lcd.setCursor(6,3);
        lcd.print(heure_departpompe);
        lcd.setCursor(8,3);
        lcd.print(F("A"));
        lcd.print(heurefin);
        lcd.print(F("H "));
        lcd.setCursor(13,3);
        lcd.print(pression,1);
        lcd.print(F("BAR"));
      }
      
        }
    drapeau=0;
     
    web();

 /* calcul DELTA_AVEC_UTC : heure d'hiver d'heure d'été 
  if ((t.mon > 3) && (t.mon < 10) || ((t.mon == 3) && (t.mday > 24) && (((t.mday) > 24) || (t.wday == 7))) || ((t.mon == 10) && ((t.wday < 25) || (((t.wday) < 25) && (t.wday != 7)))))
  {
    DELTA_AVEC_UTC =2;}
  else{
    DELTA_AVEC_UTC =1;
  }
  Serial.println(DELTA_AVEC_UTC);*/
   
    
    if ((t.hour)==0){// init compteur_ph en fin de journée
   compteur_ph=0;
   compteur_chl=0;}

   if ((t.hour)==0 and t.min==0 and t.sec<=2){// reboot en fin de journée
   chien_de_garde();}
 //**************************************************************/***vers fonction relais pompe***************************************   
  heurefin=heure_departpompe+tempspompe;
  if ((t.hour)>=heure_departpompe){
  relais_pompe();}

         modulo=t.hour%2;// fonctionnement  pompe d'injection  sur heures paires et impaires
         //Serial.print("modulo ");
         //Serial.println(modulo);
     if ((t.hour)>=heure_departpompe and modulo==1 and bidon_ph >0.5){//fonctionnement  pompe d'injection PH sur heures impaires
    relais_ph();//*********************************************************************vers fonction relais PH******************************************
   } 
   if ((t.hour)>=heure_departpompe and modulo==0 and bidon_chl >0.5 ){ //fonctionnement  pompe d'injection ORP sur heures paires
    relais_chlore();}//********************************************************************vers fonction relais chlore**************************************
  
//**********************************************avertissement bidon presque vide***************************************
 
 if (bidon_ph <= 0.5 and t.sec >= 58){
  lcd.clear();
  lcd.setCursor(1,1);
  lcd.print (F("BIDON PH A CHANGER"));
  bip_bip();
  delay(3000);
 }  
 if (bidon_chl <= 0.5 and t.sec <=2){
  lcd.clear();
  lcd.setCursor(1,1);
  lcd.print (F("BIDON CHL A CHANGER"));
  bip_bip();
  delay(3000);
 }  
} 
    
void relais_pompe(){//******************************************************************** fonction relais pompe******************
  
  heurefin=heure_departpompe+tempspompe;
  if ((t.hour)<heurefin){
    //Serial.println("pompe en marche");
    //Serial.print("temps cycle PH  ");
    //Serial.println(tempscycleph);
     
    digitalWrite(2, HIGH); //relais pompe en 4 de H6 pompe en marche
    etat_pompe=1;
  }
   else{digitalWrite(2,LOW);
   etat_pompe=0; 
   lcd.setCursor(0,3);
   lcd.print (F("     ARRET POMPE    "));}
  //********************************************************** probleme pompe filtation pression basse ****************************************************
   if (pression<=0.3 and etat_pompe==1 and heurefin < t.hour and t.sec >=10){
   digitalWrite(2,LOW);// arret pompe
   etat_pompe=0;
   delay(2000);
   lcd.clear();
   lcd.setCursor(6,2);
   lcd.print(F("PROBLEME"));
   lcd.setCursor(2,3);
   lcd.print(F("POMPE FILTRATION"));
   bip_bip();
   delay(2000);
   lcd.clear();
   }
   //********************************************************** probleme pompe filtation pression élevée ****************************************************
  if (pression>=0.7 and etat_pompe==1){
    delay(2000);
   lcd.clear();
   lcd.setCursor(6,2);
   lcd.print(F("NETTOYER "));
   lcd.setCursor(3,3);
   lcd.print(F("FILTRE A SABLE "));
   bip_bip();
   delay(2000);
   lcd.clear();
   }
   }
      
void relais_chlore(){  //******************************************************************* fonction relais chlore******************
  
  if (val_chl<600 ){
    tempo_chl=4;
    chl_moins=0.8;
  }

  else if (val_chl>=600 and val_chl <700){
    tempo_chl=2;
    chl_moins=0.4;
  }

  else if (val_chl>=700 and val_chl <750){
    tempo_chl=1;
    chl_moins=0.2;
  }
  
  if (t.hour < heurefin and val_chl >= 200 and (t.min)==debut_chl){ 
    digitalWrite(9, HIGH); //relais chlore en marche
    //Serial.print ("   INJ ORP   "); 
    /*lcd.setCursor(0,3);
    lcd.print ("   INJ ORP   "); */
    flag_chl=1;
    }
    if ((t.min)==debut_chl+tempo_chl and flag_chl==1 ){
    digitalWrite(9, LOW); //relais chl: arret pompe chl
    //Serial.println("pompe ORP à l'arret"); 
    bidon_chl = bidon_chl-chl_moins;
    compteur_chl=compteur_chl+tempo_chl;
    flag_chl=0;
    EEPROM.put(8,bidon_chl);
    delay(1);
    EEPROM.put(6,compteur_chl);
    delay(1);
    EEPROM.get(6,compteur_chl);
    EEPROM.get(8,bidon_chl);
    Serial.println(compteur_chl); 
    delay(1);
  }
}
void relais_ph(){ //******************************************************************* fonction relais ph******************
  if (val_ph>7.4 and val_ph <7.6){
    tempo_ph=1;
    //compteur_ph=compteur_ph+1;
    ph_moins=0.2;
  }
    else if (val_ph>=7.6 and val_ph<7.8){
    tempo_ph=2;
    //compteur_ph=compteur_ph+2;
    ph_moins=0.4;
   }
      else if (val_ph>=7.8 and val_ph<8.0){
    tempo_ph=3;
    //compteur_ph=compteur_ph+3;
    ph_moins=0.6;
   } 
        else if (val_ph>=8.0){
    tempo_ph=4;
    //compteur_ph=compteur_ph+4;
    ph_moins=0.8;
   }   
   if ((t.hour)<(heurefin) and val_ph>=7.5 and (t.min)==debut_ph  ){ 
    digitalWrite(5, HIGH); //relais ph en marche
    //Serial.println("pompe PH en marche"); 
    flag_ph=1;
     }
   if ((t.min)==debut_ph+tempo_ph and flag_ph==1 ){
     digitalWrite(5, LOW); //relais ph: arret pompeph 
     //Serial.println("pompe PH à l'arret"); 
     bidon_ph = bidon_ph-ph_moins;
     compteur_ph=compteur_ph+tempo_ph;
     flag_ph=0;
     
     EEPROM.put(0,bidon_ph);
     delay(1);
     EEPROM.put(4,compteur_ph);
     delay(1);
     EEPROM.get(4,compteur_ph);
     EEPROM.get(0,bidon_ph);
      }
}

void bip(){
  digitalWrite(A2, HIGH);
  delay(50);
  digitalWrite(A2,LOW);
 // Serial.println("BEEP"); 
} 

void bip_bip(){
  digitalWrite(A2,HIGH);
  delay(10);
  digitalWrite (A2,LOW);
  delay(200);
  digitalWrite(A2,HIGH);
  delay(10);
  digitalWrite (A2,LOW); 
  delay(200);
}
//***********************************************************web*************************************************
  void web(){
  chien_de_garde();
//Serial.println("Reset web"); 
  bouton_etat_hiv=digitalRead(bouton_pin_hiv);
  
  //delay(10);  
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
   // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println(F("Content-Type: text/html;"));
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          client.println(F("Refresh: 5"));  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println(F("<html>"));
          client.println(F("<body style=background-color:#33a6ff;font-size:300%>"));
//*******************************************************Sortie web de la DHM *************************************************         
           chien_de_garde();
          if (bouton_etat_hiv==1){        
            client.print(F("Date :"));
            client.print(jour);
            client.print(t.mday);
            client.print(F("/"));
            client.print(mois);
            client.print(t.mon);
            client.print(F("   Heure : "));
            client.print(heure);
            client.print(t.hour);
            client.print(F(":"));
            client.print(minute);
            client.print(t.min);
            client.print(F("<br><br><br>"));
//****************************************************valeurs sur sorties analogique 0 et 1 du PH et du chlore                              
            client.print(F("Valeur PH : "));
            client.print(val_ph);
            client.print("<br />");
            client.print("Nb injection PH : ");
            client.print(compteur_ph);
            client.print("<br />");
            client.print (F("Niveau bidon PH : "));
            client.print(bidon_ph);
            client.print (F(" L "));
            client.print("<br><br>");  
            client.print(F("Valeur ORP : "));
            client.print(val_chl,0);
            client.print(F("<br />"));
            client.print(F("Nb injection CHL : "));
            client.print(compteur_chl);
            client.print(F("<br />"));
            client.print (F("Niveau bidon CHL : "));
            client.print(bidon_chl);
            client.print (F(" L "));
            client.print(F("<br><br>"));
            
    //**********************************************************Sortie sur internet de la temperature
    chien_de_garde();
    client.print (F("Temperature de l'eau : ")); 
    client.print (temp_pisc,1); 
    client.write ("&#186");
    client.print (F("C"));  
    client.println(F("</html>"));
    client.print("<br />");
    client.print (F("Temperature de l'air ext : "));//******************temperature exterieure***********************************
    client.print (temp_ext,1);//(temp_ext);
    client.write("&#186");
    client.print (F("C"));  
    client.println("</html>");
    client.print("<br />");
    if (heurefin> t.hour){
    client.print (F("la pompe fonctionne de "));
    client.print (heure_departpompe);
    client.print (F("h "));
    client.print (F("&agrave"));
    client.print (heurefin);
    client.print (F("h"));}
    client.print(F("<br />"));
    client.print(F("Pression : "));
    client.print(pression,1);
    client.print(F("bar"));
    client.print(F("<br />"));
    if (etat_pompe==0){
    client.print (F("Pompe "));client.print (F("&agrave")); client.print (F(" l'arret"));}
    
    }
       bouton_etat_hiv=digitalRead(bouton_pin_hiv);
       if (bouton_etat_hiv==0){
        client.println(F("MODE HIVERNAGE EN COURS"));
        client.print(F("<br />"));
       
switch (temp_hiver) {
  case 1:
    client.print(F(" 00H,06H,12H,18H,22H "));
    client.print(F("<br />"));
    if (etat_pompe==1){ 
    client.print (t.hour);client.print (F("H"));}
    break;
  case 2:
    client.print(F("   00H,06H,14H,20H   "));
    client.print(F("<br />"));
    if (etat_pompe==1){ 
    client.print (t.hour);client.print (F("H"));}
    break;
  case 3:
    client.print(F("  00H,10H,18H       "));
    client.print("<br />");
    if (etat_pompe==1){ 
    client.print (t.hour);client.print (F("H"));}
    break;
  case 4:
    client.print(F("00H,02H,06H,12H,18H,20H"));
    client.print(F("<br />"));
    if (etat_pompe==1){ 
    client.print (t.hour);client.print (F("H"));}
    break;
  case 5:
    client.print (F("24H/24H"));
    client.print(F("<br />"));
    if (etat_pompe==1){ 
    client.print (t.hour);client.print (F("H"));}
    break; }
    client.print(F("<br />"));
    client.print (F("Temperarure ext : ")); 
    //client.write ("&#186");
    client.print (temp_ext);//(temp_ext);
    client.write("&#186");
    client.print (F("C"));  
     if (etat_pompe==1){
       client.print(F("<br />"));
       client.print (F("Pompe en marche"));}
       client.print(F("<br />"));
       if (etat_pompe==0){
       client.print (F("Pompe "));client.print (F("&agrave")); client.print (F(" l'arret"));}
       }
      chien_de_garde();
          break;
        }
        if (c = '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
        }
       }
    
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    //Serial.println("client disconnected");
  }
}

 void chien_de_garde(){
  digitalWrite(4,HIGH );
  delay(10);
  digitalWrite(4, LOW);
 }
