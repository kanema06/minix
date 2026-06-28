
#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <minix/sysutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>

#define KBD_STAT_PORT   0x64  
#define KBD_DATA_PORT   0x60  
#define MOUSE_IRQ       12    

#define BUF_SIZE 256
static uint8_t mouse_buf[BUF_SIZE];
static int buf_head = 0;
static int buf_tail = 0;

static int irq_hook_id;

static void mouse_intr(void)
{
    uint32_t stat, data;

    sys_inb(KBD_STAT_PORT, &stat);
    
    if (stat & 0x01) { 
        sys_inb(KBD_DATA_PORT, &data);
        
        mouse_buf[buf_head] = (uint8_t)data;
        buf_head = (buf_head + 1) % BUF_SIZE;
        
    }
}


static int mouse_open(devminor_t minor, int access, endpoint_t user_endpt)
{
    return OK;
}

static int mouse_close(devminor_t minor)
{
    return OK;
}

static ssize_t mouse_read(devminor_t minor, u64_t position, endpoint_t endpt, 
                          cp_grant_id_t grant, size_t size, int flags, cdev_id_t id)
{
    int bytes_read = 0;
    int r;
    uint8_t temp_buf[3];

    if (buf_head == buf_tail) {
        return 0; /* No hay datos listos todavía */
    }

   
    while (bytes_read < 3 && bytes_read < size && buf_head != buf_tail) {
        temp_buf[bytes_read] = mouse_buf[buf_tail];
        buf_tail = (buf_tail + 1) % BUF_SIZE;
        bytes_read++;
    }

    r = sys_safecopyto(endpt, grant, 0, (vir_bytes)temp_buf, bytes_read);
    if (r != OK) {
        return r;
    }

    return bytes_read; 
}

static struct chardriver mouse_tab = {
    .cdr_open  = mouse_open,
    .cdr_close = mouse_close,
    .cdr_read  = mouse_read,
};

static int sef_cb_init_fresh(int type, sef_init_info_t *info)
{
    /* 1. Registrar la política de la interrupción en el kernel */
    irq_hook_id = MOUSE_IRQ;
    sys_irqsetpolicy(MOUSE_IRQ, 0, &irq_hook_id);
    sys_irqenable(&irq_hook_id);

    return OK;
}

static void sef_local_startup(void)
{
    /* Configurar los callbacks del System Event Framework (SEF) */
    sef_setcb_init_fresh(sef_cb_init_fresh);
    sef_setcb_init_lu(sef_cb_init_fresh); /* Live update (opcional) */
    sef_setcb_init_restart(sef_cb_init_fresh);

    /* Registrar la función que manejará las interrupciones de hardware */
    sef_setcb_interrupt(mouse_intr);

    /* Iniciar el framework */
    sef_startup();
}

int main(int argc, char **argv)
{
    /* Inicializar la estructura del driver en el sistema */
    sef_local_startup();

    /* Ceder el control al bucle principal de la librería chardriver.
     * Esta función se bloquea y despacha automáticamente los mensajes 
     * IPC que lleguen (read, open, close, interrupciones de hardware). */
    chardriver_task(&mouse_tab);

    return OK;
}