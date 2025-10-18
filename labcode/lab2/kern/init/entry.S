#include <mmu.h>
#include <memlayout.h>

    .section .text,"ax",%progbits
    .globl kern_entry
kern_entry:
    # a0: hartid
    # a1: dtb physical address
    # save hartid and dtb address
    la t0, boot_hartid
    sd a0, 0(t0)
    la t0, boot_dtb
    sd a1, 0(t0)

    # t0 := 涓夌骇椤佃〃鐨勮櫄鎷熷湴鍧€
    lui     t0, %hi(boot_page_table_sv39)
    # t1 := 0xffffffff40000000 鍗宠櫄瀹炴槧灏勫亸绉婚噺
    li      t1, 0xffffffffc0000000 - 0x80000000
    # t0 鍑忓幓铏氬疄鏄犲皠鍋忕Щ閲� 0xffffffff40000000锛屽彉涓轰笁绾ч〉琛ㄧ殑鐗╃悊鍦板潃
    sub     t0, t0, t1
    # t0 >>= 12锛屽彉涓轰笁绾ч〉琛ㄧ殑鐗╃悊椤靛彿
    srli    t0, t0, 12

    # t1 := 8 << 60锛岃缃� satp 鐨� MODE 瀛楁涓� Sv39
    li      t1, 8 << 60
    # 灏嗗垰鎵嶈绠楀嚭鐨勯璁句笁绾ч〉琛ㄧ墿鐞嗛〉鍙烽檮鍔犲埌 satp 涓�
    or      t0, t0, t1
    # 灏嗙畻鍑虹殑 t0(鍗虫柊鐨凪ODE|椤佃〃鍩哄潃鐗╃悊椤靛彿) 瑕嗙洊鍒� satp 涓�
    csrw    satp, t0
    # 浣跨敤 sfence.vma 鎸囦护鍒锋柊 TLB
    sfence.vma
    # 浠庢锛屾垜浠粰鍐呮牳鎼缓鍑轰簡涓€涓畬缇庣殑铏氭嫙鍐呭瓨绌洪棿锛�
    #nop # 鍙兘鏄犲皠鐨勪綅缃湁浜沚ug銆傘€傛彃鍏ヤ竴涓猲op
    
    # 鎴戜滑鍦ㄨ櫄鎷熷唴瀛樼┖闂翠腑锛氶殢鎰忓皢 sp 璁剧疆涓鸿櫄鎷熷湴鍧€锛�
    lui sp, %hi(bootstacktop)

    # 鎴戜滑鍦ㄨ櫄鎷熷唴瀛樼┖闂翠腑锛氶殢鎰忚烦杞埌铏氭嫙鍦板潃锛�
    # 璺宠浆鍒� kern_init
    lui t0, %hi(kern_init)
    addi t0, t0, %lo(kern_init)
    jr t0

.section .data
    # .align 2^12
    .align PGSHIFT
    .global bootstack
bootstack:
    .space KSTACKSIZE
    .global bootstacktop
bootstacktop:

.section .data
    # 鐢变簬鎴戜滑瑕佹妸杩欎釜椤佃〃鏀惧埌涓€涓〉閲岄潰锛屽洜姝ゅ繀椤� 12 浣嶅榻�
    .align PGSHIFT
    .global boot_page_table_sv39
# 鍒嗛厤 4KiB 鍐呭瓨缁欓璁剧殑涓夌骇椤佃〃
boot_page_table_sv39:
    # 0xffffffff_c0000000 map to 0x80000000 (1G)
    # 鍓� 511 涓〉琛ㄩ」鍧囪缃负 0 锛屽洜姝� V=0 锛屾剰鍛崇潃鏄┖鐨�(unmapped)
    .zero 8 * 511
    # 璁剧疆鏈€鍚庝竴涓〉琛ㄩ」锛孭PN=0x80000锛屾爣蹇椾綅 VRWXAD 鍧囦负 1
    .quad (0x80000 << 10) | 0xcf # VRWXAD

    .global boot_hartid
boot_hartid:
    .quad 0
    .global boot_dtb
boot_dtb:
    .quad 0
