#include<context.h>
#include<memory.h>
#include<lib.h>

void clean_page(u32 pfn) {
	u64* startAddr = (u64*) osmap(pfn);
	u32 i;
	for(i=0;i<512;i++)
		*(startAddr + i) = 0;
}
u64 get_index(struct mm_segment seg, u32 what, u32 level) {
	u64 mask;
	u64 vaddr = (what!=MM_SEG_STACK)? seg.start: (seg.end-1);
	switch(level) {
		case 4:
		mask = 0b0000000000000000111111111000000000000000000000000000000000000000;
		return ((vaddr & mask) >> 39);
		case 3:
		mask = 0b0000000000000000000000000111111111000000000000000000000000000000;
		return ((vaddr & mask) >> 30);
		case 2:
		mask = 0b0000000000000000000000000000000000111111111000000000000000000000;
		return ((vaddr & mask) >> 21);
		case 1:
		mask = 0b0000000000000000000000000000000000000000000111111111000000000000;
		return ((vaddr & mask) >> 12);
	}
}

void prepare_context_mm(struct exec_context *ctx)
{
	u32 l4pfn = os_pfn_alloc(OS_PT_REG);
	u64* l4paddr = (u64*) osmap(l4pfn);
	clean_page(l4pfn);
	ctx->pgd = l4pfn;

	u32 l3pfn_data;	 u64* l3paddr_data;
	u32 l3pfn_stack; u64* l3paddr_stack;
	u32 l3pfn_code;	 u64* l3paddr_code;

	u32 l2pfn_data;	 u64* l2paddr_data;
	u32 l2pfn_stack = 0x1234; u64* l2paddr_stack;
	u32 l2pfn_code;	 u64* l2paddr_code;

	u32 l1pfn_data;	 u64* l1paddr_data;
	u32 l1pfn_stack = 0x4321; u64* l1paddr_stack;
	u32 l1pfn_code;	 u64* l1paddr_code;

	u32 pfn_data;	u64* paddr_data;
	u32 pfn_stack;	u64* paddr_stack;
	u32 pfn_code;	u64* paddr_code;

	/* ================================ LINKING LEVEL 4 TO LEVEL 3 ================================ */
	if((*(l4paddr + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 4)) & 0x1) == 0)
	{
		l3pfn_data = os_pfn_alloc(OS_PT_REG);
		clean_page(l3pfn_data);
		l3paddr_data = (u64*) osmap(l3pfn_data);
		*(l4paddr + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 4)) = 7 + (l3pfn_data << 12);
	} else {
		l3pfn_data = (*(l4paddr + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 4)) >> 12);
		l3paddr_data = (u64*) osmap(l3pfn_data);
	}

	if((*(l4paddr + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 4)) & 0x1) == 0) // If not allocated
	{
		l3pfn_code = os_pfn_alloc(OS_PT_REG);
		clean_page(l3pfn_code);
		l3paddr_code = (u64*) osmap(l3pfn_code);
		*(l4paddr + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 4)) = 7 + (l3pfn_code << 12);
	} else {
		l3pfn_code = ((*(l4paddr + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 4))) >> 12);
		l3paddr_code = (u64*) osmap(l3pfn_code);
	}

	if((*(l4paddr + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 4)) & 0x1) == 0) // If not allocated
	{
		l3pfn_stack = os_pfn_alloc(OS_PT_REG);
		clean_page(l3pfn_stack);
		l3paddr_stack = (u64*) osmap(l3pfn_stack);
		*(l4paddr + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 4)) = 7 + (l3pfn_stack << 12);
	} else {
		l3pfn_stack = *(l4paddr + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 4)) >> 12;
		l3paddr_stack = (u64*) osmap(l3pfn_stack);
	}


	/* ================================ LINKING LEVEL 3 TO LEVEL 2 ================================ */
	if((*(l3paddr_data + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 3)) & 0x1) == 0)
	{
		l2pfn_data = os_pfn_alloc(OS_PT_REG);
		clean_page(l2pfn_data);
		l2paddr_data = (u64*) osmap(l2pfn_data);
		*(l3paddr_data + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 3)) = 7 + (l2pfn_data << 12);
	} else {
		l2pfn_data = (*(l3paddr_data + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 3)) >> 12);
		l2paddr_data = (u64*) osmap(l2pfn_data);
	}

	if((*(l3paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 3)) & 0x1) == 0) // If not allocated
	{
		l2pfn_code = os_pfn_alloc(OS_PT_REG);
		clean_page(l2pfn_code);
		l2paddr_code = (u64*) osmap(l2pfn_code);
		*(l3paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 3)) = 7 + (l2pfn_code << 12);
	} else {
		l2pfn_code = (*(l3paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 3))) >> 12;
		l2paddr_code = (u64*) osmap(l2pfn_code);
	}

	if((*(l3paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 3)) & 0x1) == 0) // If not allocated
	{
		l2pfn_stack = os_pfn_alloc(OS_PT_REG);
		clean_page(l2pfn_stack);
		l2paddr_stack = (u64*) osmap(l2pfn_stack);
		*(l3paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 3)) = 7 + (l2pfn_stack << 12);
	} else {
		l2pfn_stack = *(l3paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 3)) >> 12;
		l2paddr_stack = (u64*) osmap(l2pfn_stack);
	}

	/* ================================ LINKING LEVEL 2 TO LEVEL 1 ================================ */
	if((*(l2paddr_data + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 2)) & 0x1) == 0)
	{
		l1pfn_data = os_pfn_alloc(OS_PT_REG);
		clean_page(l1pfn_data);
		l1paddr_data = (u64*) osmap(l1pfn_data);
		*(l2paddr_data + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 2)) = 7 + (l1pfn_data << 12);
	} else {
		l1pfn_data = ((*(l2paddr_data + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 2))) >> 12);
		l1paddr_data = (u64*) osmap(l1pfn_data);
	}

	if((*(l2paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 2)) & 0x1) == 0)
	{
		l1pfn_code = os_pfn_alloc(OS_PT_REG);
		clean_page(l1pfn_code);
		l1paddr_code = (u64*) osmap(l1pfn_code);
		*(l2paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 2)) = 7 + (l1pfn_code << 12);
	} else {
		l1pfn_code = ((*(l2paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 2))) >> 12);
		l1paddr_code = (u64*) osmap(l1pfn_code);
	}

	if((*(l2paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 2)) & 0x1) == 0)
	{
		l1pfn_stack = os_pfn_alloc(OS_PT_REG);
		clean_page(l1pfn_stack);
		l1paddr_stack = (u64*) osmap(l1pfn_stack);
		*(l2paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 2)) = 7 + (l1pfn_stack << 12);
	} else {
		l1pfn_stack = ((*(l2paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 2))) >> 12);
		l1paddr_stack = (u64*) osmap(l1pfn_stack);
	}

	/* ================================ LINKING LEVEL 1 TO ACTUAL DATA ================================ */
	pfn_data = ctx->arg_pfn;
	paddr_data = (u64*) osmap(pfn_data);
	*(l1paddr_data + get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 1)) = 7 + (pfn_data << 12);

	if((*(l1paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 1)) & 0x1) == 0)
	{
		pfn_code = os_pfn_alloc(USER_REG);
		clean_page(pfn_code);
		paddr_code = (u64*) osmap(pfn_code);
		*(l1paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 1)) = 5 + (pfn_code << 12);
	} else {
		pfn_code = ((*(l1paddr_code + get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 1))) >> 12);
		paddr_code = (u64*) osmap(pfn_code);
	}

	if((*(l1paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 1)) & 0x1) == 0)
	{
		pfn_stack = os_pfn_alloc(USER_REG);
		clean_page(pfn_stack);
		paddr_stack = (u64*) osmap(pfn_stack);
		*(l1paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 1)) = 7 + (pfn_stack << 12);
	} else {
		pfn_stack = ((*(l1paddr_stack + get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 1))) >> 12);
		paddr_stack = (u64*) osmap(pfn_stack);
	}
}

