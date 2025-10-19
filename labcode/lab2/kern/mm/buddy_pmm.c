#include <pmm.h>
#include <binarytree.h>
#include <string.h>
#include <buddy_pmm.h>
#include <stdio.h>
#include <list.h>

/* In the first fit algorithm, the allocator keeps a list of free blocks (known as the free list) and,
   on receiving a request for memory, scans along the list for the first block that is large enough to
   satisfy the request. If the chosen block is significantly larger than that requested, then it is 
   usually split, and the remainder added to the list as another free block.
   Please see Page 196~198, Section 8.2 of Yan Wei Min's chinese book "Data Structure -- C programming language"
*/
// LAB2 EXERCISE 1: YOUR CODE
// you should rewrite functions: default_init,default_init_memmap,default_alloc_pages, default_free_pages.
/*
 * Details of FFMA
 * (1) Prepare: In order to implement the First-Fit Mem Alloc (FFMA), we should manage the free mem block use some list.
 *              The struct free_area_t is used for the management of free mem blocks. At first you should
 *              be familiar to the struct list in list.h. struct list is a simple doubly linked list implementation.
 *              You should know howto USE: list_init, list_add(list_add_after), list_add_before, list_del, list_next, list_prev
 *              Another tricky method is to transform a general list struct to a special struct (such as struct page):
 *              you can find some MACRO: le2page (in memlayout.h), (in future labs: le2vma (in vmm.h), le2proc (in proc.h),etc.)
 * (2) default_init: you can reuse the  demo default_init fun to init the free_list and set total_size to 0.
 *              free_list is used to record the free mem blocks. total_size is the total number for free mem blocks.
 * (3) default_init_memmap:  CALL GRAPH: kern_init --> pmm_init-->page_init-->init_memmap--> pmm_manager->init_memmap
 *              This fun is used to init a free block (with parameter: addr_base, page_number).
 *              First you should init each page (in memlayout.h) in this free block, include:
 *                  p->flags should be set bit PG_property (means this page is valid. In pmm_init fun (in pmm.c),
 *                  the bit PG_reserved is setted in p->flags)
 *                  if this page  is free and is not the first page of free block, p->property should be set to 0.
 *                  if this page  is free and is the first page of free block, p->property should be set to total num of block.
 *                  p->ref should be 0, because now p is free and no reference.
 *                  We can use p->page_link to link this page to free_list, (such as: list_add_before(&free_list, &(p->page_link)); )
 *              Finally, we should sum the number of free mem block: total_size+=n
 * (4) default_alloc_pages: search find a first free block (block size >=n) in free list and reszie the free block, return the addr
 *              of malloced block.
 *              (4.1) So you should search freelist like this:
 *                       list_entry_t le = &free_list;
 *                       while((le=list_next(le)) != &free_list) {
 *                       ....
 *                 (4.1.1) In while loop, get the struct page and check the p->property (record the num of free block) >=n?
 *                       struct Page *p = le2page(le, page_link);
 *                       if(p->property >= n){ ...
 *                 (4.1.2) If we find this p, then it' means we find a free block(block size >=n), and the first n pages can be malloced.
 *                     Some flag bits of this page should be setted: PG_reserved =1, PG_property =0
 *                     unlink the pages from free_list
 *                     (4.1.2.1) If (p->property >n), we should re-caluclate number of the the rest of this free block,
 *                           (such as: le2page(le,page_link))->property = p->property - n;)
 *                 (4.1.3)  re-caluclate total_size (number of the the rest of all free block)
 *                 (4.1.4)  return p
 *               (4.2) If we can not find a free block (block size >=n), then return NULL
 * (5) default_free_pages: relink the pages into  free list, maybe merge small free blocks into big free blocks.
 *               (5.1) according the base addr of withdrawed blocks, search free list, find the correct position
 *                     (from low to high addr), and insert the pages. (may use list_next, le2page, list_add_before)
 *               (5.2) reset the fields of pages, such as p->ref, p->flags (PageProperty)
 *               (5.3) try to merge low addr or high addr blocks. Notice: should change some pages's p->property correctly.
 */




struct binary_tree_node{
    unsigned char left_max;
    unsigned char right_max;
};
#define binary_tree_node_t struct binary_tree_node
// const int vacancy_size = sizeof(list_entry_t) - sizeof(uint64_t);
struct Page_bt {
    int ref;                        // page frame's reference counter
    uint64_t flags;                 // array of flags that describe the status of the page frame
    unsigned int property;          // the num of free block, used in first fit pm manager
    // unsigned char property;
    binary_tree_node_t node[2];
    char vacancy[sizeof(list_entry_t) - 2 * sizeof(binary_tree_node_t)];  //填空对齐，比较安全
};

