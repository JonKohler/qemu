======================================
WSMT system resource protection on q35
======================================

The Windows SMM Security Mitigation Table (WSMT)
``SYSTEM_RESOURCE_PROTECTION`` flag is an attestation by firmware that
software cannot reconfigure system resources through non-architectural
mechanisms after ``ExitBootServices()``.  For QEMU q35, that claim must
be treated as a contract between QEMU and firmware, not as a property
that OVMF can infer from the machine type alone.

The flag covers more than SMM communication buffer validation.  At a
minimum, a claimable q35 configuration needs the following resources to
be fixed, locked, or explicitly out of scope:

* ACPI fixed hardware registers reported in FADT, including ``PM1a_EVT``,
  ``PM1a_CNT``, ``PM_TMR``, ``GPE0_BLK``, and the reset register.
* ICH9 LPC configuration that controls those fixed register locations,
  especially ``PMBASE`` and the ACPI control register.
* Interrupt-controller resources, including IOAPIC identity and MMIO
  placement.
* HPET resources reported through ACPI.
* PCI configuration and root-complex windows, including MCFG/ECAM and
  any chipset registers that can move root-complex register blocks.
* Optional IOMMU resources such as DMAR units when an emulated IOMMU is
  present.
* The Firmware ACPI Control Structure (FACS) location and ACPI table
  storage used by the guest.
* Guest RAM layout and decode priority, including behavior when PCI BARs
  or other programmable decodes overlap RAM.

Current status
==============

QEMU already keeps many q35 resources fixed by construction, but not all
of the WSMT requirements have a tested lockdown contract.  In
particular, the ICH9 LPC device exposes mutable ACPI PM configuration
state.  QEMU models ``GEN_PMCON_1.SMI_LOCK`` and now models an ACPI PM
base lock bit in ``GEN_PMCON_LOCK`` so firmware can freeze the PM I/O
base before handing control to the OS.

That is only one part of the WSMT requirement.  QEMU must not advertise a
guest-visible capability for ``SYSTEM_RESOURCE_PROTECTION`` until the
remaining items above have qtests or integration tests demonstrating
that they cannot be moved or reconfigured after firmware lockdown.

Proposed firmware contract
==========================

When the full audit is complete, QEMU should expose a small fw_cfg
capability that means all q35 resources covered by WSMT system resource
protection are immutable for the current machine configuration.  OVMF
should only set ``EFI_WSMT_PROTECTION_FLAGS_SYSTEM_RESOURCE_PROTECTION``
when all of the following are true:

* The QEMU capability is present.
* OVMF has programmed the q35/ICH9 lock registers it owns.
* The build is otherwise eligible to publish WSMT communication buffer
  protection flags.

Until that capability exists and is tested, OVMF must omit
``SYSTEM_RESOURCE_PROTECTION``.

Compatibility notes
===================

New lock state must be migrated as part of the existing PCI
configuration state, and post-load hooks must reapply write masks derived
from sticky lock bits.  Existing machine types that do not opt into a
future WSMT system-resource-protection capability should retain their
current behavior.
