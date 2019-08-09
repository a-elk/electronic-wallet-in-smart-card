
#include <io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
//------------------------------------------------
// Programme "hello world" pour carte à puce
// 
//------------------------------------------------


// déclaration des fonctions d'entrée/sortie définies dans "io.c"
void sendbytet0(uint8_t b);
uint8_t recbytet0(void);
void engage(uint8_t,...);
void valide();
void lis ();
void dechiffrer (uint32_t*, uint32_t*);
void chiffrer (uint32_t*, uint32_t*);

// variables globales en static ram
uint8_t cla, ins, p1, p2, p3;	// entête de commande
uint8_t sw1, sw2;		// status word
uint8_t a = 0x00;

int taille;		// taille des données introduites -- est initialisé à 0 avant la boucle
#define MAXI 32	// taille maxi des données lues
#define MAX_NAME 10
#define EEMEM  __attribute__((section(".eeprom")))
#define SIZEBUFF 30
#define SIZEKEY  16

uint8_t data[MAXI];	// données introduites
uint8_t nom[MAX_NAME] EEMEM;
uint8_t v[SIZEKEY];
uint32_t key_ram[4] = {0};
uint32_t adechiffre[2] = {0};
uint32_t k;
uint32_t k_bigend;
uint8_t key_readed[SIZEKEY] = {0};
uint16_t solde EEMEM;
uint8_t addr EEMEM;
uint16_t compteur EEMEM;
uint8_t sizename EEMEM;
uint16_t credit = 0;
uint16_t compteurram = 0; 
uint32_t figure = 0;
typedef enum {DATA=0x2a,VIDE=0} state_type;
typedef struct tmp_eeprom{
  uint8_t buf[SIZEBUFF];
  uint8_t nb_champs;
}tmp_eeprom;

tmp_eeprom tmp EEMEM;

state_type state EEMEM;

uint32_t key[4] EEMEM;

// Procédure qui renvoie l'ATR
void atr(uint8_t n, char* hist)
{
  sendbytet0(0x3b);	// définition du protocole
  sendbytet0(n);		// nombre d'octets d'historique
  while(n--)		// Boucle d'envoi des octets d'historique
    {
      sendbytet0(*hist++);
    }
}

void debiter () {
  if (p3 != 1) {
      sw1=0x6c;	// taille incorrecte
      sw2=1;		// taille attendue
      return;
  }
  sendbytet0(ins);	// acquittement
  uint8_t debit = recbytet0 ();
  uint16_t montant  = eeprom_read_word (&solde);
  if (montant < 0)
    return;
  montant -= debit;
  engage (2,&montant,&solde,0);
  valide ();
}
// émission de la version
// t est la taille de la chaîne sv
void version(int t, char* sv)
{
  int i;
  // vérification de la taille
  if (p3!=t)
    {
      sw1=0x6c;	// taille incorrecte
      sw2=t;		// taille attendue
      return;
    }
  sendbytet0(ins);	// acquittement
  // émission des données
  for(i=0;i<p3;i++)
    {
      sendbytet0(sv[i]);
    }
  sw1=0x90;
}

void chiffrer (uint32_t* v, uint32_t* k) {
  uint32_t v0=v[0], v1=v[1], sum=0, i;           /* initialisation */
  uint32_t delta=0x9e3779b9;                     /* constantes de clef */
  uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* mise en cache de la clef */
  for (i=0; i < 32; i++) {                       /* boucle principale */
    sum += delta;
    v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
    v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
  }
  v[0]=v0; v[1]=v1;
}

void dechiffrer (uint32_t* v, uint32_t* k) {
  uint32_t v0=v[0], v1=v[1], sum=0xC6EF3720,i;  /* initialisation */
  uint32_t delta=0x9e3779b9;                     /* constantes de clefs */
  uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* mise en cache de la clef */
  for (i=0; i<32; i++) {
    v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
    v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
    sum -= delta;
  }
  v[0]=v0; v[1]=v1;
}

void intro_key () {
  if (p3>MAXI)
    {
      sw1=0x6c;	// P3 incorrect
      sw2=MAXI;	// sw2 contient l'information de la taille correcte
      return;
    }
  sendbytet0(ins);	// acquitement

  for (int i = 0; i < 16; ++i) 
    v[i] = recbytet0 ();
  
  for (int i = 0; i < 4; ++i) {
    for (int j =0; j < 4; ++j) {
      int l = j + (i * 4);
      key_ram[i] = key_ram[i] | v[l];
      if (j < 3)
	key_ram[i] = key_ram[i] << 8;
    }
  }
  engage (4,&key_ram[0],&key[0],4,&key_ram[1],&key[1],4,&key_ram[2],&key[2],4,&key_ram[3],&key[3],0);
  valide ();
  sw1=0x90;
}

