#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <list>
#include <map>
using namespace std;

#define HAX_EMULATE_STATE_MMIO		0x1
#define HAX_EMULATE_STATE_REAL		0x2
#define HAX_EMULATE_STATE_NONE		0x3
#define HAX_EMULATE_STATE_INITIAL	0x4

struct BIT_FIELD
{
	int 	bit_index;
	int     bit_count;
	char*	bit_name;
	char*   description;
};

class HAXM_CPU
{
private:
	UINT32 m_vmid;
	UINT32 m_vcpuid;
	HANDLE m_hCPU;
	hax_tunnel *m_tunnel;
	hax_fastmmio*  m_iobuf;//该指针类型根据逆向工程获取
	//vcpu_state_t   m_state;
	UINT32 m_emulation_state;
public:
	vcpu_state_t   m_state;
	HAXM_CPU(UINT32 vmid, UINT32 vcpuid)
	{
		m_vmid = vmid;
		m_vcpuid = vcpuid;
		m_hCPU = INVALID_HANDLE_VALUE;
		m_tunnel = NULL;
		m_emulation_state = HAX_EMULATE_STATE_INITIAL;
		memset(&m_state, 0, sizeof(m_state));
	}

	~HAXM_CPU()
	{
		close();
	}

	bool  open()
	{
		char name[256];
		sprintf(name, "\\\\.\\hax_vm%02d_vcpu%02d", m_vmid, m_vcpuid);

		m_hCPU = CreateFile(
			name,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (m_hCPU == INVALID_HANDLE_VALUE)
		{
			printf("Open the vcpu devcie error:%s, ec:%d\n", name, GetLastError());
			return false;
		}
		return setup_channel() && get_cpu_state();
	}

	bool  close()
	{
		if (m_hCPU != INVALID_HANDLE_VALUE)
		{
			if (!CloseHandle(m_hCPU)) return false;
			m_hCPU = INVALID_HANDLE_VALUE;
		}
		return true;
	}

	bool  setup_channel()
	{
		int ret;
		struct hax_tunnel_info info;
		DWORD dSize = 0;

		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_IOCTL_SETUP_TUNNEL,
			NULL, 0,
			&info, sizeof(info),
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			printf("Failed to setup the hax tunnel\n");
			return false;
		}

		if (info.size < sizeof(hax_tunnel))
		{
			printf("Invalid hax tunnel size %x\n", info.size);
			return false;
		}
		m_tunnel = (struct hax_tunnel *)(info.va);
		m_iobuf = (hax_fastmmio *)(info.io_va);
		return true;
	}

