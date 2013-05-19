#include <stdint.h>
#include <stdio.h>

enum keys
 {
      key_alpha = 0,
      key_beta = 1,
      key_gamma = 2
 };

typedef struct {
  uint16_t code;
  char *str;
}ValType;

 ValType values[] = 
 {
   [ key_alpha ] = { 0x03b1,"alpha" },
   [ key_gamma ] = { 0x03b3,"gamma" },
   [ key_beta ]  = { 0x03b2,"beta" },
 };

const int num_keys = key_gamma - key_alpha + 1;

void main(){
  int i;
  for ( i = 0 ; i < num_keys ; i++) {
    printf("key %d val %s", i,values[i].str);
  }

}
