#ifndef __PIC_H__
#define __PIC_H__

#define PIC_MASTER_CMD_PORT  0x20
#define PIC_MASTER_DATA_PORT 0x21
#define PIC_SLAVE_CMD_PORT   0xa0
#define PIC_SLAVE_DATA_PORT  0xa1
#define PIC_ACK              0x20

void pic_init(void);
void pic_install_handler(void (*handler)(), int irq_num);
void pic_ack_interrupt(int irq_num);

#endif /* end of include guard: __PIC_H__ */
