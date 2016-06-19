#include "cpu.h"
#include "vga.h"
#include "stdio.h"
#include <string.h>

char *CPU::table_lookup_model()
{
	struct cpu_model_info
	{
		int vendor;
		int family;
		char *model_names[16];
	};

	static struct cpu_model_info cpu_models[] =
	{
		{ CPU_VENDOR_INTEL, 4,{ "486 DX-25/33", "486 DX-50", "486 SX", "486 DX/2", "486 SL", "486 SX/2", NULL, "486 DX/2-WB", "486 DX/4", "486 DX/4-WB", NULL, NULL, NULL, NULL, NULL, NULL } },
		{ CPU_VENDOR_INTEL, 5,{ "Pentium 60/66 A-step", "Pentium 60/66", "Pentium 75 - 200", "OverDrive PODP5V83", "Pentium MMX", NULL, NULL, "Mobile Pentium 75 - 200", "Mobile Pentium MMX", NULL, NULL, NULL, NULL, NULL, NULL, NULL } },
		{ CPU_VENDOR_INTEL, 6,{ "Pentium Pro A-step", "Pentium Pro", NULL, "Pentium II (Klamath)", NULL, "Pentium II (Deschutes)", "Mobile Pentium II", "Pentium III (Katmai)", "Pentium III (Coppermine)", NULL, "Pentium III (Cascades)", NULL, NULL, NULL, NULL } },
		{ CPU_VENDOR_AMD, 4,{ NULL, NULL, NULL, "486 DX/2", NULL, NULL, NULL, "486 DX/2-WB", "486 DX/4", "486 DX/4-WB", NULL, NULL, NULL, NULL, "Am5x86-WT", "Am5x86-WB" } },
		{ CPU_VENDOR_AMD, 5,{ "K5/SSA5", "K5", "K5", "K5", NULL, NULL, "K6", "K6", "K6-2", "K6-3", NULL, NULL, NULL, NULL, NULL, NULL } },
		{ CPU_VENDOR_AMD, 6,{ "Athlon", "Athlon", "Athlon", NULL, "Athlon", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL } },
		{ CPU_VENDOR_UMC, 4,{ NULL, "U5D", "U5S", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL } },
		{ CPU_VENDOR_NEXGEN, 5,{ "Nx586", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL } },
		{ CPU_VENDOR_RISE, 5,{ "iDragon", NULL, "iDragon", NULL, NULL, NULL, NULL, NULL, "iDragon II", "iDragon II", NULL, NULL, NULL, NULL, NULL, NULL } }
	};
	struct cpu_model_info *info = cpu_models;

	if (m_model >= 16) return NULL;

	for (int i = 0; i < sizeof(cpu_models) / sizeof(struct cpu_model_info); i++) 
	{
		if (info->vendor == m_vendor && info->family == m_family) 
		{
			return info->model_names[m_model];
		}
		info++;
	}

	return NULL;
}

CPU::CPU()
{
	CPUID_INFO cpuid_info;
	// Get vendor ID
	
	cpuid(0x00000000, &cpuid_info);
	
	m_cpuid_level = cpuid_info.eax;
	*(uint32*)(m_vendorid + 0)= cpuid_info.ebx;
	*(uint32*)(m_vendorid + 4)= cpuid_info.edx;
	*(uint32*)(m_vendorid + 8)= cpuid_info.ecx;
	*(uint32*)(m_vendorid + 12) = 0;
	
	if (strcmp(m_vendorid, "GenuineIntel") == 0) 
	{
		m_vendor = CPU_VENDOR_INTEL;
		strcpy(m_vendorname,"Intel");
	}
	else if (strcmp(m_vendorid, "AuthenticAMD") == 0) 
	{
		m_vendor = CPU_VENDOR_AMD;
		strcpy(m_vendorname , "AMD");
	}
	else if (strcmp(m_vendorid, "CyrixInstead") == 0) 
	{
		m_vendor = CPU_VENDOR_CYRIX;
		strcpy(m_vendorname , "Cyrix");
	}
	else if (strcmp(m_vendorid, "UMC UMC UMC ") == 0) 
	{
		m_vendor = CPU_VENDOR_UMC;
		strcpy(m_vendorname , "UMC");
	}
	else if (strcmp(m_vendorid, "CentaurHauls") == 0) 
	{
		m_vendor = CPU_VENDOR_CENTAUR;
		strcpy(m_vendorname , "Centaur");
	}
	else if (strcmp(m_vendorid, "NexGenDriven") == 0) 
	{
		m_vendor = CPU_VENDOR_NEXGEN;
		strcpy(m_vendorname , "NexGen");
	}
	else if (strcmp(m_vendorid, "GenuineTMx86") == 0 || strcmp(m_vendorid, "TransmetaCPU") == 0)
	{
		m_vendor = CPU_VENDOR_TRANSMETA;
		strcpy(m_vendorname , "Transmeta");
	}
	else 
	{
		m_vendor = CPU_VENDOR_UNKNOWN;
		strcpy(m_vendorname , m_vendorid);
	}

	// Get model and features
	if (m_cpuid_level >= 0x00000001)
	{
		cpuid(0x00000001, &cpuid_info);
		m_family = (cpuid_info.eax >> 8) & 15;
		m_model = (cpuid_info.eax >> 4) & 15;
		m_stepping = cpuid_info.eax & 15;
		m_features = cpuid_info.edx;
	}

	// SEP CPUID bug: Pentium Pro reports SEP but doesn't have it
	if (m_family == 6 && m_model < 3 && m_stepping < 3) m_features &= ~CPU_FEATURE_SEP;

	// Get brand string
	cpuid(0x80000000, &cpuid_info);
	if (cpuid_info.eax > 0x80000000)
	{
		char model[64];
		char *p, *q;
		int space;

		memset(model, 0, 64);
		cpuid(0x80000002, (CPUID_INFO*)model);
		cpuid(0x80000003, (CPUID_INFO*)(model + 16));
		cpuid(0x80000004, (CPUID_INFO*)(model + 32));

		// Trim brand string
		p = model;
		q = m_modelid;
		space = 0;
		while (*p == ' ') p++;
		while (*p) 
		{
			if (*p == ' ') 
			{
				space = 1;
			}
			else 
			{
				if (space) *q++ = ' ';
				space = 0;
				*q++ = *p;
			}
			p++;
		}
		*q = 0;
	}
	else 
	{
		char *modelid = table_lookup_model();
		if (modelid) 
		{
			sprintf(m_modelid, "%s %s", m_vendorname, modelid);
		}
		else 
		{
			sprintf(m_modelid, "%s %d86", m_vendorname, m_family);
		}
	}
	printf("cpu: %s family %d model %d stepping %d\n", m_modelid, m_family, m_model, m_stepping);
}


CPU::~CPU()
{
}


void CPU::cpuid(uint32 id, CPUID_INFO* cpuid_info)
{
	__asm
	{
		mov   eax, id
		cpuid
		push	esi
		mov		esi, [cpuid_info]
		mov		[esi]CPUID_INFO.eax, eax
		mov		[esi]CPUID_INFO.ebx, ebx
		mov		[esi]CPUID_INFO.ecx, ecx
		mov		[esi]CPUID_INFO.edx, edx
		pop		esi
	}
}