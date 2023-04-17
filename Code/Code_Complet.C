#include <SPI.h> //communication SPI
#include <TFT.h> //Utilisation de l'écran
#include "RTClib.h"
#include "SD.h"
#include <EEPROM.h>


//************************Definition des pins***************************************
#define cs   10 //D10
#define dc   9 //D9
#define rst  8 //D8
#define SD_CS 7 //D7
#define LED 6 //D6, luminosité écran


#define capteur 2 // D2, capteur vitesse
#define BOUTON A0
#define BOUTONREC A2


/************************Definition des variables**********************************
/*************************************Variable Temps**************************/
int min = 1;
int sec = 1;
int min_prec = 0;
int sec_prec = 0;
unsigned long Temps=0;
unsigned long Tdepart = 0;
unsigned long Tparcouru =0; //temps
unsigned long t_afficheTemps=millis();


/*************************************Variable IHM**************************/
bool statut_compteur;
bool statut_rec;
unsigned long t_action1 = 0;
unsigned long t_action2 = 0;
unsigned long t_reaction = 0;
char TEXTE[3]; //variable pour afficher le texte
char TEXTEs[5];//variables pour le temps
char TEXTEm[5];
char TEXTEd[5];
int lum = 255;//Definition luminosité


/*************************************Variable vitesse**************************/
//variable à remplacer par les mesures
int inj = 2000;
int conso = 5000;
float Vit=0; // Vitesse
float Vit_prec=0;


int etatPresent=0, etatPrecedent=0;
int Nv = 0; //nombre de changement d'état entre 2 recup de vitesse
float B = 16; // Nb bande
unsigned long tPresent = 0;
unsigned long Ttour = 0;
unsigned long tPrecedent=0; //temps
int TSeuil = 500; // rafraichissement du temps calcul Vit en ms
float R=0.055;//R=0.2364; //rayon roue


/*************************************Variable distance**************************/
unsigned long T_dist1 = millis();
unsigned long T_dist2 = 0; //temps
float distance_parcourue=0;
float distance_precedente=10000;


/*************************************Variable Vitesse moyenne**************************/
float VitMoy=0;
float VitMoy_prec=0;
unsigned long Tactuel = millis();


/*************************************Variable Carte SD***************************/
int nb_courses_mem;
int nb_courses;
String nomFichier;
//char nomFichier[8]="MES";
File monFichier;

//Création instance écran
TFT screen = TFT(cs, dc, rst);
//Création horloge
RTC_DS1307 rtc;


void setup() {
  //Initialisation de l'écran et affichage écran noir
  screen.begin();
  screen.background(0, 0, 0);
 
  //declaration capteurs
  pinMode(capteur,INPUT);
  pinMode(BOUTON,INPUT_PULLUP);//Bouton interruption
  pinMode(BOUTONREC,INPUT_PULLUP);


  //Resistance de pullup pour les boutons
  digitalWrite(BOUTON,HIGH);
  digitalWrite(BOUTONREC,HIGH);


  //Allumage de l'écran
  analogWrite(LED, lum);


  //Gestion du temps
  rtc.begin();
  DateTime now = rtc.now();
  DateTime dt = DateTime(__DATE__, __TIME__);
  delay(3000);
  Serial.begin(9600);
 
  //Gestion des interruptions
  //attachInterrupt(digitalPinToInterrupt(BOUTON),changement_ecran,CHANGE);
  PCICR |= B00000010; // We activate the interrupts of the PC port
  PCMSK1 |= B00000101; // We activate the interrupts on pin A0/A2  Serial.begin(9600);
 
  statut_compteur = true;
  statut_rec = false;
 
  //Initialisation de la carte SD
  SD.begin(SD_CS);
  //Initialisation EEPROM
  //EEPROM.put(0,init_eeprom);//A lancer une fois seulement à la toute première utilisation du compteur
  //Initialisation du compteur et du BLE
  init_IHM();

  //initialisation temps
  tPresent=millis();
}




