#ifndef INTERRUPTS_H
#define INTERRUPTS_H

void gdt_init(void);
void idt_init(void);
void pic_init(void);
void apic_init(void);
void sti(void);

#endif
