#include <inc/x86.h>
#include <inc/string.h>
#include <kern/pmap.h>
#include <kern/e1000.h>

// LAB 6: Your driver code here
volatile uint32_t * e1000;

struct tx_desc tx_d[TXRING_LEN] __attribute__((aligned (PGSIZE))) 
		= {{0, 0, 0, 0, 0, 0, 0}};
struct packet pbuf[TXRING_LEN] __attribute__((aligned (PGSIZE)))
		= {{{0}}};

struct rx_desc rx_d[RXRING_LEN] __attribute__((aligned (PGSIZE)))
		= {{0, 0, 0, 0, 0, 0}};
struct packet prbuf[RXRING_LEN] __attribute__((aligned (PGSIZE)))
		= {{{0}}};

int
pci_e1000_attach(struct pci_func * pcif) 
{
    pci_func_enable(pcif);

    e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("e1000: bar0  %x size0 %x\n", pcif->reg_base[0], pcif->reg_size[0]);

    e1000[TDBAL/4] = PADDR(tx_d);
    e1000[TDBAH/4] = 0;
    e1000[TDLEN/4] = TXRING_LEN * sizeof(struct tx_desc);
    e1000[TDH/4] = 0;
    e1000[TDT/4] = 0;
    e1000[TCTL/4] = TCTL_EN | TCTL_PSP | (TCTL_CT & (0x10 << 4)) | (TCTL_COLD & (0x40 << 12));
    e1000[TIPG/4] = 10 | (8 << 10) | (12 << 20);
    cprintf("e1000: status %x\n", e1000[STATUS/4]);

    return 0;
}

int
e1000_transmit(void *addr, size_t len)
{
	uint32_t tail = e1000[TDT/4];
	struct tx_desc *nxt = &tx_d[tail];
	
	if((nxt->status & TXD_STAT_DD) != TXD_STAT_DD)
		return -1;	
	if(len > TBUFFSIZE)
		len = TBUFFSIZE;

	memmove(&pbuf[tail], addr, len);
	nxt->length = (uint16_t)len;
	nxt->status &= !TXD_STAT_DD;
	e1000[TDT/4] = (tail + 1) % TXRING_LEN;
	return 0;
}