void read_key () {
  if (p3>MAXI)
    {
      sw1=0x6c;
      sw2=MAXI;
      return;
    }
  sendbytet0(ins);
  
  for (int j = 0; j < 4; ++j) {
    k = eeprom_read_dword (&key[j]);
    k_bigend = ((k>>24)&0xff) | ((k<<8)&0xff0000) |
                    ((k>>8)&0xff00) |
                    ((k<<24)&0xff000000);
    for (int i = 0; i < 4; ++i) {
      int l = i + (j * 4);
      key_readed[l] = k_bigend & 0XFF;
      k_bigend = k_bigend >> 8;
    }
  }
  for (int i = 0; i < 16; ++i) {
    sendbytet0(key_readed[i]);
  }
  sw1=0x90;
  }

// commande de réception de données
void intro_data()
{
  int i;
  // vérification de la taille
  if (p3>MAXI)
    {
      sw1=0x6c;	// P3 incorrect
      sw2=MAXI;	// sw2 contient l'information de la taille correcte
      return;
    }
  sendbytet0(ins);	// acquitement

  for(i=0;i<p3;i++)	// boucle d'envoi du message
    {
      data[i]=recbytet0();
    }
  taille=p3; 		// mémorisation de la taille des données lues
  sw1=0x90;
}

// commande de réception de données
void send_data()
{
  int i;
  // vérification de la taille
  if (p3 != taille){
    sw1=0x6c;
    return;
  }
  sendbytet0(ins);	// acquitement

  for(i=0;i<p3;i++)	// boucle d'envoi du message
    {
      sendbytet0(data[i]);
    }
  taille=0; 		// mémorisation de la taille des données lues
  sw1=0x90;
}

void send_name_eeprom()
{
  int i;
  //p3= eeprom_read_byte(&sizename);
  uint8_t taille_attendue = eeprom_read_byte(&sizename);
  if (p3 != taille_attendue)
    {
      sw1=0x6c;	// P3 incorrect
      sw2=taille_attendue;	// sw2 contient l'information de la taille correcte
      return;
    }
  sendbytet0(ins);	// acquitement
  
  for(i=0;i< taille_attendue;i++)
    sendbytet0(eeprom_read_byte(&nom[i]));
  sw1=0x90;
}


void engage (uint8_t taille,...) {
  eeprom_write_byte((uint8_t*)&state,VIDE); // mets ETAT à VIDE 
  va_list args;
  va_start (args,taille);              
  uint8_t* src;                   //adresse source en ram
  uint8_t* dest;                  //adresse destination finale en eeprom
  uint8_t nb_champs = 0;
  uint8_t* ptr = tmp.buf;
  while (taille != 0) {
    src = va_arg(args,uint8_t*);
    dest = va_arg(args,uint8_t*);
    eeprom_write_byte (ptr,taille); //ecriture taille dans tampon
    ++ptr;
    eeprom_write_block(src,ptr,taille);   // ecriture bloque dans tampon
    ptr += taille;
    eeprom_write_word ((uint16_t*)ptr,(uint16_t)dest);      //ecriture adresse destination
    ptr += 2;
    ++nb_champs;
    taille = va_arg (args,int);
  }
  eeprom_write_byte(&tmp.nb_champs,nb_champs);
  va_end (args);
  eeprom_write_byte((uint8_t*)&state,DATA);
}

void valide () {
  if (eeprom_read_byte((uint8_t*)&state) == DATA) {
    uint8_t nb_champs = eeprom_read_byte(&tmp.nb_champs);
    uint8_t* ptr = tmp.buf;
    for (int i =0; i < nb_champs; ++i) {
      uint8_t taille = eeprom_read_byte(ptr);   //lecture taille
      ++ptr;
      uint8_t buf[SIZEBUFF];
      eeprom_read_block(buf,ptr,taille);       //lecture bloque 
      ptr += taille;
      uint8_t* addr;
      addr = (uint8_t*)eeprom_read_word((uint16_t*)ptr);          //lecture addresse
      ptr += 2;
      eeprom_write_block(buf,addr,taille);
    }
    eeprom_write_byte((uint8_t*)&state,VIDE);           //etat à vide
  }
}

