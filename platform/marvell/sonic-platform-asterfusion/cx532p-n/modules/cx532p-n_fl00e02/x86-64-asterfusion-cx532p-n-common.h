#include <linux/i2c.h>
#include <linux/i2c-dev.h>
int get_bmc_data(u8 command, u8 fst_command, u8 sec_command, u8 vnum_command, union i2c_smbus_data *cx532p_bmc_read_data);
