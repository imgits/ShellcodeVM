#pragma once
/* MSRs */
#define IA32_FEATURE_CONTROL_CODE				0x03A
#define IA32_SYSENTER_CS                        0x174
#define IA32_SYSENTER_ESP                       0x175
#define IA32_SYSENTER_EIP                       0x176
#define IA32_DEBUGCTL                           0x1D9
#define IA32_VMX_BASIC_MSR_CODE					0x480
#define IA32_VMX_PINBASED_CTLS                  0x481
#define IA32_VMX_PROCBASED_CTLS                 0x482
#define IA32_VMX_EXIT_CTLS                      0x483
#define IA32_VMX_ENTRY_CTLS                     0x484
#define IA32_VMX_MISC                           0x485
#define IA32_VMX_CR0_FIXED0                     0x486
#define IA32_VMX_CR0_FIXED1                     0x487
#define IA32_VMX_CR4_FIXED0                     0x488
#define IA32_VMX_CR4_FIXED1                     0x489
#define	IA32_FS_BASE    						0xc0000100
#define	IA32_GS_BASE							0xc0000101
#define IA64_SYSENTER_EIP                       0xC0000082

#define MSR_IA32_MTRRCAP				0xfe
#define MSR_IA32_MTRR_DEF_TYPE			0x2ff
#define MSR_IA32_MTRR_PHYSBASE(n)		(0x200 + 2*(n))
#define MSR_IA32_MTRR_PHYSMASK(n)		(0x200 + 2*(n) + 1)
#define MSR_IA32_MTRR_FIX64K_00000		0x250
#define MSR_IA32_MTRR_FIX16K_80000		0x258
#define MSR_IA32_MTRR_FIX16K_A0000		0x259
#define MSR_IA32_MTRR_FIX4K_C0000		0x268
#define MSR_IA32_MTRR_FIX4K_C8000		0x269
#define MSR_IA32_MTRR_FIX4K_D0000		0x26a
#define MSR_IA32_MTRR_FIX4K_D8000		0x26b
#define MSR_IA32_MTRR_FIX4K_E0000		0x26c
#define MSR_IA32_MTRR_FIX4K_E8000		0x26d
#define MSR_IA32_MTRR_FIX4K_F0000		0x26e
#define MSR_IA32_MTRR_FIX4K_F8000		0x26f




#define MSR_LASTBRANCH_0_FROM_IP 0x40
#define MSR_LASTBRANCH_0_TO_IP 0x60
#define MSR_LASTBRANCH_TOS 0x1C9