void loop()
{
  while (statut_compteur==true){
    
    
    
    if (statut_rec==true){
      if (Vit>1){
        Tdepart=millis();
        T_dist1=millis();
        while(statut_rec==true){
          if (millis()%500==0){
            affiche_vit();
            calcul_distance();
            Calcul_VitesseMoy();
            temps();
            affiche_VMoy();
            affiche_dis();
            affiche_temps();
            //Serial.println(Vit);
            //Serial.print(",");
            //Serial.println(VitMoy);
            //Serial.print(",");
            //Serial.println(distance_parcourue);
            Ttour = millis();
            Nv=0;
            Vit_prec=Vit;
            enregistrer();
          }
          else
          {
          calcul_vitesse();
          }
        }
      }
      else if (millis()%500==0){
            affiche_vit();
            Ttour = millis();
            Nv=0;
            Vit_prec=Vit;
            }
      else
          {
          calcul_vitesse();
          }
    }
    
    else if (millis()%500==0){
            affiche_vit();
            VitMoy=0;
            distance_parcourue=0;
            Temps=0;
            affiche_VMoy();
            affiche_dis();
            affiche_temps();
            Ttour = millis();
            Nv=0;
            Vit_prec=Vit;
            }
    else{
      calcul_vitesse();  
    }
  }

}



/**********************************************Fonctions IHM**********************************************************************************************/
 void init_IHM(){ //Permet d'initialiser le compteur en affichant tout
   screen.stroke(255,255,255);//Texte en blanc
   param_unit();    
   affiche_conso();
   affiche_vit();
   affiche_VMoy();
   affiche_temps();
   affiche_dis();
   logo_bluetooth();
}
void affiche_conso(){
  screen.setTextSize(2);
  screen.stroke(255,255,255);//Texte en blanc
   //Effacement conso
  screen.fillRect(5, 3, 46, 20, ST7735_BLACK);//efface tout
  //Ecriture conso
  String(conso).toCharArray(TEXTE,5);    
  if (conso > 999){
  screen.text(TEXTE, 4, 5);
  } else if (conso >99){
  screen.text(TEXTE, 16, 5);
  } else if (conso >9){
  screen.text(TEXTE, 28, 5);
  } else {
  screen.text(TEXTE, 40, 5);  
  }
  //Effacement injections
  screen.fillRect(110, 5, 50, 15, ST7735_BLACK);
  //Ecriture injections
  String(inj).toCharArray(TEXTE,5);    
  if (inj > 999){
  screen.text(TEXTE, 110, 5);
  } else if (inj >99){
  screen.text(TEXTE, 122, 5);
  } else if (inj >9){
  screen.text(TEXTE, 136, 5);
  } else {
  screen.text(TEXTE, 148, 5);    
  }
}


void affiche_vit(){//Affichage de la vitesse
   screen.setTextSize(5);
   screen.stroke(0,255,255);//Texte en jaune
   //Effacement de la vitesse
   if (int(Vit/10)!=int(Vit_prec/10)){ //Si le premier chiffre change
  screen.fillRect(10, 50, 65, 40, ST7735_BLACK);  
   }
   if (int(Vit)!=int(Vit_prec)){ //Si le second chiffre de la vitesse change, on efface seulement ce chiffre (permet d'éviter l'effet clignotement de l'image)  
  screen.fillRect(40, 50, 25, 40, ST7735_BLACK);
   }
   if (int(Vit*10)!=int(Vit_prec*10)){  //A modifier : cas décimal
   screen.fillRect(100, 40, 25, 50, ST7735_BLACK);
   }
   if(Vit<10){
   String(Vit).toCharArray(TEXTE,4);
  screen.text(TEXTE, 40, 50);
   }
   else{
  String(Vit).toCharArray(TEXTE,5);
  screen.text(TEXTE, 10, 50);
   }
   screen.setTextSize(1);
   screen.text("km/h", 130, 70 );
}


