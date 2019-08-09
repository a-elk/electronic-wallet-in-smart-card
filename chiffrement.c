#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
  uint32_t v0=v[0], v1=v[1], sum=0xC6EF3720, i;  /* initialisation */
  uint32_t delta=0x9e3779b9;                     /* constantes de clefs */
  uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* mise en cache de la clef */
  for (i=0; i<32; i++) {                         /* boucle principale */
    v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
    v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
    sum -= delta;
  }
  v[0]=v0; v[1]=v1;
}


int main (int argc, char* argv[]) {
  uint16_t montant = atoi(argv[1]);
  uint32_t check = 3;
  uint16_t compteur = atoi(argv[2]);
  printf("montant : %u  compteur : %u\n",montant,compteur);
  uint8_t key[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  uint32_t key_32[4] = {0};
  for (int i = 0; i < 4; ++i) {
    for (int j =0; j < 4; ++j) {
      int l = j + (i * 4);
      key_32[i] = key_32[i] | key[l];
      if (j < 3)
	key_32[i] = key_32[i] << 8;
    }
  }
  for (int i = 0; i < 4; ++i) {
    printf ("key : %u  ",key_32[i]);
  }
  uint8_t* p = (uint8_t*)&key_32[0];
  for (int i = 0; i < 16; ++i) {
    printf ("p : %d  ",p[i]);
  }
  printf ("\n");
  uint16_t check1 = check >> 16;
  uint16_t check2 = check & 0Xffff;
  uint32_t val32[2] = {0};
  val32[0] = (val32[0] | montant) << 16;
  val32[0] = val32[0] | check1;
  val32[1] = (val32[1] | check2) << 16;
  val32[1] = val32[1] | compteur;
  printf("v[0] : %u v[1] : %u\n",val32[0],val32[1]);
  chiffrer (val32, key_32);
  printf("Apres chiffrement\n");
  printf("v[0] : %u v[1] : %u\n",val32[0],val32[1]);
  uint8_t commande[8] = {0};
  uint64_t tmp = 0;
  tmp = val32[0];
  tmp = tmp << 32;
  tmp = tmp | val32[1];
  commande[0] = (tmp & 0xFF00000000000000) >> 56;
  commande[1] = (tmp & 0xFF000000000000) >> 48;
  commande[2] = (tmp & 0xFF0000000000) >> 40;
  commande[3] = (tmp & 0xFF00000000) >> 32;
  commande[4] = (tmp & 0xFF000000) >> 24;
  commande[5] = (tmp & 0xFF0000) >> 16 ;
  commande[6] = (tmp & 0xFF00) >> 8;
  commande[7] = tmp & 0xFF;
  printf("commande chiffrée en hexa : %x %x %x %x %x %x %x %x\n",commande[0],commande[1],commande[2],commande[3],commande[4],commande[5],commande[6],commande[7]);
  dechiffrer (val32, key_32);
  //printf("Apres dechiffrement\n");
  //printf("v[0] : %u v[1] : %u\n",val32[0],val32[1]);
  uint8_t commande2[8] = {0};
  uint64_t tmp2 = 0;
  tmp2 = val32[0];
  tmp2 = tmp2 << 32;
  tmp2 = tmp2 | val32[1];
  commande2[0] = (tmp2 & 0xFF00000000000000) >> 56;
  commande2[1] = (tmp2 & 0xFF000000000000) >> 48;
  commande2[2] = (tmp2 & 0xFF0000000000) >> 40;
  commande2[3] = (tmp2 & 0xFF00000000) >> 32;
  commande2[4] = (tmp2 & 0xFF000000) >> 24;
  commande2[5] = (tmp2 & 0xFF0000) >> 16 ;
  commande2[6] = (tmp2 & 0xFF00) >> 8;
  commande2[7] = tmp2 & 0xFF;
  //  printf("commande2 dechiffrée en hexa : %x %x %x %x %x %x %x %x\n",commande2[0],commande2[1],commande2[2],commande2[3],commande2[4],commande2[5],commande2[6],commande2[7]);
  return 0;
}