	bool  get_msrs(hax_msr_data *msrs)
	{
		int ret;
		hax_fd fd;
		HANDLE hDeviceVCPU;
		DWORD dSize = 0;
		for (int i = 0; i < msrs->nr_msr;i++)
		{
			msrs->entries[i].value = 0x123456789ABCDEF0;
		}
		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_IOCTL_GET_MSRS,
			msrs, sizeof(*msrs),
			msrs, sizeof(*msrs),
			&dSize,
			(LPOVERLAPPED)NULL);
		return (!!ret);
	}

	bool  set_msrs(hax_msr_data *msrs)
	{
		int ret;
		hax_fd fd;
		HANDLE hDeviceVCPU;
		DWORD dSize = 0;
		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_IOCTL_SET_MSRS,
			msrs, sizeof(*msrs),
			msrs, sizeof(*msrs),
			&dSize,
			(LPOVERLAPPED)NULL);
		return (!!ret);
	}

	bool   get_fpu_state(fx_layout *fpu)
	{
		int ret;
		DWORD dSize = 0;
		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_IOCTL_GET_FPU,
			NULL, 0,
			fpu, sizeof(*fpu),
			&dSize,
			(LPOVERLAPPED)NULL);
		return (!!ret);
	}

	bool  sget_fpu_state(fx_layout *fpu)
	{
		int ret;
		DWORD dSize = 0;
		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_IOCTL_SET_FPU,
			fpu, sizeof(*fpu),
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		return (!!ret);
	}

	bool  get_cpu_state()
	{
		int ret;
		DWORD dSize = sizeof(m_state);
		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_GET_REGS,
			NULL, 0,
			&m_state, sizeof(m_state),
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			printf("get_vcpu_state error:%d\n", GetLastError());
			return false;
		}
		return true;
	}

	bool  set_cpu_state(vcpu_state_t* state=NULL)
	{
		int ret;
		DWORD dSize;
		if (!state) state = &m_state;
		if (state!=&m_state) m_state = *state;
		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_SET_REGS,
			state, sizeof(vcpu_state_t),
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret)
		{
			printf("set_vcpu_state error:%d\n", GetLastError());
			return false;
		}
		return true;
	}

	bool  inject_interrupt(int vector)
	{
		int ret;
		DWORD dSize;
		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_IOCTL_INTERRUPT,
			&vector, sizeof(vector),
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		return (!!ret);
	}

	void  show_reg_bits(char* reg_name, BIT_FIELD* bits, UINT64 reg_value)
	{
		printf("%s=%016I64X\n", reg_name, reg_value);
		UINT64 bit_mask = 0;
		UINT64 bit_value= 0;
		for (int i = 0; i < 64; i++)
		{
			if (bits[i].bit_name == NULL) break;
			printf("%12s =", bits[i].bit_name);
			bit_mask = 0;
			for (int j = 0; j < bits[i].bit_count; j++) bit_mask = bit_mask * 2 + 1;
			bit_value = (reg_value >> bits[i].bit_index) & bit_mask;
			printf("%d %s [bit%d]\n", (UINT32)bit_value, bits[i].description, bits[i].bit_index);
		}
	}

	bool  show_state()
	{
		printf("--------------------------------------- GPRs -------------------------------\n");
		printf("RAX=%016I64X RBX=%016I64X RCX=%016I64X RDX=%016I64X \n", m_state._rax, m_state._rbx, m_state._rcx,m_state._rdx);
		printf("RSP=%016I64X RBP=%016I64X RSI=%016I64X RDI=%016I64X \n", m_state._rsp, m_state._rbp, m_state._rsi, m_state._rdi);
		printf("R8 =%016I64X R9 =%016I64X R10=%016I64X R11=%016I64X \n", m_state._r8, m_state._r9, m_state._r10, m_state._r11);
		printf("R12=%016I64X R13=%016I64X R14=%016I64X R15=%016I64X \n", m_state._r12, m_state._r13, m_state._r14, m_state._r15);
		printf("RIP=%016I64X FLG=%016I64X PDE=%016I64X EFE=%08X \n", m_state._rip, m_state._rflags, m_state._pde, m_state._efer);
		printf("\n");
		printf("CR0=%016I64X CR2=%016I64X CR3=%016I64X CR4=%016I64X \n", m_state._cr0, m_state._cr2, m_state._cr3, m_state._cr4);
		printf("DR0=%016I64X DR1=%016I64X DR2=%016I64X DR3=%016I64X \n", m_state._dr0, m_state._dr1, m_state._dr2, m_state._dr3);
		printf("DR6=%016I64X DR7=%016I64X INT=%016I64X ACT=%08X \n", m_state._dr6, m_state._dr7, m_state._interruptibility_state, m_state._activity_state);
		printf("\n");
		printf("SYSENTER_CS=%08X RIP=%016I64X RSP=%016I64X \n", m_state._sysenter_cs, m_state._sysenter_eip, m_state._sysenter_esp);
		printf("\n");
		printf("sreg  selector     ar         base       limit\n");
		printf("CS      %04X %8X %016I64X %08X \n", m_state._cs.selector, m_state._cs.ar, m_state._cs.base, m_state._cs.limit);
		printf("DS      %04X %8X %016I64X %08X \n", m_state._ds.selector, m_state._ds.ar, m_state._ds.base, m_state._ds.limit);
		printf("ES      %04X %8X %016I64X %08X \n", m_state._es.selector, m_state._es.ar, m_state._es.base, m_state._es.limit);
		printf("FS      %04X %8X %016I64X %08X \n", m_state._fs.selector, m_state._fs.ar, m_state._fs.base, m_state._fs.limit);
		printf("GS      %04X %8X %016I64X %08X \n", m_state._gs.selector, m_state._gs.ar, m_state._gs.base, m_state._gs.limit);
		printf("SS      %04X %8X %016I64X %08X \n", m_state._ss.selector, m_state._ss.ar, m_state._ss.base, m_state._ss.limit);
		printf("\n");
		printf("GDTR    %04X %8X %016I64X %08X \n", m_state._gdt.selector, m_state._gdt.ar, m_state._gdt.base, m_state._gdt.limit);
		printf("IDTR    %04X %8X %016I64X %08X \n", m_state._idt.selector, m_state._idt.ar, m_state._idt.base, m_state._idt.limit);
		printf("LDTR    %04X %8X %016I64X %08X \n", m_state._ldt.selector, m_state._ldt.ar, m_state._ldt.base, m_state._ldt.limit);
		printf("TR      %04X %8X %016I64X %08X \n", m_state._tr.selector, m_state._tr.ar, m_state._tr.base, m_state._tr.limit);
		printf("\n");
		
		BIT_FIELD CR0[] = {
		{ 0, 1, "PE", "Protection Enabled" },
		{ 1, 1, "MP", "Monitor Coprocessor" },
		{ 2, 1, "EM", "Emulation FPU" },
		{ 3, 1, "TS", "Task Switched" },
		{ 4, 1, "ET", "Extension Type" },
		{ 5, 1, "NE", "Numeric Error" },
		{ 16, 1, "WP", "Write Protect" },
		{ 18, 1, "AM", "Alignment Mask" },
		{ 29, 1, "NW", "Not Writethrough" },
		{ 30, 1, "CD", "Cache Disable" },
		{ 31, 1, "PG", "Paging" },
		{0,0,NULL,NULL}
		};
		
		BIT_FIELD CR4[] = {
			{ 0, 1, "VME", "Virtual-8086 Mode Extensions" },
			{ 1, 1, "PVI", "Protected-Mode Virtual Interrupts" },
			{ 2, 1, "TDS", "Time Stamp Disable" },
			{ 3, 1, "DE", "Debugging Extensions" },
			{ 4, 1, "PSE", "Page Size Extensions" },
			{ 5, 1, "PAE", "Physical Address Extension" },
			{ 6, 1, "MCE", "Machine-Check Enable" },
			{ 7, 1, "PG", "Page Global Enable" },
			{ 8, 1, "PCE", "Performance-Monitoring Counter Enable" },
			{ 9, 1, "OSFXSR", "Operating System Support for FXSAVE and FXRSTOR instructions" },
			{ 10, 1, "OSXMMEXCPT", "Operating System Support for Unmasked SIMD Floating-Point Exceptions" },
			{ 13, 1, "VMXE", "VMX-Enable" },
			{ 14, 1, "WMXE", "SMX-Enable" },
			{ 16, 1, "FSGSBASE", "FSGSBASE-Enable" },
			{ 17, 1, "PCIDE", "PCID-Enable" },
			{ 18, 1, "OSXSAVE", "XSAVE and Processor Extended States-Enable" },
			{ 20, 1, "SMEP", "SMEP-Enable" },
			{ 21, 1, "SMAP", "SMAP-Enable" },
			{ 22, 1, "PKE", "Protection-Key-Enable" },
			{ 0, 0, NULL, NULL }
		};
		//Extended Feature Enable Register
		BIT_FIELD EFER[] = {
			{ 0, 1, "SCE", "System-Call Extension" },
			{ 8, 1, "LME", "Long Mode Enable" },
			{ 10, 1, "LMA", "Long Mode Active" },
			{ 11, 1, "NXE", "No-Execute Enable " },
			{ 12, 1, "SVME", "Secure Virtual Machine Enable" },
			{ 13, 1, "LMSLE", "Long Mode Segment Limit Enable" },
			{ 14, 1, "FFXSR", "Fast FXSAVE/FXRSTOR" },
			{ 0, 0, NULL, NULL }
		};

		//show_reg_bits("CR0", CR0, m_state._cr0);
		//show_reg_bits("CR4", CR4, m_state._cr4);
		//show_reg_bits("EFER", EFER, m_state._efer);

		//show_reg_bits("CR0", CR0, 0x21);
		//show_reg_bits("CR4", CR4, 0x2000);

		return true;
	}

	void  write_vmcs(int field, UINT64 value)
	{
#define SET_SEGMENT_ACCESS(seg, value) \
	(seg).available = ((value) >> 12) & 1; \
	(seg).present = ((value) >> 7) & 1; \
	(seg).dpl = ((value) >> 5) & ~(~0 << (6-5+1)); \
	(seg).desc = ((value) >> 4) & 1; \
	(seg).type = ((value) >> 0) & ~(~0 << (3-0+1));


		// Set the required field
		switch (field)
		{
		case VMCS_GUEST_ES_SELECTOR:		m_state._es.selector = (uint16_t)value;	break;
		case VMCS_GUEST_ES_LIMIT:			m_state._es.limit = (uint32)value;		break;
		case VMCS_GUEST_ES_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._es, value)	break;
		case VMCS_GUEST_ES_BASE:			m_state._es.base = (uint64)value;			break;

		case VMCS_GUEST_CS_SELECTOR:		m_state._cs.selector = (uint16_t)value;	break;
		case VMCS_GUEST_CS_LIMIT:			m_state._cs.limit = (uint32)value;		break;
		case VMCS_GUEST_CS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._cs, value)	break;
		case VMCS_GUEST_CS_BASE:			m_state._cs.base = (uint64)value;			break;

		case VMCS_GUEST_SS_SELECTOR:		m_state._ss.selector = (uint16_t)value;	break;
		case VMCS_GUEST_SS_LIMIT:			m_state._ss.limit = (uint32)value;		break;
		case VMCS_GUEST_SS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._ss, value)	break;
		case VMCS_GUEST_SS_BASE:			m_state._ss.base = (uint64)value;			break;

		case VMCS_GUEST_DS_SELECTOR:		m_state._ds.selector = (uint16_t)value;	break;
		case VMCS_GUEST_DS_LIMIT:			m_state._ds.limit = (uint32)value;		break;
		case VMCS_GUEST_DS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._ds, value)	break;
		case VMCS_GUEST_DS_BASE:			m_state._ds.base = (uint64)value;			break;

		case VMCS_GUEST_FS_SELECTOR:		m_state._fs.selector = (uint16_t)value;	break;
		case VMCS_GUEST_FS_LIMIT:			m_state._fs.limit = (uint32)value;		break;
		case VMCS_GUEST_FS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._fs, value)	break;
		case VMCS_GUEST_FS_BASE:			m_state._fs.base = (uint64)value;			break;

		case VMCS_GUEST_GS_SELECTOR:		m_state._gs.selector = (uint16_t)value;	break;
		case VMCS_GUEST_GS_LIMIT:			m_state._gs.limit = (uint32)value;		break;
		case VMCS_GUEST_GS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._gs, value)	break;
		case VMCS_GUEST_GS_BASE:			m_state._gs.base = (uint64)value;			break;

		case VMCS_GUEST_LDTR_SELECTOR:		m_state._ldt.selector = (uint16_t)value;	break;
		case VMCS_GUEST_LDTR_LIMIT:			m_state._ldt.limit = (uint32)value;		break;
		case VMCS_GUEST_LDTR_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._ldt, value)	break;
		case VMCS_GUEST_LDTR_BASE:			m_state._ldt.base = (uint64)value;		break;

		case VMCS_GUEST_TR_SELECTOR:		m_state._tr.selector = (uint16_t)value;	break;
		case VMCS_GUEST_TR_LIMIT:			m_state._tr.limit = (uint32)value;		break;
		case VMCS_GUEST_TR_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(m_state._tr, value)	break;
		case VMCS_GUEST_TR_BASE:			m_state._tr.base = (uint64)value;			break;

		case VMCS_GUEST_GDTR_LIMIT:			m_state._gdt.limit = (uint32)value;		break;
		case VMCS_GUEST_GDTR_BASE:			m_state._gdt.base = (uint64)value;		break;

		case VMCS_GUEST_IDTR_LIMIT:			m_state._idt.limit = (uint32)value;		break;
		case VMCS_GUEST_IDTR_BASE:			m_state._idt.base = (uint64)value;		break;

		case VMCS_GUEST_CR0:				m_state._cr0 = (uint64)value;				break;
		case VMCS_GUEST_CR3:				m_state._cr3 = (uint64)value;				break;
		case VMCS_GUEST_CR4:				m_state._cr4 = (uint64)value;				break;

		case VMCS_GUEST_DR7:				m_state._dr7 = (uint64)value;				break;
		case VMCS_GUEST_RSP:				m_state._rsp = (uint64)value;				break;
		case VMCS_GUEST_RIP:				m_state._rip = (uint64)value;				break;
		case VMCS_GUEST_RFLAGS:				m_state._rflags = (uint64)value;			break;

		case VMCS_GUEST_IA32_SYSENTER_CS:	m_state._sysenter_cs = (uint32)value;		break;
		case VMCS_GUEST_IA32_SYSENTER_ESP:	m_state._sysenter_esp = (uint64)value;	break;
		case VMCS_GUEST_IA32_SYSENTER_EIP:	m_state._sysenter_eip = (uint64)value;	break;

		default:
			__debugbreak();
		}

	}

	void   reset(UINT32 EntryPoint)
	{
		m_emulation_state = HAX_EMULATE_STATE_INITIAL;
		m_tunnel->user_event_pending = 0;
		m_tunnel->ready_for_interrupt_injection = 0;

		if (!get_cpu_state()) __debugbreak();

		write_vmcs(VMCS_GUEST_CS_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_CS_LIMIT, 0xfffff); //代码可能位于0x10000以上，因此将CS段限设为1M
		write_vmcs(VMCS_GUEST_CS_ACCESS_RIGHTS, 0x9b);
		write_vmcs(VMCS_GUEST_CS_BASE, 0);

		write_vmcs(VMCS_GUEST_DS_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_DS_LIMIT, 0xffff);
		write_vmcs(VMCS_GUEST_DS_ACCESS_RIGHTS, 0x93);
		write_vmcs(VMCS_GUEST_DS_BASE, 0);

		write_vmcs(VMCS_GUEST_ES_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_ES_LIMIT, 0xffff);
		write_vmcs(VMCS_GUEST_ES_ACCESS_RIGHTS, 0x93);
		write_vmcs(VMCS_GUEST_ES_BASE, 0);

		write_vmcs(VMCS_GUEST_FS_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_FS_LIMIT, 0xffff);
		write_vmcs(VMCS_GUEST_FS_ACCESS_RIGHTS, 0x93);
		write_vmcs(VMCS_GUEST_FS_BASE, 0);

		write_vmcs(VMCS_GUEST_GS_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_GS_LIMIT, 0xffff);
		write_vmcs(VMCS_GUEST_GS_ACCESS_RIGHTS, 0x93);
		write_vmcs(VMCS_GUEST_GS_BASE, 0);

		write_vmcs(VMCS_GUEST_SS_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_SS_LIMIT, 0xffff);
		write_vmcs(VMCS_GUEST_SS_ACCESS_RIGHTS, 0x93);
		write_vmcs(VMCS_GUEST_SS_BASE, 0);

		write_vmcs(VMCS_GUEST_LDTR_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_LDTR_LIMIT, 0);
		write_vmcs(VMCS_GUEST_LDTR_ACCESS_RIGHTS, 0x10000);
		write_vmcs(VMCS_GUEST_LDTR_BASE, 0);

		write_vmcs(VMCS_GUEST_TR_SELECTOR, 0);
		write_vmcs(VMCS_GUEST_TR_LIMIT, 0);
		write_vmcs(VMCS_GUEST_TR_ACCESS_RIGHTS, 0x83);
		write_vmcs(VMCS_GUEST_TR_BASE, 0);

		write_vmcs(VMCS_GUEST_GDTR_LIMIT, 0);
		write_vmcs(VMCS_GUEST_GDTR_BASE, 0);

		write_vmcs(VMCS_GUEST_IDTR_LIMIT, 0);
		write_vmcs(VMCS_GUEST_IDTR_BASE, 0);

		write_vmcs(VMCS_GUEST_CR0, 0x20);
		write_vmcs(VMCS_GUEST_CR3, 0x0);
		write_vmcs(VMCS_GUEST_CR4, 0x2000);

		write_vmcs(VMCS_GUEST_RSP, EntryPoint-4);
		write_vmcs(VMCS_GUEST_RIP, EntryPoint);
		write_vmcs(VMCS_GUEST_RFLAGS, 0x200 | 0x2);

		if (!set_cpu_state()) __debugbreak();
	}

	int  run()
	{
		int ret;
		DWORD dSize = 0;

		ret = DeviceIoControl(m_hCPU,
			HAX_VCPU_IOCTL_RUN,
			NULL, 0,
			NULL, 0,
			&dSize,
			(LPOVERLAPPED)NULL);
		if (!ret) return 0;
		return m_tunnel->_exit_status;
	}
};