void affiche_VMoy(){
  screen.setTextSize(2);


   //Effacement de la vitesse moyenne
   if (int(VitMoy/10)!=int(VitMoy_prec/10)){ //Si le premier chiffre change
  screen.fillRect(48, 110, 12, 20, ST7735_BLACK);
   }
   if (int(VitMoy)!=int(VitMoy_prec)){ //Si le second chiffre de la vitesse change, on efface seulement ce chiffre (permet d'éviter l'effet clignotement de l'image)  
  screen.fillRect(58, 110, 12, 20, ST7735_BLACK);  
   }
   if (int(VitMoy*10)!=int(VitMoy_prec*10)){  //A modifier : cas décimal
  screen.fillRect(78, 110, 18, 20, ST7735_BLACK);
   }
   //Ecriture de la vitesse moyenne
   if(VitMoy<10){
  screen.stroke(0,0,255);//rouge
  String(VitMoy).toCharArray(TEXTE,4);
  screen.text(TEXTE, 60, 110);
   }  
   else if(VitMoy<20 or VitMoy >30){
  screen.stroke(0,0,255);//Rouge
  String(VitMoy).toCharArray(TEXTE,5);
  screen.text(TEXTE, 48, 110);
   }
   else{
  screen.stroke(0,128,0);//vert
  String(VitMoy).toCharArray(TEXTE,5);
  screen.text(TEXTE, 48, 110);
   }
   screen.setTextSize(1);
   screen.text("km/h", 100, 115);
   VitMoy_prec=VitMoy;
}




void param_unit(){//Affichage des unités
  //Affichage des unités
  screen.setTextSize(1);
  screen.stroke(255,255,255);//blanc
  screen.text("km/l",5,25);
  screen.text("inj", 140, 25);
  screen.text("km", 144, 115); //distance parcourue
}




void affiche_temps(){
  screen.setTextSize(1);
  screen.stroke(255,255,255);
  screen.text(":",17,115); //les 2 points de l'heure
 
  min_prec=min;
  sec_prec=sec;
  min=Temps/60;
  sec=Temps%60;
 
  if(min_prec!=min or sec_prec!=sec){
  //effacement à implanter
  //Gestion minutes
  if (int(min/10)!=int(min_prec/10)){
  screen.fillRect(5, 115, 12, 10, ST7735_BLACK);//Le tout
  }
  if(min<10){
  screen.fillRect(11,115,6,10,ST7735_BLACK); //Unité
  String(min).toCharArray(TEXTEm,5);
  screen.text("0",5,115);
  screen.text(TEXTEm,11,115);
  }
  else{
  screen.fillRect(11,115,6,10,ST7735_BLACK); //Unité
  String(min).toCharArray(TEXTEm,5);
  screen.text(TEXTEm,5,115);
  }
   
  //Gestion secondes
  if (int(sec/10)!=int(sec_prec/10)){
  screen.fillRect(23,115,12,10,ST7735_BLACK); //Le tout
  }
  if(sec<10){
  screen.fillRect(29,115,6,10,ST7735_BLACK); //Unité
  String(sec).toCharArray(TEXTEs,5);
  screen.text("0",23,115);
  screen.text(TEXTEs,29,115);
  }
  else{
  screen.fillRect(29,115,6,10,ST7735_BLACK); //Unité
  String(sec).toCharArray(TEXTEs,5);
  screen.text(TEXTEs,23,115);
  }
  }
}




void temps(){
   if(Tdepart==0){
  Tdepart=millis();
  }
  Temps=(millis() - Tdepart)*0.001;
}


void affiche_dis(){
   if(int(distance_parcourue*10)!=int(distance_precedente*10)){
  screen.setTextSize(1);
  screen.stroke(255,255,255);
  screen.fillRect(133,105,43,10,ST7735_BLACK);
  screen.fillRect(139,105,7,10,ST7735_BLACK);
  screen.fillRect(151,105,7,10,ST7735_BLACK);  
  String(distance_parcourue).toCharArray(TEXTEd,4);
  screen.text(TEXTEd,133,105);
  distance_precedente=distance_parcourue;
  }
}


void logo_bluetooth(){//Affichage du logo bluetooth
  screen.stroke(ST7735_RED);
  screen.line(80, 5, 85, 8);
  screen.line(80, 11, 85, 8);
  screen.line(80, 5, 80, 11);
 
  screen.line(80,11,77,9);
  screen.line(80,11,77,13);
 
  screen.line(80,11,85,14);
  screen.line(80,17,80,11);
  screen.line(80,17,85,14);
}
/**********************************************Fonctions SD*********************************************************************************************************/
void init_SD(){//Initialise le fichier dans lequel sont stockées les informations
  filename();//Initialisation du nom du fichier
  monFichier = SD.open(nomFichier,FILE_WRITE);
  if(monFichier){
  monFichier.print("Temps");
  monFichier.print(";");
  monFichier.print("Vitesse");
  monFichier.print(";");
  monFichier.print("Vitesse moyenne");
  monFichier.print(";");
  monFichier.print("Distance");
  monFichier.print(";");
  monFichier.println("Consommation");
  monFichier.close();
  } else {
  Serial.println("Erreur !");
  }
}
void enregistrer(){//Enregistre les données dans le fichier initialisé précedemment
  monFichier = SD.open(nomFichier,FILE_WRITE);
  monFichier.print(Temps);
  monFichier.print(";");
  monFichier.print(Vit);
  monFichier.print(";");
  monFichier.println(VitMoy);
  monFichier.print(";");
  monFichier.print(distance_parcourue);
  monFichier.print(";");
  monFichier.println(conso);
  monFichier.close();
}


