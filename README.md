# regul-piscine
Régulation PH &amp; ORP pour piscine

Le but est de réaliser une régulation PH et ORP, d’automatiser le temps de filtration en fonction de la température de l’eau et du volume d’eau, de contrôler la pression dans le circuit hydraulique  et d’ajouter un hivernage actif  en fonction de la température extérieure. Le tout affiché sur un LCD de 4 lignes de 20 caractères dans le local technique, et également visible sur internet.
 le matériel utilisé :
-	Un ARDUINO UNO
-	Un module Ethernet W5100
-	Un module RTC DS3231
-	Deux sondes de température 18B20 (piscine & extérieure)
-	Un capteur de pression 0-5 bars
-	Un afficheur LCD 4X20
-	Un module de 3 relais
-	Un circuit imprimé double faces d’interconnexion de l’ensemble avec  
o	Deux régulateurs 5V/1A 7805
o	Résistances et condensateurs.
-	Deux circuit imprimés double faces  amplis des sondes PH et ORP avec pour chacun :
o	3 CI AOP
o	Un isolement galvanique alimentation + & - 5 volts 
o	Un isolement opto pour le signal.  
o	Résistances et condensateurs.
-	Fil de câblage, boitier, connecteurs, boutons poussoir, interrupteur… et EasyEDA pour la réalisation des schémas et PCB.
