
#include <i2c/i2c.h>

#define DIV_ROUND(n,d)         (((n) + ((d)/2)) / (d))

int ltc3676_pmic_setup(struct i2c_client *client);
int rn5t567_pmic_setup(struct i2c_client *client);
int rn5t618_pmic_setup(struct i2c_client *client);