typedef struct {
    struct Page_bt *base;
    unsigned int total_size;           
} free_area_bt_t;
static free_area_bt_t free_area;
#define binary_tree_root (free_area.base)
#define total_size (free_area.total_size)

// convert list entry to page_bt
// #define le2pagebt(bte, member)                 
//     to_struct((bte), struct Page_bt, member)
#define get_node(index) (&binary_tree_root[index / 2].node[index & 1])
#define LEFT_LEAF(index) ((index<<1)|1)
#define RIGHT_LEAF(index) ((index+1)<<1)
#define PARENT(index) (((index+1)>>1)-1)
#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
#define max(a,b) a>b?a:b
// static
// binary_tree_node_t *get_node(struct Page_bt *base, int index){
//     return base[index / 2].node + index % 2;
// }

static
int get_index(int offset, int property, int tot_size){
    int index = 0;
    while(tot_size > property){
        int half = tot_size >> 1;
        if (offset < half) {
            index = LEFT_LEAF(index);
            
        } else {
            index = RIGHT_LEAF(index);
            offset -= half;
        }
        tot_size = half;
    }
    return index;
}
static unsigned fixsize(unsigned size) {
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return size+1;
}


static unsigned char log2(int x) {
    unsigned char i = 0;
    while (x > 1) {
        x >>= 1;
        i++;
    }
    return i;
}

static void
buddy_init(void) {
    // binary_tree_init(&free_binary_tree);
    binary_tree_root = NULL;
    total_size = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    /*保证n是2的幂次方*/
    if (!IS_POWER_OF_2(n))
        n = fixsize(n)>>1;
    binary_tree_root = (struct Page_bt *)base;
    total_size = n;
    // cprintf("buddy_init_memmap: total_size = %d\n", total_size);
    
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
        /*LAB2 CHALLENGE 1: YUGE*/ 
        p->flags = p->property = 0;
        // p->offset = (uint64_t)p - (uint64_t)base;
        // 初始化每个页框的标志和属性信息，并将页框的偏移量设置为相对于基地址的偏移量
        set_page_ref(p, 0);
        // 清空当前页框的标志和属性信息，并将页框的引用计数设置为0

    }
    int tree_size = 2*n-1;
    unsigned char block_size = log2(n) + 1;
    for (int i=0; i<tree_size; i++) {
        if(IS_POWER_OF_2(i+1)){
            block_size --;
        }
        
        binary_tree_node_t *node = get_node(i);
        node->left_max = block_size;
        node->right_max = block_size;
    }
    // base_bt->property = n;
    // SetPageProperty(base);
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    if (!IS_POWER_OF_2(n))
        n = fixsize(n);
    // cprintf("buddy_alloc_pages: n = %d\n", n);
    if (n > total_size) {
        return NULL;
    }
    unsigned char log2n = log2(n) + 1;
    unsigned char log2_total_size = log2(total_size) + 1;
    unsigned char block_size = log2_total_size;
    
    int index = 0;
    int offset = 0;
    binary_tree_node_t *node = get_node(index);
    
    while (block_size > log2n) {
        node = get_node(index);
        if (node->right_max >= log2n) {
            index = RIGHT_LEAF(index);
            block_size --;
            offset += 1<<(block_size-1);
            continue;
        }
        if (node->left_max >= log2n) {
            index = LEFT_LEAF(index);
            block_size --;
            continue;
        }
        break;
    }
    if(block_size > log2n)
        return NULL;
    struct Page_bt *ret = binary_tree_root + offset;
    
    node = get_node(index);
    ret->property = 1<<(block_size-1);
    // ClearPageProperty(ret);
    node->left_max = 0;
    node->right_max = 0;
    unsigned char maxn = 0;
    
    //最后进行更新
    for(block_size; index>0; block_size++){
        int parent = PARENT(index);
        binary_tree_node_t *pnode = get_node(parent);
        if(index == LEFT_LEAF(parent)){
            pnode->left_max = maxn;
            maxn = max(maxn, pnode->right_max);
        }
        else{
            pnode->right_max = maxn;
            maxn = max(maxn, pnode->left_max);
        }
        index = parent;
    }
    return (struct Page *)ret;
}
static void
buddy_free_pages(struct Page *base, size_t n) {
    
    assert(n > 0);
    if(base == NULL)
        return;
    if (!IS_POWER_OF_2(n))
        n = fixsize(n);
    
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }
    // SetPageProperty(base);
    
    struct Page_bt *base_bt = (struct Page_bt *)base;
    int index = get_index(base_bt - binary_tree_root, n, total_size);
    binary_tree_node_t *node = get_node(index);
    
    base->property = 0;
    unsigned char block_size = log2(n) + 1;
    // if(block_size){
    node->left_max = block_size-1;
    node->right_max = block_size-1;
    // }
    unsigned char maxn = block_size;
    for(; index > 0; index = PARENT(index)){
        int parent = PARENT(index);
        binary_tree_node_t *pnode = get_node(parent);
        if(index == LEFT_LEAF(parent)){
            pnode->left_max = maxn;
        }
        else{
            pnode->right_max = maxn;
        }
        if(pnode->right_max == block_size && pnode->left_max == block_size)
            maxn = block_size+1;
        else
            maxn = max(pnode->right_max, pnode->left_max);
        block_size ++;
    }
}