int write_vmcs(char* name, int value)
{
	return 0;
}

//HAX_VCPU_IOCTL_SET_MSRS
int set_msrs(int msr, int value)
{
	if (msr <= 0x1d9)
	{
		if (msr == 0x1d9) return 0;
		if (msr > 0xfe)
		{
			if (msr == 0x174)  return write_vmcs("GUEST_SYSENTER_CS", value);
			if (msr == 0x175)  return write_vmcs("GUEST_SYSENTER_ESP", value);
			if (msr == 0x175)  return write_vmcs("GUEST_SYSENTER_EIP", value);
			if (msr == 0x17A)  return 0;
			if (msr == 0x1A0)  return 0;
			goto LABEL_12;
		}
		if (msr == 0xFE)                       // IA32_MTRRCAP (MTRRcap)
		{
			//*(_QWORD *)&vcpu->u0208 = value;
			return 0;
		}
		if (msr == 16) //IA32_TIME_STAMP_COUNTER (TSC)
		{
			return write_vmcs("VMX_TSC_OFFSET", value);
		}
		if (msr == 27) return 0;                      // IA32_APIC_BASE (APIC_BASE)
		if (msr <= 0x29) goto LABEL_12;
		if (msr <= 0x2C || msr == 0x79) return 0; //IA32_BIOS_UPDT_TRIG(BIOS_UPDT_TRIG)=0x79
		if (msr == 0x8B) return 0; //IA32_BIOS_SIGN_ID
		goto LABEL_12;
	}
LABEL_12:
	return 0;
	//*((_QWORD *)&vcpu->u0008 + 2 * (((unsigned __int8)msr >> 1) + 39i64)) = value;
}