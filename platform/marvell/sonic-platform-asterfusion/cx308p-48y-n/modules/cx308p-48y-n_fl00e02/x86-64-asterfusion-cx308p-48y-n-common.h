int read_8bit_temp(u8 sign,u8 value);
int uart_write_cmd(u8 cmd, u8 sub_cmd_1, u8 sub_cmd_2, u8 n_para);
int uart_read_cmd(u8 cmd, u8 sub_cmd_1, u8 sub_cmd_2, u8 n_para, u8 *r_data);
