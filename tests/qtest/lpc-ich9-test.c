/*
 * QTest testcases for ich9 case
 *
 * Copyright (c) 2020 Li Qiang <liq3ea@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"

#include "libqtest.h"

#define ICH9_LPC_CONFIG_ADDR  0x8000f800
#define ICH9_LPC_PMBASE       0x40
#define ICH9_LPC_ACPI_CTRL    0x44
#define ICH9_LPC_GEN_PMCON_1  0xa0
#define ICH9_LPC_GEN_PMCON_LOCK  0xa6

#define ICH9_LPC_GEN_PMCON_1_SMI_LOCK           (1 << 4)
#define ICH9_LPC_GEN_PMCON_LOCK_ACPI_BASE_LOCK  (1 << 0)
#define ICH9_LPC_PMBASE_BASE_ADDRESS_MASK       0xff80

static void ich9_lpc_config_writel(QTestState *s, uint8_t offset, uint32_t value)
{
    qtest_outl(s, 0xcf8, ICH9_LPC_CONFIG_ADDR | offset);
    qtest_outl(s, 0xcfc, value);
}

static uint32_t ich9_lpc_config_readl(QTestState *s, uint8_t offset)
{
    qtest_outl(s, 0xcf8, ICH9_LPC_CONFIG_ADDR | offset);
    return qtest_inl(s, 0xcfc);
}

static void ich9_lpc_config_writew(QTestState *s, uint8_t offset, uint16_t value)
{
    qtest_outl(s, 0xcf8, ICH9_LPC_CONFIG_ADDR | (offset & ~3));
    qtest_outw(s, 0xcfc + (offset & 3), value);
}

static uint16_t ich9_lpc_config_readw(QTestState *s, uint8_t offset)
{
    qtest_outl(s, 0xcf8, ICH9_LPC_CONFIG_ADDR | (offset & ~3));
    return qtest_inw(s, 0xcfc + (offset & 3));
}

static void ich9_lpc_config_writeb(QTestState *s, uint8_t offset, uint8_t value)
{
    qtest_outl(s, 0xcf8, ICH9_LPC_CONFIG_ADDR | (offset & ~3));
    qtest_outb(s, 0xcfc + (offset & 3), value);
}

static uint8_t ich9_lpc_config_readb(QTestState *s, uint8_t offset)
{
    qtest_outl(s, 0xcf8, ICH9_LPC_CONFIG_ADDR | (offset & ~3));
    return qtest_inb(s, 0xcfc + (offset & 3));
}

static void test_lp1878642_pci_bus_get_irq_level_assert(void)
{
    QTestState *s;

    s = qtest_init("-M q35 "
                   "-nographic -monitor none -serial none");

    qtest_outl(s, 0xcf8, 0x8000f840); /* PMBASE */
    qtest_outl(s, 0xcfc, 0x5d00);
    qtest_outl(s, 0xcf8, 0x8000f844); /* ACPI_CTRL */
    qtest_outl(s, 0xcfc, 0xeb);
    qtest_outw(s, 0x5d02, 0x205d);
    qtest_quit(s);
}

static void test_gen_pmcon_locks(void)
{
    QTestState *s;
    uint32_t pmbase;
    uint8_t acpi_ctrl;
    uint16_t gen_pmcon_1;
    uint16_t gen_pmcon_lock;

    s = qtest_init("-M q35 "
                   "-nographic -monitor none -serial none");

    ich9_lpc_config_writel(s, ICH9_LPC_PMBASE, 0x600);
    pmbase = ich9_lpc_config_readl(s, ICH9_LPC_PMBASE);
    g_assert_cmphex(pmbase & ICH9_LPC_PMBASE_BASE_ADDRESS_MASK, ==, 0x600);

    ich9_lpc_config_writeb(s, ICH9_LPC_ACPI_CTRL, 0x84);
    acpi_ctrl = ich9_lpc_config_readb(s, ICH9_LPC_ACPI_CTRL);
    g_assert_cmphex(acpi_ctrl, ==, 0x84);

    ich9_lpc_config_writew(s, ICH9_LPC_GEN_PMCON_LOCK,
                           ICH9_LPC_GEN_PMCON_LOCK_ACPI_BASE_LOCK);
    gen_pmcon_lock = ich9_lpc_config_readw(s, ICH9_LPC_GEN_PMCON_LOCK);
    g_assert_cmphex(gen_pmcon_lock, ==,
                    ICH9_LPC_GEN_PMCON_LOCK_ACPI_BASE_LOCK);

    ich9_lpc_config_writel(s, ICH9_LPC_PMBASE, 0x700);
    pmbase = ich9_lpc_config_readl(s, ICH9_LPC_PMBASE);
    g_assert_cmphex(pmbase & ICH9_LPC_PMBASE_BASE_ADDRESS_MASK, ==, 0x600);

    ich9_lpc_config_writeb(s, ICH9_LPC_ACPI_CTRL, 0x85);
    acpi_ctrl = ich9_lpc_config_readb(s, ICH9_LPC_ACPI_CTRL);
    g_assert_cmphex(acpi_ctrl, ==, 0x84);

    ich9_lpc_config_writew(s, ICH9_LPC_GEN_PMCON_LOCK, 0);
    gen_pmcon_lock = ich9_lpc_config_readw(s, ICH9_LPC_GEN_PMCON_LOCK);
    g_assert_cmphex(gen_pmcon_lock, ==,
                    ICH9_LPC_GEN_PMCON_LOCK_ACPI_BASE_LOCK);

    ich9_lpc_config_writew(s, ICH9_LPC_GEN_PMCON_1,
                           ICH9_LPC_GEN_PMCON_1_SMI_LOCK);
    gen_pmcon_1 = ich9_lpc_config_readw(s, ICH9_LPC_GEN_PMCON_1);
    g_assert_cmphex(gen_pmcon_1 & ICH9_LPC_GEN_PMCON_1_SMI_LOCK, ==,
                    ICH9_LPC_GEN_PMCON_1_SMI_LOCK);

    ich9_lpc_config_writew(s, ICH9_LPC_GEN_PMCON_1, 0);
    gen_pmcon_1 = ich9_lpc_config_readw(s, ICH9_LPC_GEN_PMCON_1);
    g_assert_cmphex(gen_pmcon_1 & ICH9_LPC_GEN_PMCON_1_SMI_LOCK, ==,
                    ICH9_LPC_GEN_PMCON_1_SMI_LOCK);

    qtest_quit(s);
}

int main(int argc, char **argv)
{
    const char *arch = qtest_get_arch();

    g_test_init(&argc, &argv, NULL);

    if (strcmp(arch, "i386") == 0 || strcmp(arch, "x86_64") == 0) {
        qtest_add_func("ich9/test_lp1878642_pci_bus_get_irq_level_assert",
                       test_lp1878642_pci_bus_get_irq_level_assert);
        qtest_add_func("ich9/test_gen_pmcon_locks", test_gen_pmcon_locks);
    }

    return g_test_run();
}