void push_check(u32 arr[], int* tos, u32 pfn) {
	int i=0;
	for(i=0;i<(*tos);i++)
		if(pfn==arr[i])
			return;
	(*tos) = (*tos) + 1;
	arr[(*tos)-1] = pfn;
}
void cleanup_context_mm(struct exec_context *ctx)
{
	u32 temp;

	u32 p1[50];
	u32 p2[50];
	int t1 = 0, t2 = 0;

	u32 l4pfn = ctx->pgd;
	push_check(p1, &t1, l4pfn);

	u32 l4offset_data = get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 4);
	u32 l4offset_code = get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 4);
	u32 l4offset_stack = get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 4);

	u32 l3offset_data = get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 3);
	u32 l3offset_code = get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 3);
	u32 l3offset_stack = get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 3);

	u32 l2offset_data = get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 2);
	u32 l2offset_code = get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 2);
	u32 l2offset_stack = get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 2);

	u32 l1offset_data = get_index(ctx->mms[MM_SEG_DATA], MM_SEG_DATA, 1);
	u32 l1offset_code = get_index(ctx->mms[MM_SEG_CODE], MM_SEG_CODE, 1);
	u32 l1offset_stack = get_index(ctx->mms[MM_SEG_STACK], MM_SEG_STACK, 1);

	u64 *l4addr = (u64*) osmap(l4pfn);

	u32 l3pfn_data = (*(l4addr + l4offset_data)) >> 12;
	u32 l3pfn_code = (*(l4addr + l4offset_code)) >> 12;
	u32 l3pfn_stack = (*(l4addr + l4offset_stack)) >> 12;
	push_check(p1, &t1, l3pfn_data);
	push_check(p1, &t1, l3pfn_code);
	push_check(p1, &t1, l3pfn_stack);

	u64 *l3addr_data = (u64*) osmap(l3pfn_data);
	u64 *l3addr_code = (u64*) osmap(l3pfn_code);
	u64 *l3addr_stack = (u64*) osmap(l3pfn_stack);

	u32 l2pfn_data = (*(l3addr_data + l3offset_data)) >> 12;
	u32 l2pfn_code = (*(l3addr_code + l3offset_code)) >> 12;
	u32 l2pfn_stack = (*(l3addr_stack + l3offset_stack)) >> 12;
	push_check(p1, &t1, l2pfn_data);
	push_check(p1, &t1, l2pfn_code);
	push_check(p1, &t1, l2pfn_stack);

	u64 *l2addr_data = (u64*) osmap(l2pfn_data);
	u64 *l2addr_code = (u64*) osmap(l2pfn_code);
	u64 *l2addr_stack = (u64*) osmap(l2pfn_stack);

	u32 l1pfn_data = (*(l2addr_data + l2offset_data)) >> 12;
	u32 l1pfn_code = (*(l2addr_code + l2offset_code)) >> 12;
	u32 l1pfn_stack = (*(l2addr_stack + l2offset_stack)) >> 12;
	push_check(p1, &t1, l1pfn_data);
	push_check(p1, &t1, l1pfn_code);
	push_check(p1, &t1, l1pfn_stack);

	u64 *l1addr_data = (u64*) osmap(l1pfn_data);
	u64 *l1addr_code = (u64*) osmap(l1pfn_code);
	u64 *l1addr_stack = (u64*) osmap(l1pfn_stack);

	u32 pfn_data = (*(l1addr_data + l1offset_data)) >> 12;
	u32 pfn_code = (*(l1addr_code + l1offset_code)) >> 12;
	u32 pfn_stack = (*(l1addr_stack + l1offset_stack)) >> 12;
	push_check(p2, &t2, pfn_data);
	push_check(p2, &t2, pfn_code);
	push_check(p2, &t2, pfn_stack);

	int i;
	for(i=0;i<t1;i++)
		os_pfn_free(OS_PT_REG, p1[i]);
	os_pfn_free(OS_PT_REG, p1[t1-1]+1);

	for(i=0;i<t2;i++)
		os_pfn_free(USER_REG, p2[i]);
	
	return;
}
