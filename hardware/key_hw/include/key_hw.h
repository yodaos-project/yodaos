#ifdef _KEY_HW_H_
#define _KEY_HW_H_

#include <stdint.h>

/*Function: get_hardware_key
 *Description: get public key from hardware otp area
 *Parameter: key_buf, the pointer to store public key
 *Return value: public key length, negative value presents get hardware key error
 * */
int get_hardware_key(char *key_buf);

#endif /*_KEY_HW_H_*/