void filename(){//Fonction permettant de générer le nom du fichier en fonction de ce qui existe déja sur la carte
  // On vérifie que la carte SD est prête
  
  // On ouvre le fichier pour écrire des données
  String filename = "data";
  int count = 0;
  
  // On cherche le numéro le plus élevé utilisé pour un fichier CSV
  while (SD.exists(filename + String(count) + ".csv")) {
    count++;
  }
  
  // On ajoute le numéro au nom du fichier
  filename += String(count) + ".csv";

  // On stocke le nom du fichier créé dans la variable nomFichier
  nomFichier = filename;
  
  // On affiche le nom du fichier créé
  Serial.print("Fichier CSV créé: ");
  Serial.println(filename);
}
/**********************************************Fonctions Interruptions**********************************************************************************************/
//PCINT1_vect
ISR (PCINT1_vect){//Mecanisme d'interruption
  t_reaction = millis();
  if(t_reaction - t_action1>200){
    if (digitalRead(BOUTON)==LOW){
      if (statut_compteur==true){
        analogWrite(LED, 0);//Extinction de l'écran
        statut_rec==false;
      }else{
        analogWrite(LED, lum);//Allumage de l'écran
      }
      statut_compteur = !statut_compteur;
      t_action1 = millis();
    }
  }
  if(t_reaction - t_action2>200){
    if(digitalRead(BOUTONREC)==LOW){
      if(statut_compteur==true){
        if(statut_rec==true){
          screen.fillCircle(90, 13, 3, ST7735_BLACK);
        }else{
          screen.fillCircle(90, 13, 3, ST7735_BLUE);
          init_SD();
        }
        statut_rec=!statut_rec;
        t_action2 = millis();
      }
    }   
  }
}


/*************************Calcul Vitesse - Distance - Vitesse moyenne************************/
void calcul_vitesse(){
  etatPresent = digitalRead(capteur);
  if (etatPresent!=etatPrecedent){ //Vérification d’un changement de bande
  Nv++;  //Nombre de bande passées
  etatPrecedent=etatPresent;
  if(Vit>12 and Nv==2*B)  //Vérification des 3 tours de roue
  {
  tPresent = millis();
  Vit=(2*2*PI*R*3.6)/((tPresent-Ttour)*0.001);//Calcul de la vitesse
  Ttour=millis();
  Nv=0; //Remise à 0 du nombre de bande passées


  }
  else if(Vit>6 and Nv==0.5*B){
  tPresent = millis();
  Vit=(0.5*2*PI*R*3.6)/((tPresent-Ttour)*0.001);//Calcul de la vitesse
  Ttour=millis();
  Nv=0; //Remise à 0 du nombre de bande passées
  }
  else if(Nv==0.25*B){
  tPresent = millis();
  Vit=(0.25*2*PI*R*3.6)/((tPresent-Ttour)*0.001);//Calcul de la vitesse
  Ttour=millis();
  Nv=0; //Remise à 0 du nombre de bande passées
  }
  }
  if(millis()-tPresent>=1000){ //Remise à 0 de la vitesse
  Vit=0;
  Nv=0;
  tPresent=millis();
  }
}

 
void calcul_distance(){
  T_dist2=millis();
  distance_parcourue+=Vit*(T_dist2-T_dist1)/3600000;  //Calcul de la distance
  T_dist1=millis();
  }


void Calcul_VitesseMoy(){
  Tactuel = millis();
  Tparcouru=(Tactuel-Tdepart); //Calcul du temps passé depuis le départ
  VitMoy=(distance_parcourue/Tparcouru)*3600000; //Calcul de la vitesse moyenne
}
