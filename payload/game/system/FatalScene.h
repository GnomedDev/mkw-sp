// The porting tool fails on the address itself
void **GetSpAllocAddr();
#define lyt_spAlloc (*GetSpAllocAddr())