static size_t
buddy_total_size_pages(void) {
    return total_size;
}

// static void
// basic_check(void) {
//     struct Page *p0, *p1, *p2;
//     p0 = p1 = p2 = NULL;
//     assert((p0 = alloc_page()) != NULL);
//     assert((p1 = alloc_page()) != NULL);
//     assert((p2 = alloc_page()) != NULL);

//     assert(p0 != p1 && p0 != p2 && p1 != p2);
//     assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

//     assert(page2pa(p0) < npage * PGSIZE);
//     assert(page2pa(p1) < npage * PGSIZE);
//     assert(page2pa(p2) < npage * PGSIZE);

//     list_entry_t free_list_store = free_list;
//     list_init(&free_list);
//     assert(list_empty(&free_list));

//     unsigned int total_size_store = total_size;
//     total_size = 0;

//     assert(alloc_page() == NULL);

//     free_page(p0);
//     free_page(p1);
//     free_page(p2);
//     assert(total_size == 3);

//     assert((p0 = alloc_page()) != NULL);
//     assert((p1 = alloc_page()) != NULL);
//     assert((p2 = alloc_page()) != NULL);

//     assert(alloc_page() == NULL);

//     free_page(p0);
//     assert(!list_empty(&free_list));

//     struct Page *p;
//     assert((p = alloc_page()) == p0);
//     assert(alloc_page() == NULL);

//     assert(total_size == 0);
//     free_list = free_list_store;
//     total_size = total_size_store;

//     free_page(p);
//     free_page(p1);
//     free_page(p2);
// }

// LAB2: below code is used to check the best fit allocation algorithm (your EXERCISE 1) 
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
buddy_check(void) {
    cprintf("buddy_check:\n");
    int score = 0 ,sumscore = 4;
    // int count = 0, total = 0;
    
    // list_entry_t *le = &free_list;
    // while ((le = list_next(le)) != &free_list) {
    //     struct Page *p = le2page(le, page_link);
    //     assert(PageProperty(p));
    //     count ++, total += p->property;
    // }
    // assert(total == total_size_pages());

    // basic_check();
    int tot_size = buddy_total_size_pages();
    /*check 1: 占了一半+1的空间后无法再分配*/
    struct Page *p0 = alloc_pages(tot_size/2+1);
    assert(p0 != NULL);
    struct Page *p1 = alloc_pages(1);
    assert(p1 == NULL);
    free_pages(p0, tot_size/2+1);
    unsigned char log2n = log2(tot_size);
    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n",score, sumscore);
    #endif

    /*check 2: 分配2^0,2^1 ... 2^(log2n-1),再分配1个，分配成功，再分配1个，分配失败*/
    struct Page *p[log2n+2];
    for(int i=0; i<log2n; i++){
        p[i] = alloc_pages(1<<i);
        char *s = "check 2: alloc 2^n failed";

        // s[17]= '0'+;
        assert_s(p[i] != NULL,s);
    }
    p[log2n] = alloc_pages(1);
    assert(p[log2n] != NULL);
    p[log2n+1] = alloc_pages(1);
    assert(p[log2n+1] == NULL);
    for(int i=0; i<=log2n+1; i++){
        free_pages(p[i], 1<< (i<log2n ? i : 0));
    }
    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n",score, sumscore);
    #endif
    /*check 3: 分配2^(log2n-1),2^(log2n-2) ... 2^0,再分配1个，分配成功，再分配1个，分配失败*/
    for(int i=log2n-1; i>=0; i--){
        p[i] = alloc_pages(1<<i);
        assert(p[i] != NULL);
    }
    p[log2n] = alloc_pages(1);
    assert(p[log2n] != NULL);
    p[log2n+1] = alloc_pages(1);
    assert(p[log2n+1] == NULL);
    for(int i=0; i<=log2n+1; i++){
        free_pages(p[i], 1<< (i<log2n ? i : 0));
    }
    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n",score, sumscore);
    #endif
    /*check 4: 分配n+1个，分配失败*/
    p0 = alloc_pages(tot_size+1);
    assert(p0 == NULL);
    free_page(p0);
    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n",score, sumscore);
    #endif

}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_total_size_pages,
    .check = buddy_check,
};