void intro_name_eeprom()
{
  int i;
  if (p3>MAX_NAME)
    {
      sw1=0x6c;	// P3 incorrect
      sw2=MAXI;	// sw2 contient l'information de la taille correcte
      return;
    }
  sendbytet0(ins);	// acquitement
  for(i=0;i<p3;i++)	// boucle d'envoi du message
    data[i]=recbytet0();
  uint16_t s = 0;
  engage(p3,data,nom,1,&p3,&sizename,2,&s,&solde,0);
  valide();
  sw1=0x90;
}

void crediter(){
  if (p3 != 8)
    {
      sw1=0x6c;	// P3 incorrect
      sw2=MAXI;	// sw2 contient l'information de la taille correcte
      return;
    }
  sendbytet0(ins);	// acquitement
  for (int i = 0;i < 8; ++i){
    data[i] = recbytet0 ();
  }
  adechiffre[0] = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
  adechiffre[1] = ((uint32_t)data[4] << 24) | ((uint32_t)data[5] << 16) | ((uint32_t)data[6] << 8) | (uint32_t)data[7];
  for (int i = 0; i < 4; ++i) {
    k = eeprom_read_dword (&key[i]);
    key_ram[i] = k;
  }
  dechiffrer (adechiffre,key_ram);
  credit = adechiffre[0] >> 16;
  uint16_t fig1 = adechiffre[0] & 0xFFFF;
  uint16_t fig2 = adechiffre[1] >> 16;
  compteurram = adechiffre[1] & 0xFFFF;
  figure = ((uint32_t)fig1 << 16) | (uint32_t)fig2;
  if (figure != 0 || compteurram <= eeprom_read_word (&compteur))
    return;
  uint16_t nouveau_solde = credit + eeprom_read_word (&solde);
  if (nouveau_solde > 0XFFFF)
    return;
  if (compteurram == 0XFFFF) {
    uint16_t zero = 0;
    engage (2,&nouveau_solde,&solde,2,&zero,&compteur,0);
  }
  else
    engage (2,&nouveau_solde,&solde,2,&compteurram,&compteur,0);
  valide ();
  sw1=0x90;
}

void reinitialise_compteur () {
  if (p3 > 2)
    {
      sw1=0x6c;	// P3 incorrect
      sw2=MAXI;	// sw2 contient l'information de la taille correcte
      return;
    }
  sendbytet0(ins);	// acquitement
  uint8_t v = 0;
  engage (2,&v,&compteur,0);
  valide ();
  sw1=0x90;
}

void read_solde () {
  if (p3 > 2)
    {
      sw1=0x6c;	// P3 incorrect
      sw2=MAXI;	// sw2 contient l'information de la taille correcte
      return;
    }
  sendbytet0(ins);	// acquitement
  uint16_t tmp =  (eeprom_read_word (&solde));
  sendbytet0 ((uint8_t)(tmp >> 8));
  sendbytet0 ((uint8_t)(tmp & 0XFF));
  sw1 = 0x90;
}
// Programme principal
//--------------------
int main(void)
{
  // initialisation des ports
  ACSR=0x80;
  DDRB=0xff;
  DDRC=0xff;
  DDRD=0;
  PORTB=0xff;
  PORTC=0xff;
  PORTD=0xff;
  ASSR=1<<EXCLK;
  TCCR2A=0;
  ASSR|=1<<AS2;


  // ATR
  atr(11,"Hello scard");
  
  taille=0;
  sw2=0;		// pour éviter de le répéter dans toutes les commandes
  // boucle de traitement des commandes
  valide();
  for(;;)
    {
      // lecture de l'entête
      cla=recbytet0();
      ins=recbytet0();
      p1=recbytet0();
      p2=recbytet0();
      p3=recbytet0();
      sw2=0;
      switch (cla)
	{
	case 0x80:
	  switch(ins)
	    {
	    case 0:
	      version(4,"1.00");
	      break;
	    case 1:
	      intro_data();
	      break;
	    case 2:
	      send_data();
	      break;
	    case 3:
	      intro_name_eeprom();
	      break;
	    case 4:
	      send_name_eeprom();
	      break;
	    case 5:
	      crediter();
	      break;
	    case 6:
	      debiter();
	      break;
	    case 7:
	      intro_key();
	      break;
	    case 8:
	      read_key();
	      break;;
	    case 9:
	      read_solde();
	      break;
	    case 10:
	      reinitialise_compteur();
	      break;
	    default:
	      sw1=0x6d; // code erreur ins inconnu
	    }
	  break;
	default:
	  sw1=0x6e; // code erreur classe inconnue
	}
      sendbytet0(sw1); // envoi du status word
      sendbytet0(sw2);
    }
  return 0;
}
