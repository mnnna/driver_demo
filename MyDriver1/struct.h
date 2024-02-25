#pragma once
#include"ia32/ia32.hpp"
# include<ntddk.h>

struct PAGE_TABLE {
	struct {
		pte_64* pte;
		pde_64* pde;
		pdpte_64 pedpte;
		pml4e_64 pml4e;
	} Entry;
	ULONG64 VirtualAddress;
};